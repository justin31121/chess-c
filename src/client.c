#define FS_IMPLEMENTATION
#include <core/fs.h>

#define CHESS_IMPLEMENTATION
#include "chess.h"

#define STR_IMPLEMENTATION
#include <core/str.h>

#include <core/types.h>

#define IP_IMPLEMENTATION
#include <core/ip.h>

int main(int argc, char **argv) {

  if(argc < 2) {
    fprintf(stderr, "ERROR: Please provide a ip\n");
    fprintf(stderr, "USAGE: %s <ip>\n", argv[0]);
    return 1; 
  }	  
  char *hostname = argv[1];

  Fs_File file_stdin;
  if(fs_file_stdin(&file_stdin) != FS_ERROR_NONE) {
    fprintf(stderr, "ERROR: Cannot open stdin for reading\n");
    return 1;
  }

  u16 port = 4040;
  Ip_Socket s;
  if(ip_socket_copen(&s, hostname, port, 1) != IP_ERROR_NONE) {
    fprintf(stderr, "ERROR: Cannot not connect to port: %u\n", port);
    return 1;
  }

  Chess_Game game;
  chess_game_default(&game); 
  int black = -1;
  int started = 0;
  int blacks_turn = 0;

  str message = {0};

  u8 buf[1024];
  u64 buf_len = 0;
  Chess_Move move;
  while(1) {
    if(!started) {
      int try_again = 0;
      u64 read;
      switch(ip_socket_read(&s, buf, sizeof(buf), &read)) {
      case IP_ERROR_REPEAT:
	try_again = 1;
	break;
      case IP_ERROR_NONE:
	break;
      default:
	TODO();
      }
      if(try_again) continue;
      str received_message = str_from(buf, read);
      if(str_eqc(received_message, "w")) {
	black = 0;
      } else if(str_eqc(received_message, "b")) {
	black = 1;
      } else {
	TODO();
      }
      started = 1;
      blacks_turn = 0;

    } else {

      if(black == blacks_turn) {

	if(message.len > 0) {
	  // Its your turn, but you didn't send your move yet. Writing to server ...

	  u64 written;
	  switch(ip_socket_write(&s, message.data, message.len, &written)) {
	  case IP_ERROR_NONE:
	    message = str_from(message.data + written, message.len - written);
	    break;
	  case IP_ERROR_REPEAT:
	    // Do nothing, try again to write in next iteration
	    break;
	  default:
	    TODO();
	  }							

	  if(message.len == 0) {
	    // After the message fully transmitted, switch the turn
	    blacks_turn = 1 - blacks_turn;	
	    buf_len = 0;
	  }
	} else {
	  // Its your turn, reading from stdin ...

	  chess_game_dump(&game);
	  printf("> "); fflush(stdout);

	  u64 read;
	  if(fs_file_read(&file_stdin,
			  buf,
			  sizeof(buf),
			  &read) != FS_ERROR_NONE) {
	    TODO();
	  }
	  if(read >= sizeof(buf)) {
	    TODO();
	  }
	  if(read > FS_SEP_LEN) read -= FS_SEP_LEN;
	  buf[read] ='\0';

	  if(chess_move_from_cstr((char *) buf, &move) && chess_game_move(&game, &move)) {	
	    // prepare message
	    message = str_from((u8 *) &move, sizeof(move));	
	  } else {
	    // keep reading from stdin
	  }
	}

      } else { // white
	// Wait for completed message from server. Reading from server ...

	int try_again = 0;
	u64 read;
	switch(ip_socket_read(&s, buf + buf_len, sizeof(buf) - buf_len, &read)) {
	case IP_ERROR_REPEAT:
	  try_again = 1;
	  break;
	case IP_ERROR_NONE:
	  buf_len += read;
	  break;
	default:
	  TODO();
	}
	if(try_again) continue;

	if(buf_len < sizeof(Chess_Move)) {
	  // Keep reading ...
	} else if(buf_len == sizeof(Chess_Move)) {
	  if(!chess_game_move(&game, (Chess_Move *) buf)) TODO();
	  blacks_turn = 1 - blacks_turn;
	} else { // buf_len > sizeof(Chess_Move)
	  TODO();
	}
      }
    }
  }

  ip_socket_close(&s);


  return 0;
}
