#ifndef CHESS_H
#define CHESS_H

#ifndef CHESS_DEF
#  define CHESS_DEF static inline
#endif // CHESS_DEF

#ifndef CHESS_ASSERT
#  include <assert.h>
#  define CHESS_ASSERT assert
#endif // CHESS_ASSERT

typedef enum {
  CHESS_KIND_NONE = 0,
  CHESS_KIND_PAWN,
  CHESS_KIND_KNIGHT,
  CHESS_KIND_BISHOP,
  CHESS_KIND_ROOK,
  CHESS_KIND_QUEEN,
  CHESS_KIND_KING,
} Chess_Kind;

typedef struct {
  Chess_Kind kind;
  int black;
} Chess_Piece;

CHESS_DEF void chess_piece_from_char(char c, Chess_Piece *p); 

typedef struct {
  int from;
  int to;
} Chess_Move;	

CHESS_DEF int chess_move_eq(Chess_Move *a, Chess_Move *b); 
CHESS_DEF int chess_move_from_cstr(char *cstr, Chess_Move *move);

#define CHESS_N 8
#define CHESS_HISTORY_CAP 63

typedef struct {
  int blacks_turn;
  Chess_Piece board[CHESS_N * CHESS_N];
  Chess_Move history[CHESS_HISTORY_CAP];
  int history_len;
} Chess_Game;

CHESS_DEF void chess_game_reset(Chess_Game *g); 
CHESS_DEF void chess_game_default(Chess_Game *g); 
CHESS_DEF void chess_game_dump(Chess_Game *g);
CHESS_DEF void chess_game_rewind(Chess_Game *g, int rewind_to);
CHESS_DEF int chess_game_is_check(Chess_Game *g);
CHESS_DEF int chess_game_move(Chess_Game *g, Chess_Move *m);
CHESS_DEF int chess_game_over(Chess_Game *g, int *white_or_black_won);

CHESS_DEF int chess_game_check_path(Chess_Game *g,
				    int src_x,
				    int src_y,
				    int dst_x,
				    int dst_y,
				    int dx_n,
				    int dy_n);
CHESS_DEF int chess_game_validate_move_bishop(Chess_Game *g,
					      int src_x,
					      int src_y,
					      int dst_x,
					      int dst_y,
					      int dx_abs,
					      int dy_abs,
					      int dx_n,				    
					      int dy_n);
CHESS_DEF int chess_game_validate_move_rook(Chess_Game *g,
					    int src_x,
					    int src_y,
					    int dst_x,
					    int dst_y,
					    int dx_abs,
					    int dy_abs,
					    int dx_n,
					    int dy_n);
CHESS_DEF int chess_game_piece_has_moved(Chess_Game *g, int pos);
CHESS_DEF int chess_game_validate_move(Chess_Game *g, Chess_Move *m);
CHESS_DEF void chess_game_perform_move_impl(Chess_Game *g, Chess_Move *m);
CHESS_DEF void chess_game_perform_move(Chess_Game *g, Chess_Move *m);
CHESS_DEF int chess_game_king_position(Chess_Game *g);
CHESS_DEF int chess_game_available_moves(Chess_Game *g);

#ifdef CHESS_IMPLEMENTATION

char chess_kind_char[] = {
  [CHESS_KIND_NONE]   = '_',
  [CHESS_KIND_PAWN]   = 'p',
  [CHESS_KIND_KNIGHT] = 'n',
  [CHESS_KIND_BISHOP] = 'b',
  [CHESS_KIND_ROOK]   = 'r',
  [CHESS_KIND_QUEEN]  = 'q',
  [CHESS_KIND_KING]   = 'k',
};

CHESS_DEF void chess_piece_from_char(char c, Chess_Piece *p) {
  if(c == '_') {
    *p = (Chess_Piece) { .kind = CHESS_KIND_NONE };
    return;
  }
  
  if('A' <= c && c <= 'Z') {
    c += ' ';
    p->black = 1; // black
  } else {
    
    p->black = 0; // white
  }

  switch(c) {
  case 'r':
    p->kind = CHESS_KIND_ROOK;
    break;
  case 'n':
    p->kind = CHESS_KIND_KNIGHT;
    break;
  case 'b':
    p->kind = CHESS_KIND_BISHOP;
    break;
  case 'q':
    p->kind = CHESS_KIND_QUEEN;
    break;
  case 'k':
    p->kind = CHESS_KIND_KING;
    break;
  case 'p':
    p->kind = CHESS_KIND_PAWN;
    break;
  default:
    fprintf(stderr, "ERROR: Unknown char '%c' in char_from_piece\n", c);
    fflush(stderr);
    exit(1);
  }
}	

CHESS_DEF int chess_move_eq(Chess_Move *a, Chess_Move *b) {
  return
    a->from == b->from &&
    a->to == b->to;    
}

CHESS_DEF int chess_move_from_cstr(char *cstr, Chess_Move *move) {

  int x, y;
  
  if(!cstr) return 0;
  if(*cstr < 'a' || 'h' < *cstr) return 0;
  x = *cstr - 'a';
  cstr++;

  if(!cstr) return 0;
  if(*cstr < '1' || '8' < *cstr) return 0;
  y = (CHESS_N - 1) - (*cstr - '1');
  cstr++;
  move->from = y * CHESS_N + x;

  if(!cstr) return 0;
  if(*cstr != ' ') return 0;
  cstr++;

  if(!cstr) return 0;
  if(*cstr < 'a' || 'h' < *cstr) return 0;
  x = *cstr - 'a';
  cstr++;

  if(!cstr) return 0;
  if(*cstr < '1' || '8' < *cstr) return 0;
  y = (CHESS_N - 1) - (*cstr - '1');
  cstr++;
  move->to = y * CHESS_N + x;

  if(*cstr) return 0;

  return 1;
}

// White
static Chess_Move CHESS_MOVE_CASTLE_WHITE_RIGHT = {
  .from = (CHESS_N-1) * CHESS_N + 4,
  .to   = (CHESS_N-1) * CHESS_N + (CHESS_N-2)
};
static Chess_Move CHESS_MOVE_CASTLE_WHITE_RIGHT_rook = {
  .from = (CHESS_N-1) * CHESS_N + (CHESS_N-1),
  .to   = (CHESS_N-1) * CHESS_N + 5,
};

static Chess_Move CHESS_MOVE_CASTLE_WHITE_LEFT = {
  .from = (CHESS_N-1) * CHESS_N + 4,
  .to   = (CHESS_N-1) * CHESS_N + 2
};
static Chess_Move CHESS_MOVE_CASTLE_WHITE_LEFT_rook = {
  .from = (CHESS_N-1) * CHESS_N + 0,
  .to   = (CHESS_N-1) * CHESS_N + 3,
};

// Black
static Chess_Move CHESS_MOVE_CASTLE_BLACK_RIGHT = {
  .from = 0 * CHESS_N + 4,
  .to   = 0 * CHESS_N + (CHESS_N-2),
};
static Chess_Move CHESS_MOVE_CASTLE_BLACK_RIGHT_rook = {
  .from = 0 * CHESS_N + (CHESS_N-1),
  .to   = 0 * CHESS_N + 5,
};

static Chess_Move CHESS_MOVE_CASTLE_BLACK_LEFT = {
  .from = 0 * CHESS_N + 4,
  .to   = 0 * CHESS_N + 2,
};
static Chess_Move CHESS_MOVE_CASTLE_BLACK_LEFT_rook = {
  .from = 0 * CHESS_N + 0,
  .to   = 0 * CHESS_N + 3,
};

CHESS_DEF void chess_game_reset(Chess_Game *g) {
  char *INITIAL_BOARD =
    "RNBQKBNR"
    "PPPPPPPP"
    "________"
    "________"
    "________"
    "________"
    "pppppppp"
    "rnbqkbnr"
    ;
  
  for(int j=0;j<CHESS_N;j++) {
    for(int i=0;i<CHESS_N;i++) {
      chess_piece_from_char(INITIAL_BOARD[j * CHESS_N + i], &g->board[j * CHESS_N + i]);
    }
  }
  g->blacks_turn = 0;
}

CHESS_DEF void chess_game_default(Chess_Game *g) {
  chess_game_reset(g);
  g->history_len = 0;
}

CHESS_DEF void chess_game_dump(Chess_Game *g) {
  for(int j=0;j<CHESS_N;j++) {
    for(int i=0;i<CHESS_N;i++) {
      if(i == 0) {
	printf("%d  ", 8 - j);
      }
      
      Chess_Piece piece = g->board[j * CHESS_N + i];
      char c = chess_kind_char[piece.kind];

      if(piece.black) {
	c -= ' ';
      }
      
      printf("%c ", c);
    }
    printf("\n");
  }
  printf("\n");
  printf("   a b c d e f g h\n");
  fflush(stdout);
}

CHESS_DEF int chess_game_over(Chess_Game *g, int *white_or_black_won) {
  if(chess_game_available_moves(g) > 0) {
    return 0;
  }
  *white_or_black_won = g->blacks_turn;
  return 1;

}

CHESS_DEF int chess_game_available_moves(Chess_Game *g) {
  int moves = 0;
  
  for(int k=0;k<CHESS_N*CHESS_N;k++) {
    Chess_Piece p = g->board[k];
    if(p.kind == CHESS_KIND_NONE) {
      continue;
    }
      
    if(p.black != g->blacks_turn) {
      continue;
    }
    
    for(int l=0;l<CHESS_N*CHESS_N;l++) {

      Chess_Move move = { .from = k, .to = l };
      if(!chess_game_validate_move(g, &move)) {
	continue;
      }

      chess_game_perform_move(g, &move);      
      if(!chess_game_is_check(g)) {
	moves++;
      }
      chess_game_rewind(g, g->history_len - 1);
      
    }
    
  }

  return moves;

}

CHESS_DEF int chess_game_validate_move(Chess_Game *g, Chess_Move *m) {
  // source and destination cannot be equal
  if(m->from == m->to) {
    return 0;
  }

  int src_x = m->from % CHESS_N;
  int src_y = m->from / CHESS_N;

  int dst_x = m->to % CHESS_N;
  int dst_y = m->to / CHESS_N;

  // all moves must happen inside the board
  if(src_x < 0 || CHESS_N <= src_x ||
     src_y < 0 || CHESS_N <= src_y) {
    return 0;
  }

  Chess_Piece piece = g->board[m->from];

  // only pieces can move
  if(piece.kind == CHESS_KIND_NONE) {
    return 0;
  }

  // only pieces from the right turn can move
  if(piece.black != g->blacks_turn) {
    return 0;
  }

  // pieces can not move on the same color
  Chess_Piece dest = g->board[m->to];
  if(dest.kind != CHESS_KIND_NONE &&
     dest.black == piece.black) {
    return 0;
  }

  int dx = dst_x - src_x;
  int dy = dst_y - src_y;

  int dx_abs;
  if(dx < 0) {
    dx_abs = -1 * dx;
  } else {
    dx_abs = dx;
  }

  int dy_abs;
  if(dy < 0) {
    dy_abs = -1 * dy;
  } else {
    dy_abs = dy;
  }

  int dx_n;
  if(dx > 0) {
    dx_n = 1;
  } else if(dx < 0) {
    dx_n = -1;
  } else {
    dx_n = 0;
  }

  int dy_n;
  if(dy > 0) {
    dy_n = 1;
  } else if(dy < 0) {
    dy_n = -1;
  } else {
    dy_n = 0;
  }

  switch(piece.kind) {

  case CHESS_KIND_NONE:
    return 0; // unreachable
    
  case CHESS_KIND_PAWN:
    // TODO: implement 'En passant'

    // pawns must move vertically
    if(dy == 0) { 
      return 0;
    }

    // pawns must move towards the enemy
    if(piece.black) {
      if(dy < 0) { // up
	return 0;
      } else {     // down
	
      }
    } else { // piece.write
      if(dy < 0) { // up
	
      } else {     // down
	return 0;
      }
    }
    
    // pawns must move vertically 1 or 2 steps
    if(dy_abs == 1) {

      if(dx == 0) { // dx == 0, dy_abs == 1

	// pawns can not move, if something
	// is in front of them
	if(dest.kind == CHESS_KIND_NONE) {
	  
	} else {
	  return 0;
	}
	 
      } else { // (dx != 0) => (dx_abs > 0)

	// pawns can only move vertically 1 and
	// horizontally 1, if they capture
	if(dx_abs == 1) {  // dy_abs == 1, dx_abs == 1

	  if(dest.kind != CHESS_KIND_NONE &&
	     dest.black != piece.black) {
	    
	  } else {
	    return 0;
	  }
	  
	} else {
	  return 0;
	}
	
      }

    } else if(dy_abs == 2) {

      // pawns can only move vertically 2 steps,
      // if horizontal step is 0 and they are in
      // there initial position and there ist nothing
      if(dx != 0) {
	return 0;
      }

      if(dest.kind != CHESS_KIND_NONE) {
	return 0;
      }

      if(piece.black) {
	if(src_y == 1) {
	  
	} else {
	  return 0;
	}
	
      } else { // piece.white
	if(src_y == (CHESS_N-2)) {
	  
	} else {
	  return 0;
	}
	
      }
      
    } else { // dy_abs > 2
      return 0;
    }
    
    break;

  case CHESS_KIND_KNIGHT:

    // knights can either move: 
    // horizontally 2 and vertically 1 or
    // horizontally 1 and vertically 2
    if(dx_abs == 2 && dy_abs == 1) {
      
    } else if(dx_abs == 1 && dy_abs == 2) {
      
    } else {
      return 0;
    }

    break;

  case CHESS_KIND_BISHOP:

    if(!chess_game_validate_move_bishop(g,
					src_x,
					src_y,
					dst_x,
					dst_y,
					dx_abs,
					dy_abs,
					dx_n,
					dy_n)) {
      return 0;
    }
    
    break;

  case CHESS_KIND_ROOK:

    if(!chess_game_validate_move_rook(g,
				      src_x,
				      src_y,
				      dst_x,
				      dst_y,
				      dx_abs,
				      dy_abs,
				      dx_n,
				      dy_n)) {
      return 0;
    }

    break;

  case CHESS_KIND_QUEEN:

    ; // -pedantic
    int good = 0;
    if(chess_game_validate_move_bishop(g,
				       src_x,
				       src_y,
				       dst_x,
				       dst_y,
				       dx_abs,
				       dy_abs,
				       dx_n,
				       dy_n)) {
      good = 1;
    }

    if(!good && 
       chess_game_validate_move_rook(g,
				     src_x,
				     src_y,
				     dst_x,
				     dst_y,
				     dx_abs,
				     dy_abs,
				     dx_n,
				     dy_n)) {
      good = 1;
    }

    if(!good) {
      return 0;
    }

    break;

  case CHESS_KIND_KING:
    
    if(dx_abs <= 1 && dy_abs <= 1) {
      // kings can move 1 step vertically and horizontally
      // in every direction

      
    } else {
      // Kings may 'castle'

      if(!chess_game_check_path(g,
				src_x,
				src_y,
				dst_x,
				dst_y,
				dx_n,
				dy_n)) {
	return 0;
      }

      // TODO: Must the path be unattacked by the opponent?

      int king_pos;
      int rook_pos;
      if(piece.black) {

	if(chess_move_eq(m, &CHESS_MOVE_CASTLE_BLACK_LEFT)) {
	  king_pos = CHESS_MOVE_CASTLE_BLACK_LEFT.from;
	  rook_pos = CHESS_MOVE_CASTLE_BLACK_LEFT_rook.from;
	} else if(chess_move_eq(m, &CHESS_MOVE_CASTLE_BLACK_RIGHT)) {
	  king_pos = CHESS_MOVE_CASTLE_BLACK_RIGHT.from;
	  rook_pos = CHESS_MOVE_CASTLE_BLACK_RIGHT_rook.from;
	} else {
	  return 0;
	}
	
      } else { // piece.white
	
	if(chess_move_eq(m, &CHESS_MOVE_CASTLE_WHITE_LEFT)) {
	  king_pos = CHESS_MOVE_CASTLE_WHITE_LEFT.from;
	  rook_pos = CHESS_MOVE_CASTLE_WHITE_LEFT_rook.from;
	} else if(chess_move_eq(m, &CHESS_MOVE_CASTLE_WHITE_RIGHT)) {
	  king_pos = CHESS_MOVE_CASTLE_WHITE_RIGHT.from;
	  rook_pos = CHESS_MOVE_CASTLE_WHITE_RIGHT_rook.from;
	} else {
	  return 0;
	}
	
      }

      if(chess_game_piece_has_moved(g, king_pos) ||
	 chess_game_piece_has_moved(g, rook_pos)) {
	return 0;
      }
      
    }
    
    break;
    
  }
  
  return 1;
    
}

CHESS_DEF void chess_game_perform_move_impl(Chess_Game *g, Chess_Move *m) {
  g->board[m->to] = g->board[m->from];
  g->board[m->from] = (Chess_Piece) { .kind = CHESS_KIND_NONE };  
}

CHESS_DEF void chess_game_perform_move(Chess_Game *g, Chess_Move *m) {
  chess_game_perform_move_impl(g, m);
  
  if(g->history_len == CHESS_HISTORY_CAP) {
    fprintf(stderr, "ERROR: history-overflow\n");
    fflush(stderr);
    exit(1);
  }
  g->history[g->history_len++] = *m;

  Chess_Move *rook_move = NULL;
  if(chess_move_eq(m, &CHESS_MOVE_CASTLE_WHITE_LEFT)) {
    rook_move = &CHESS_MOVE_CASTLE_WHITE_LEFT_rook;
  } else if(chess_move_eq(m, &CHESS_MOVE_CASTLE_WHITE_RIGHT)) {
    rook_move = &CHESS_MOVE_CASTLE_WHITE_RIGHT_rook;
  } else if(chess_move_eq(m, &CHESS_MOVE_CASTLE_BLACK_LEFT)) {
    rook_move = &CHESS_MOVE_CASTLE_BLACK_LEFT_rook;
  } else if(chess_move_eq(m, &CHESS_MOVE_CASTLE_BLACK_RIGHT)) {
    rook_move = &CHESS_MOVE_CASTLE_BLACK_RIGHT_rook;
  }

  if(rook_move) {
    chess_game_perform_move_impl(g, rook_move);
  }

  g->blacks_turn = 1 - g->blacks_turn;
  
}

CHESS_DEF int chess_game_is_check(Chess_Game *g) {
  int king_pos = chess_game_king_position(g);

  for(int k=0;k<CHESS_N*CHESS_N;k++) {
    Chess_Piece p = g->board[k];
    if(p.kind == CHESS_KIND_NONE) {
      continue;
    }
      
    if(p.black != g->blacks_turn) {
      continue;
    }

    Chess_Move move = { .from = k, .to = king_pos };
    if(chess_game_validate_move(g, &move)) {      
      return 1;
    }
      
  }

  return 0;

}

CHESS_DEF void chess_game_rewind(Chess_Game *g, int rewind_to) {
  CHESS_ASSERT(0 <= rewind_to && rewind_to < g->history_len);
  
  chess_game_default(g);
  
  for(int i=0;i<rewind_to;i++) {    
    CHESS_ASSERT(chess_game_move(g, &g->history[i]));
  }

}

CHESS_DEF int chess_game_validate_move_bishop(Chess_Game *g,
					      int src_x,
					      int src_y,
					      int dst_x,
					      int dst_y,
					      int dx_abs,
					      int dy_abs,
					      int dx_n,				    
					      int dy_n) {
  // bishops can only move
  // diagonally =>
  //     dx_abs == dy_abs
  if(dx_abs != dy_abs) {
    return 0;
  }

  if(!chess_game_check_path(g,
			    src_x,
			    src_y,
			    dst_x,
			    dst_y,
			    dx_n,
			    dy_n)) {
    return 0;
  }

  return 1;

}

CHESS_DEF int chess_game_validate_move_rook(Chess_Game *g,
					    int src_x,
					    int src_y,
					    int dst_x,
					    int dst_y,
					    int dx_abs,
					    int dy_abs,
					    int dx_n,
					    int dy_n) {
  // rooks can only move
  // diametrally =>
  //     (dx_abs > 0 && dy_abs == 0) || (dx_abs == 0 && dy_abs > 0)
  if(dx_abs > 0) {

    if(dy_abs > 0) { // dx_abs > 0, dy_abs > 0
      return 0;
    } else {         // dx_abs > 0, dy_abs == 0
	
    }
      
  } else { // (0 <= dx_abs && dx_abs <= 0) => dx_abs == 0

    if(dy_abs > 0) { // dx_abs == 0, dy_abs > 0
	
    } else {         // dx_abs == 0, dy_abs == 0
      return 0;
    }
      
  }

  if(!chess_game_check_path(g,
			    src_x,
			    src_y,
			    dst_x,
			    dst_y,
			    dx_n,
			    dy_n)) {
    return 0;
  }

  return 1;
  
}

CHESS_DEF int chess_game_check_path(Chess_Game *g,
				    int src_x,
				    int src_y,
				    int dst_x,
				    int dst_y,
				    int dx_n,
				    int dy_n) {
  int x = src_x + dx_n;
  int y = src_y + dy_n;

  while(x != dst_x || y != dst_y) {
    if(g->board[y * CHESS_N + x].kind != CHESS_KIND_NONE) {
      return 0;
    }

    x += dx_n;
    y += dy_n;
  }

  return 1;

}

CHESS_DEF int chess_game_piece_has_moved(Chess_Game *g, int pos) {
  for(int i=0;i<g->history_len;i++) {
    Chess_Move move = g->history[i];

    if(move.from == pos) {
      return 1;
    }
  }

  return 0;

}

CHESS_DEF int chess_game_king_position(Chess_Game *g) {
  int king_pos = -1;
  for(int k=0;k<CHESS_N*CHESS_N;k++) {
    Chess_Piece p = g->board[k];
    if(p.kind != CHESS_KIND_KING) {
      continue;
    }
      
    if(p.black == g->blacks_turn) {
      continue;
    }

    king_pos = k;
  }
  CHESS_ASSERT(0 <= king_pos && king_pos < CHESS_N*CHESS_N);

  return king_pos;

}

CHESS_DEF int chess_game_move(Chess_Game *g, Chess_Move *m) {
  if(!chess_game_validate_move(g, m)) {
    return 0;
  }
  
  chess_game_perform_move(g, m); 

  if(chess_game_is_check(g)) {
    chess_game_rewind(g, g->history_len - 1);
    return 0;
  }
  
  return 1;

}

#endif // CHESS_IMPLEMENTATION

#endif // CHESS_H
