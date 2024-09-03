#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHESS_IMPLEMENTATION
#include "chess.h"

#define FS_IMPLEMENTATION
#include <core/fs.h>

typedef unsigned char u8;
typedef unsigned long long u64;

#define panic(...) do{						\
    fprintf(stderr, "%s:%d:ERROR: ", __FILE__, __LINE__);	\
    fflush(stderr);						\
    fprintf(stderr, __VA_ARGS__); fflush(stderr);		\
    exit(1);							\
  }while(0)

#define UNREACHABLE() panic("UNREACHABLE")
#define TODO() panic("TODO")

int main() {

  Fs_File file_stdin;
  if(fs_file_stdin(&file_stdin) != FS_ERROR_NONE) {
    TODO();
  }

  Chess_Game game;
  chess_game_default(&game);

  char buf[1024];

  while(1) {    
    chess_game_dump(&game);
    int white_or_black;
    if(chess_game_over(&game, &white_or_black)) {

      if(white_or_black) { // white
	printf("white won\n");
      } else {             // black
	printf("black won\n");
      }

      return 0;
    }

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
    buf[read] = '\0';

    if(strcmp(buf, "b") == 0) {
      chess_game_rewind(&game, game.history_len - 1);
      
    } else if(strcmp(buf, "q") == 0) {
      return 0;

    } else if(strcmp(buf, "r") == 0) {
      chess_game_default(&game);
      
    } else { 
      Chess_Move move;
      if(chess_move_from_cstr(buf, &move)) {

	printf("\tINFO: %d|(%d, %d) -> %d|(%d, %d)\n",
	       move.from,
	       move.from % CHESS_N,
	       move.from / CHESS_N,
	     
	       move.to,
	       move.to % CHESS_N,
	       move.to / CHESS_N);
      
	if(!chess_game_move(&game, &move)) {
	  printf("\tERROR: Cannot perform move\n");
	}
      
      } else {
	printf("\tERROR: Cannot parse move '%s'\n", buf);
      }
      fflush(stdout);
      
    }
    
    
    
  }

  
  return 0;
}
