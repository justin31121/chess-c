#include <stdio.h>

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

#define CHESS_IMPLEMENTATION
#include "chess.h"

#define IO_IMPLEMENTATION
#include <core/io.h>

#define STR_IMPLEMENTATION
#include <core/str.h>

typedef unsigned char u8;
typedef unsigned long long u64;

int main(int argc, char **argv) {

  if(argc < 2) {
    fprintf(stderr, "ERROR: Please provide a ip\n");
    fprintf(stderr, "USAGE: %s <ip>\n", argv[0]);
    return 1; 
  }	  

  Io_File file_stdin;
  if(io_file_stdin(&file_stdin) != IO_ERROR_NONE) {
    TODO();
  }

  Ip_Socket s;
  switch(ip_socket_copen(&s, argv[1], 4040)) {
  case IP_ERROR_NONE:
    // pass
    break;
  default:
    TODO();
  }

  Chess_Game game;
  u8 buf[1024];
  str bufs;

  printf("Connected to server. Waiting for opponent ...\n"); fflush(stdout);

  bufs = str_from(buf, sizeof(buf));
  if(ip_socket_reads(&s, bufs) != IP_ERROR_NONE) {
    TODO();
  }
  printf(str_fmt"\n", str_arg(bufs));

  int black;
  if(str_eqc(bufs, "WHITE")) {
    black = 0;
  } else if(str_eqc(bufs, "BLACK")) {
    black = 1;
  } else {
    TODO();
  }

  u64 written;
  if(ip_socket_writec(&s, (u8 *) "OK", &written) != IP_ERROR_NONE) {
    TODO();
  }

  u64 read;
  if(ip_socket_read(&s, (u8 *) &game, sizeof(game), &read) != IP_ERROR_NONE ||
     read != sizeof(game)) {
    printf("read: %llu, sizeof(game): %llu\n", read, (u64) sizeof(game)); fflush(stdout);
    TODO();
  }

  while(1) {
    printf("==================================================\n");
    chess_game_dump(&game);

    if(game.blacks_turn == black) {

      printf("> "); fflush(stdout);

      if(io_file_read(&file_stdin,
		      buf,
		      sizeof(buf),
		      &read) != IO_ERROR_NONE) {
	TODO();
      }
      if(read >= sizeof(buf)) {
	TODO();
      }
      if(read > IO_SEP_LEN) read -= IO_SEP_LEN;
      buf[read] = '\0';

      Chess_Move move;
      if(chess_move_from_cstr((char *) buf, &move)) {	
	if(ip_socket_write(&s, (u8 * ) &move, sizeof(move), &written) != IP_ERROR_NONE ||
	   written != sizeof(move)) {
	  TODO();
	}
	
	if(ip_socket_read(&s, (u8 *) &game, sizeof(game), &read) != IP_ERROR_NONE ||
	   read != sizeof(game)) {
	  TODO();
	}
	      
      } else {
	// repeat reading
	
      }
      
    } else {

      printf("Waiting for opponent ...\n"); fflush(stdout);

      Ip_Error error = ip_socket_read(&s, (u8 *) &game, sizeof(game), &read);
      switch(error) {
      case IP_ERROR_NONE:
	// pass
	break;
      case IP_ERROR_CONNECTION_CLOSED:
	printf("server closed connection. Exiting ...\n"); fflush(stdout);
	return 1;
	
	break;
      default:
	panic("ERROR: Unhandled error: %d", error);
	break;
      }
      if(read != sizeof(game)) {
	TODO();
      }
      
    }
    
  }

  ip_socket_close(&s);
  
  return 0;
}
