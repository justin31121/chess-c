#include <stdio.h>
#include <assert.h>

#define panic(...) do{						\
    fprintf(stderr, "%s:%d:ERROR: ", __FILE__, __LINE__);	\
    fflush(stderr);						\
    fprintf(stderr, __VA_ARGS__); fflush(stderr);		\
    exit(1);							\
  }while(0)

#define UNREACHABLE() panic("UNREACHABLE")
#define TODO() panic("TODO")

#define IP_IMPLEMENTATION
#include <core/ip.h>

#define STR_IMPLEMENTATION
#include <core/str.h>

#define CHESS_IMPLEMENTATION
#include "chess.h"

typedef unsigned char u8;
typedef unsigned long long u64;

typedef struct {
  int black;
  int okayed;

  u64 id;
} Player;

typedef struct {
  Player players[2];
  int announced;
} Session;

int session_init(Ip_Server *server,
		 Session *session) {

  if(server->active_clients != 2) {
    return 0;
  }
  session->announced = 0;
  
  int once = 1;		
  for(u64 i=0;i<server->sockets_count;i++) {
    Ip_Socket *s = &server->sockets[i];
    if(!(s->flags & IP_VALID)) {
      continue;
    }

    if(!(s->flags & IP_CLIENT)) {
      continue;
    }

    session->players[1 - once] = (Player) {
      .black  = once,
      .okayed = 0,
      .id     = i,
    };

    str message;
    if(once) {
      message = str_fromc("WHITE");
      once = 0;	    
    } else { // white
      message = str_fromc("BLACK");
    }

    u64 written;
    switch(ip_socket_writes(s, message, &written)) {
    case IP_ERROR_NONE:
      // pass
      break;
    default:
      TODO();
      break;
    }
    if(written != message.len) {
      TODO();
    }

  }

  return 1;
}

void session_update_players(Ip_Server *server,
			    Session *session,
			    Chess_Game *game) {
  
  for(u64 i=0;i<sizeof(session->players)/sizeof(*session->players);i++) {
    Player *player = &session->players[i];
    Ip_Socket *socket = &server->sockets[player->id];

    u64 written;
    switch(ip_socket_write(socket, (u8 *) game, sizeof(*game), &written)) {
    case IP_ERROR_NONE:
      // pass
      break;
    default:
      TODO();
      break;
    }
    if(written != sizeof(*game)) {
      TODO();
    }
  }

}

int session_announce_player(Ip_Server *server, Session *session, Chess_Game *game, u64 id) {

  int all_okay = 1;
  for(u64 i=0;i<sizeof(session->players)/sizeof(*session->players);i++) {
    Player *player = &session->players[i];
    
    if(player->id == id) {
      if(player->okayed) {
	return 0;
      } else {
	player->okayed = 1;
      }
    }

    all_okay = all_okay && player->okayed;
  }

  if(!session->announced && all_okay) {
    session_update_players(server, session, game);
    session->announced = 1;    
  }
  
  return 1;  
}

int main() {  

  Ip_Server server;
  switch(ip_server_open(&server,
			4040,
			2)) {
  case IP_ERROR_NONE:
    break;
  default:
    TODO();
    break;
  }

  printf("Listening on port 4040 for incoming connections\n"); fflush(stdout);

  Session session;
  Chess_Game game;
  printf("sizeof(Game): %llu\n", (u64) sizeof(game)); fflush(stdout);
  assert(sizeof(game) == 1024);
  
  u8 buf[1024];
  while(1) {
    
    u64 index;

    int try_again = 0;
    switch(ip_server_next(&server, &index)) {
    case IP_ERROR_REPEAT:
      try_again = 1;
      break;
    case IP_ERROR_NONE:
      // pass
      break;
    default:
      TODO();
      break;      
    }
    if(try_again) {
      continue;
    }
    
    Ip_Socket *socket = &server.sockets[index];            
    if(socket->flags & IP_SERVER) {

      u64 client_index;
      Ip_Address address;
      switch(ip_server_accept(&server, &client_index, &address)) {
      case IP_ERROR_REPEAT:
	break;
      case IP_ERROR_NONE:
	printf("client %llu connected. Active clients: %llu\n",
	       client_index,
	       server.active_clients); fflush(stdout);
	break;
      default:
	TODO();
	break;
      }

      // Try to Initialize the game
      if(session_init(&server, &session)) {
	chess_game_default(&game);
      }
      
    } else {

      u64 read;
      switch(ip_socket_read(socket, buf, sizeof(buf), &read)) {
      case IP_ERROR_NONE:
	// pass
	break;
      case IP_ERROR_CONNECTION_CLOSED:
	ip_server_discard(&server, index);
	  
	printf("client %llu disconnected. Active clients: %llu\n",
	       index,
	       server.active_clients); fflush(stdout);

	// Abort the game / Disconnect all clients
	for(u64 i=0;i<server.sockets_count;i++) {
	  Ip_Socket *s = &server.sockets[i];
	  if(!(s->flags & IP_VALID)) {
	    continue;
	  }

	  if(!(s->flags & IP_CLIENT)) {
	    continue;
	  }

	  ip_socket_close(s);
	  server.active_clients--;

	  printf("\tClosed connection to client %llu. Active clients: %llu\n",
		 i,
		 server.active_clients); fflush(stdout);
	}

	break;
      default:
	TODO();
	break;
      }
	
      if(!(socket->flags & IP_VALID)) {
	continue;
      }

      if(server.active_clients != 2) {
	continue;
      }

      if(!session.announced) {
	if(session_announce_player(&server,
				   &session,
				   &game,
				   index)) {
	  continue;
	}
      }
      if(!session.announced) {
	continue;
      }
	
      Chess_Move move = *(Chess_Move *) buf;
      printf("\tINFO: %d|(%d, %d) -> %d|(%d, %d)\n",
	     move.from,
	     move.from % CHESS_N,
	     move.from / CHESS_N,
	     move.to,
	     move.to % CHESS_N,
	     move.to / CHESS_N); fflush(stdout);

      chess_game_move(&game, &move);
      session_update_players(&server, &session, &game);
	
    }


    
  }

  return 0;
}
