#include <stdio.h>

#define FRAME_IMPLEMENTATION
#include <core/frame.h>

#define RENDERER_IMPLEMENTATION
#include <core/renderer.h>

#define MUI_IMPLEMENTATION
#include <core/mui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb_image.h>

#include <core/types.h>

#define CHESS_IMPLEMENTATION
#include "chess.h"

#define WIDTH 800
#define HEIGHT 800

void mui_render(void *userdata, Mui* m) {
  Renderer *r = userdata;
  
  if(m->type == MUI_TYPE_TEXTURE) {
    Renderer_Texture *t = m->userdata;

    if(t->is_grey) {
      m->c1.w = -1 * m->c1.w;
      m->c2.w = -1 * m->c2.w;
      m->c3.w = -1 * m->c3.w;
    } else {
      m->c1.w = -1 - m->c1.w;
      m->c2.w = -1 - m->c2.w;
      m->c3.w = -1 - m->c3.w;
    }

    if(r->texture_active != t->index) {
      renderer_end(r);
      r->texture_active = t->index;
      glUniform1i(glGetUniformLocation(r->program, "tex"), r->texture_active);
    }    
  }  

  renderer_triangle(r,
		    cast(Renderer_Vec2f, m->p1), cast(Renderer_Vec4f, m->c1), cast(Renderer_Vec2f, m->uv1),
		    cast(Renderer_Vec2f, m->p2), cast(Renderer_Vec4f, m->c2), cast(Renderer_Vec2f, m->uv2),
		    cast(Renderer_Vec2f, m->p3), cast(Renderer_Vec4f, m->c3), cast(Renderer_Vec2f, m->uv3));
}

s32 chess_piece_to_x[] = {
  [CHESS_KIND_NONE] = -1,
  [CHESS_KIND_PAWN] = 5,
  [CHESS_KIND_KNIGHT] = 3,
  [CHESS_KIND_BISHOP] = 2,
  [CHESS_KIND_ROOK] = 4,
  [CHESS_KIND_QUEEN] = 1,
  [CHESS_KIND_KING] = 0,
};

int main() {

  Frame frame;
  if(frame_open(&frame, WIDTH, HEIGHT, 0) != FRAME_ERROR_NONE) {
    TODO();
  }

  Renderer renderer;
  if(!renderer_open(&renderer)) {
    TODO();
  }

  s32 pieces_width, pieces_height;
  u8 *pieces_data = stbi_load("pieces.png", &pieces_width, &pieces_height, 0, 4);
  if(!pieces_data) {
    TODO();
  }
  
  Renderer_Texture pieces_texture;
  if(!renderer_texture_create(&pieces_texture,
			      &renderer,
			      pieces_data,
			      (f32) pieces_width,
			      (f32) pieces_height,
			      0)) {
    TODO();
  }

  Mui mui = {0};
  mui.render = mui_render;
  mui.render_userdata = &renderer;

  Chess_Game game;
  chess_game_default(&game);

  s32 dragged_piece_index = -1;

  Frame_Event event;
  while(frame.running) {

    int clicked = 0;
    int released = 0;
    
    while(frame_peek(&frame, &event)) {

      switch(event.type) {
      case FRAME_EVENT_KEYPRESS: {
	if(event.as.key == 'Q') {
	  frame.running = 0;
	}
	if(event.as.key == 'R') {
	  chess_game_default(&game);
	}
	if(event.as.key == 'B') {
	  if(game.history_len > 0) {
	    chess_game_rewind(&game, game.history_len - 1);  
	  }
	  
	}
      } break;
      case FRAME_EVENT_MOUSEPRESS: {
	clicked = 1;
      } break;
      case FRAME_EVENT_MOUSERELEASE: {
	released = 1;
      } break;
	
      }
    }

    Mui_Vec2f cell_size = mui_vec2f(frame.width / CHESS_N, frame.height / CHESS_N);

    Mui_Vec2f size = mui_vec2f(frame.width, frame.height);

    if(released) {
      Mui_Vec2f mouse = mui_vec2f(frame.mouse_x, frame.mouse_y);
      
      if(dragged_piece_index >= 0 &&
	 mui_point_in_rect(mouse, mui_vec2f(0, 0), size)) {

	s32 x = (s32) (frame.mouse_x / cell_size.x);
	s32 y = (s32) (frame.mouse_y / cell_size.y);

	Chess_Move move = {
	  .from = dragged_piece_index,
	  .to   = x + (CHESS_N - y - 1)*CHESS_N,
	};

	chess_game_move(&game, &move);
        
      }
      dragged_piece_index = -1;
      
    }

    renderer_begin(&renderer, frame.width, frame.height);

    f32 piece_padding_w = 6.f / pieces_texture.width;
    f32 piece_padding_h = 10.f / pieces_texture.width;
    
    f32 piece_w = 128.f / pieces_texture.width;
    f32 piece_h = 128.f / pieces_texture.height;
    
    Mui_Vec2f piece_size = mui_vec2f(piece_w, piece_h);

    for(s32 y=0;y<CHESS_N;y++) {
      for(s32 x=0;x<CHESS_N;x++) {
	
	Mui_Vec2f cell_pos = mui_vec2f(x*cell_size.x, y*cell_size.y);

	Mui_Vec4f color;
	if((y + x) & 0x1) {
	  color = mui_vec4f(0.16470f, 0.1686f, 0.3137f, 1);
	} else {
	  color = mui_vec4f(0.7372f, 0.7372f, 0.7372f, 1);
	}

	mui_rect(&mui,
		 cell_pos,
		 cell_size,
		 color);

	s32 index = x + (CHESS_N - y - 1)*CHESS_N;

	Chess_Piece *p = &game.board[index];
	if(p->kind == CHESS_KIND_NONE) {
	  continue;
	}

	if(dragged_piece_index == index) {
	  continue;
	}

	if(clicked && mui_point_in_rect(mui_vec2f(frame.mouse_x, frame.mouse_y),
					cell_pos,
					cell_size)) {
	  dragged_piece_index = index;
	}
	  
	mui_texture(&mui,
		    &pieces_texture,
		    cell_pos,
		    cell_size,
		    mui_vec2f(chess_piece_to_x[p->kind]*(piece_w+piece_padding_w),
			      (1 - p->black)*(piece_h+piece_padding_h)),
		    piece_size,
		    mui_vec4f(1, 1, 1, 1));
	
      }            
    }

    if(dragged_piece_index >= 0) {
      Chess_Piece *p = &game.board[dragged_piece_index];
      
      mui_texture(&mui,
		  &pieces_texture,
		  mui_vec2f(frame.mouse_x - cell_size.x/2, frame.mouse_y - cell_size.y/2),
		  cell_size,
		  mui_vec2f(chess_piece_to_x[p->kind]*(piece_w+piece_padding_w),
			    (1 - p->black)*(piece_h+piece_padding_h)),
		  piece_size,
		  mui_vec4f(1, 1, 1, 1));      
    }

    
    renderer_end(&renderer);

    frame_swap_buffers(&frame);
  }

  frame_close(&frame);

  return 0;
}
