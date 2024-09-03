#include <stdio.h>

#include <core/types.h>

#define IP_IMPLEMENTATION
#include <core/ip.h>

#define STR_IMPLEMENTATION
#include <core/str.h>

#define CHESS_IMPLEMENTATION
#include "chess.h"

typedef struct {
  str message;
  Ip_Socket *socket;
} Player;

void abort_game(Ip_Sockets *s, Chess_Game *game, Player *players, u64 index) {
  printf("Client %llu disconnected\n", index);
  if(ip_sockets_unregister(s, index) != IP_ERROR_NONE) TODO();
  *players[index].socket = ip_socket_invalid();

  u64 other_index = 1 - index;
  if(players[other_index].socket->flags & IP_VALID) {
    printf("Closing connection to Client %llu\n", other_index);	  
    if(ip_sockets_unregister(s, other_index) != IP_ERROR_NONE) TODO();
    ip_socket_close(players[other_index].socket);
    *players[other_index].socket = ip_socket_invalid();	  
  }

  chess_game_default(game);
  memset(players, 0, sizeof(Player) * 2);
  players[0].socket = &s->sockets[0];
  players[1].socket = &s->sockets[1];
  
}

int main() {
  Ip_Sockets sockets;
  if(ip_sockets_open(&sockets, 3) != IP_ERROR_NONE) {
    TODO();
  }
  if(ip_socket_sopen(&sockets.sockets[2], 4040, 0) != IP_ERROR_NONE) {
    TODO();
  }
  if(ip_sockets_register(&sockets, 2) != IP_ERROR_NONE) {
    TODO();
  }

  printf("Listening on port 4040 for incoming connections\n"); fflush(stdout);

  Chess_Game game;
  chess_game_default(&game);
  Player players[2] = {0};
  players[0].socket = &sockets.sockets[0];
  players[1].socket = &sockets.sockets[1];

  u8 buf[1024];
  u64 buf_len = 0;

  Chess_Move move;
  Ip_Address address;
  while(1) {
    int try_again = 0;

    u64 index;
    Ip_Mode mode;
    switch(ip_sockets_next(&sockets, &index, &mode)) {
    case IP_ERROR_REPEAT:
      try_again = 1;	
      break;
    case IP_ERROR_NONE:
      break;
    default:
      TODO();
    }
    if(try_again) {
      continue;
    }

    Ip_Socket *s = &sockets.sockets[index];
    if(s->flags & IP_SERVER) {
      if(mode != IP_MODE_READ) TODO();
			
      Ip_Socket *client = players[0].socket;
      u64 client_index = 0;
      if(client->flags & IP_VALID) {
	client = players[1].socket;
	client_index = 1;
      }
      if(client->flags & IP_VALID) TODO();
      if(ip_socket_accept(s, client, &address) != IP_ERROR_NONE) {
	TODO();
      }
      if(ip_sockets_register(&sockets, client_index) != IP_ERROR_NONE) {
	TODO();
      }
      printf("Client %llu connected\n", client_index);

      if((players[0].socket->flags & IP_VALID) &&
	 (players[1].socket->flags & IP_VALID)) {
	printf("Starting the game\n");

	players[0].message = str_from((u8 *) "w", 1);
	players[1].message = str_from((u8 *) "b", 1); 
	players[0].socket->flags |= IP_WRITING;
	players[1].socket->flags |= IP_WRITING;
      }

    } else { // s->flags & IP_CLIENT

      switch(mode) {
      case IP_MODE_READ: {

	int keep_reading = 1;
	while(keep_reading) {
	  /* if(index != game.blacks_turn) TODO(); */

	  u64 read;
	  Ip_Error error = ip_socket_read(s,
					  buf + buf_len,
					  sizeof(buf) - buf_len,
					  &read);
	  switch(error) {
	  case IP_ERROR_NONE:
	    buf_len += read;

	    if(buf_len < sizeof(Chess_Move)) {
	      // Keep reading ...
	    } else if(buf_len == sizeof(Chess_Move)) {
	      if(!chess_game_move(&game, (Chess_Move *) buf)) TODO();
	      buf_len = 0;

	      u64 other_index = 1 - index;
	      memcpy(&move, buf, sizeof(Chess_Move));
	      players[other_index].message = str_from((u8 *) &move, sizeof(Chess_Move));
	      players[other_index].socket->flags |= IP_WRITING;
	      keep_reading = 0;
	    } else { //buf_len > sizeof(buf_len)
	      TODO();
	    }
	    
	    break;
	  case IP_ERROR_REPEAT:
	    keep_reading = 0;
	    break;
	  case IP_ERROR_EOF:
	    abort_game(&sockets, &game, players, index);
	    buf_len = 0;
	    sockets.ret = -1;
	    keep_reading = 0;
	    break;
	  default:
	    TODO();
	  }

	}

      } break;

      case IP_MODE_WRITE: {
	Player *player = &players[index];
	if(player->message.len == 0) {
	  TODO();
	}

	int keep_writing = 1;
	while(keep_writing && player->message.len > 0) {
	  u64 written;
	  Ip_Error error = ip_socket_write(s, 
					   player->message.data,
					   player->message.len,
					   &written);
	  switch(error) {
	  case IP_ERROR_NONE:
	    player->message = str_from(player->message.data + written, player->message.len - written);
	    break;
	  case IP_ERROR_REPEAT:
	    keep_writing = 0;
	    break;
	  default:
	    printf("%d\n", error);
	    TODO();
	  }
	}

	if(player->message.len == 0) {
	  player->socket->flags &= ~IP_WRITING;
	}
      } break;


      case IP_MODE_DISCONNECT: {
	abort_game(&sockets, &game, players, index);
	buf_len = 0;
	sockets.ret = -1;
      } break;
      }
      
    }
  }

  ip_sockets_close(&sockets);

}
