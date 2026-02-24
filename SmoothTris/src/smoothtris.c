#include <c64/vic.h>
#include <c64/sprites.h>
#include <c64/keyboard.h>
#include <c64/memmap.h>
#include <c64/rasterirq.h>
#include <c64/sid.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gamevars.h"
#include "effects.h"
#include "srs.h"
#include "shadow.h"
//#include "tetromino.h"

#pragma section( music, 0)
#pragma region( music, 0xa000, 0xc000, , , {music} )
#pragma data(music)
__export const char music[] = {
	#embed 0x2000 0x7c "../resources/CommandoTetrisV4.sid" 
};
#pragma data(data)

const char EffectChars[] = {
      0,  60, 126, 126, 126, 126,  60,   0, // Index 40
      0,  60, 126, 102, 126, 126,  60,   0, // Index 41
      0,   0,  24,  60,  24,   0,   0,   0, // Index 42
    255, 129, 129, 129, 129, 129, 129, 255  // Index 43
};

const char EffectCharsMulti[] = {
  0xaa, 0xaa, 0xbd, 0xbd, 0xbd, 0xbd, 0x55, 0x55, 0x00, 0x3c, 0xff, 0xff,
  0xff, 0xff, 0x3c, 0x00, 0x00, 0x3c, 0xc3, 0xc3, 0xc3, 0xc3, 0x3c, 0x00,
  0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x00, 0x00
};

bool	music_off;
void music_init(char tune){
	__asm
	{
		lda		tune
		jsr		$a000
	}
}

void music_play(void) {
	__asm
	{
		jsr		$a003
	}
}

void music_silence(void)
{
	sid.voices[0].ctrl = 0;
	sid.voices[0].susrel = 0;
	sid.voices[1].ctrl = 0;
	sid.voices[1].susrel = 0;
	sid.voices[2].ctrl = 0;
	sid.voices[2].susrel = 0;
}

void music_toggle(void)
{
	if (music_off)
		music_off = false;
	else
	{
		music_off = true;
		music_silence();
	}
}

__interrupt void music_irq(void) {
	vic.color_border++;
  if(TheGame.state == GS_PLAYING) {
    music_play();
  }
	vic.color_border--;
}

RIRQCode	music_rirq;


bool collision(char row, signed char line, char rot) {
  __assume(row < 20);
  __assume(rot < 5);
  char dimx = TheGame.currentTile.dimx[rot];
  char dimy = TheGame.currentTile.dimy[rot];

  // horizontal bounds
  if (row < 0) return true;
  if (row + dimx - 1 > 9) return true;

  // bottom bound
  if (line + dimy - 1 > 19) return true;

  // if piece is above the visible grid, just skip those rows in the test
  char ystart = 0;
  if (line < 0) {
    ystart = (char)(-line);
    if (ystart >= dimy) return false; // whole piece above grid => no collision with fixed tiles
  }

  for (char x = 0; x < dimx; x++) {
    for (char y = ystart; y < dimy; y++) {
      if (TheGame.currentTile.data[TheGame.currentTile.accessNodes[rot] + dimx * y + x] == 1) {
        if (TheGame.grid[row + x][line + y] != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

void fixSpriteOffsets(char rot) {
  __assume(rot < 5);
  char sp_count = 0;
  for(char y = 0; y < TheGame.currentTile.dimy[rot]; y++) {
    for(char x = 0; x < TheGame.currentTile.dimx[rot]; x++) {
      if(TheGame.currentTile.data[TheGame.currentTile.accessNodes[rot] + y * TheGame.currentTile.dimx[rot] + x] == 1) {
        TheGame.currentTile.x_spr_offsets[sp_count] = x;
        TheGame.currentTile.y_spr_offsets[sp_count] = y;
        sp_count++;
      }
    }
  }
}

void status_init(void) {
	for(char i=0; i<40; i++)
		Screen[i] = StatusText[i];
}

void int_to_padded_str(unsigned int value, char* buffer, unsigned char length) {
  buffer[length] = '\0';
  for (signed char i = length - 1; i >= 0; i--) {
    if (value > 0 || i == length - 1) { 
      buffer[i] = (value % 10) + '0';
      value /= 10;
    } else {
      buffer[i] = '0';
    }
  }
}

void int_to_padded_str2(unsigned int value, char* buffer, unsigned char length, unsigned char start) {
  buffer[length * 2 - 1] = '\0';
  for (signed char i = length - 1; i >= 0; i--) {
    if (value > 0 || i == length - 1) { 
      buffer[i * 2] = (value % 10) + start;
      buffer[i * 2 + 1] = (value % 10) + start + 1;
      value /= 10;
    } else {
      buffer[i] = start;
      buffer[i] = start;
    }
  }
}

void statusUpdate(void) {
  char num_buf[7];
  int_to_padded_str(TheGame.score, num_buf, 6);
  char* screen = Screen + 8;
  for(unsigned char i = 0; i < 6; i++) {
    screen[i] = num_buf[i]; 
  }
  int_to_padded_str(TheGame.lines, num_buf, 6);
  screen = Screen + 23;
  for(unsigned char i = 0; i < 6; i++) {
    screen[i] = num_buf[i]; 
  }
  int_to_padded_str(TheGame.level + 1, num_buf, 2);
  screen = Screen + 38;
  for(unsigned char i = 0; i < 2; i++) {
    screen[i] = num_buf[i]; 
  }
}

void statusUpdate2(void) {
  char num_buf[13];
  int_to_padded_str2(TheGame.score, num_buf, 6, 54);
  char* screen = Screen + 12;
  for(unsigned char i = 0; i < 12; i++) {
    screen[i] = num_buf[i]; 
  }
  int_to_padded_str2(TheGame.level + 1, num_buf, 2, 54);
  screen = Screen + 36;
  for(unsigned char i = 0; i < 4; i++) {
    screen[i] = num_buf[i]; 
  }
  int_to_padded_str2(TheGame.lines, num_buf, 6, 54);
  screen = Screen + 40 + 12;
  for(unsigned char i = 0; i < 12; i++) {
    screen[i] = num_buf[i]; 
  }
}

void showHideNextTile(bool hide) {

  // clear the preview character area
  #pragma unroll(full)
  for(char y = 0; y < 2; y++) {
    #pragma unroll(full)
    for(char x = 0; x < 4; x++) {
      Screen[(40 * (PREVIEWY)) + (40 * y) + (PREVIEWX + 1 + x)] = 0x20;
    }
  }
  if(! hide) {

    // draw the shape of the next tile
    char c = 0;
    for(char y = 0; y < TheGame.nextTile.dimy[0]; y++) {
      for(char x = 0; x < TheGame.nextTile.dimx[0]; x++) {
        if(TheGame.nextTile.data[c] == 1) {
          Screen[(40 * (PREVIEWY))+ (40 * y) + (PREVIEWX + 1 + x)] = singleBlockCharacter;
          Color[(40 * (PREVIEWY))+ (40 * y) + (PREVIEWX + 1 + x)] = TheGame.nextTile.color;
        }
        c++;
      }
    }
  }
}

void putTile(void) {
  /*
  spr_set(0, true, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(1, true, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(2, true, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(3, true, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(4, true, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(5, true, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(6, true, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  spr_set(7, true, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color, false, false, false);
  */
  vspr_set(0, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(1, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(2, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(3, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(4, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(5, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(6, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color);
  vspr_set(7, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS), 
    (unsigned)Sprite / 64, TheGame.currentTile.color); 
  vspr_sort();
	vspr_update();
}

void moveTileToPos(bool setColor) {
  /*
  if(setColor) {
    spr_color(0, TheGame.currentTile.color);
    spr_color(1, TheGame.currentTile.color);
    spr_color(2, TheGame.currentTile.color);
    spr_color(3, TheGame.currentTile.color);
    spr_color(4, TheGame.currentTile.color);
    spr_color(5, TheGame.currentTile.color);
    spr_color(6, TheGame.currentTile.color);
    spr_color(7, TheGame.currentTile.color);
  }
  spr_move(0, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(1, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(2, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(3, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(4, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(5, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(6, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS));
  spr_move(7, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS));
  */
 if(setColor) {
    vspr_color(0, TheGame.currentTile.color);
    vspr_color(1, TheGame.currentTile.color);
    vspr_color(2, TheGame.currentTile.color);
    vspr_color(3, TheGame.currentTile.color);
    vspr_color(4, TheGame.currentTile.color);
    vspr_color(5, TheGame.currentTile.color);
    vspr_color(6, TheGame.currentTile.color);
    vspr_color(7, TheGame.currentTile.color);
  }
  vspr_move(0, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(1, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(2, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(3, 23 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    49 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(4, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[0] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[0] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(5, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[1] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[1] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(6, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[2] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[2] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_move(7, 24 + BOWLSTARTX + (TheGame.currentTile.x_spr_offsets[3] * 8) + (TheGame.tile_row_graph >> FBITS), 
    50 + BOWLSTARTY + (TheGame.currentTile.y_spr_offsets[3] * 8) + (TheGame.tile_line_graph >> FBITS));
  vspr_sort();
	vspr_update();
}

bool rotate(bool left) {
	int tempRot = TheGame.rotation;
  
  // switch to SRS / wall kick rotation style
  if(try_rotate(&tempRot, TheGame.tileno, &TheGame.tile_row, &TheGame.tile_line, left ? -1 : 1)) {
    TheGame.rotation = tempRot;
    fixSpriteOffsets(TheGame.rotation);
    drawShadow();
    return true;
  }
  return false;
  /*
  if(left) {
    if(tempRot == 3) tempRot = 0; else tempRot++;
  } else {
    if(tempRot == 0) tempRot = 3; else tempRot--;
  }

	// check if the tile would fit the grid 
	if(collision(TheGame.tile_row, TheGame.tile_line, tempRot)) return(false);

	// no errors, erase old position and put it on screen with new rotation
	TheGame.rotation = tempRot;
  fixSpriteOffsets(TheGame.rotation);
	return(true);
  */
}

char getNextTile(void) {
  char nextTile = 7;
  if(TheGame.bagpos == 7 || TheGame.state == GS_READY) {
    TheGame.bagpos = 0;
    
    // if the cheatmode is activated, always produce the long line tile
    if(TheGame.cheat == 3) {
      for(char pos = 0; pos < 7; pos++) {
        TheGame.bag[pos] = 0;
      }
    } else {
      for(char pos = 0; pos < 7; pos++) {
        TheGame.bag[pos] = 7;
      }
      char i = 0;
      char newTile;
      while(i != 7) {
        bool v = true;
        newTile = rand() % 7;
        for(char pos = 0; pos < 7; pos++) {
          if(TheGame.bag[pos] == newTile) {
            v = false;
            break;
          }
        }
        if(v) {
          TheGame.bag[i] = newTile;
          i++;
        }
      }
    }
  }
  nextTile = TheGame.bag[TheGame.bagpos];
  TheGame.bagpos++;
  return nextTile;
}

void newtile(void) {
	/* check if there is a nextTile defined, if not, create one */
	if(TheGame.state == GS_READY) {
		TheGame.nexttileno = getNextTile();
		TheGame.nextTile = tiles[TheGame.nexttileno];
	}

	// copy nextTile to current tile and reset values
	TheGame.currentTile = TheGame.nextTile;
	TheGame.tileno = TheGame.nexttileno;
	TheGame.rotation = 0;
	TheGame.tile_row = (TheGame.currentTile.dimx[TheGame.rotation] == 2) ? 4 : 3;
	TheGame.tile_row_graph = (TheGame.tile_row * BLOCKSIZE) << FBITS;
	TheGame.tile_line = 0;
  TheGame.tile_line_graph = (TheGame.tile_line * BLOCKSIZE) << FBITS;
  fixSpriteOffsets(TheGame.rotation);

	// check if tiles have reached top 
	// NOTE: This is the ONLY place where the toplimit is checked!!
	if(collision(TheGame.tile_row, TheGame.tile_line, TheGame.rotation)) {
    game_state(GS_GAME_OVER);
    return;
  }
  
  if(TheGame.state == GS_EXIT || TheGame.state == GS_GAME_OVER || TheGame.state == GS_PANIC) {
    return;
  }
  
  // create or reposition the sprites
  if(TheGame.state == GS_READY) {
    putTile();
  } else {
    moveTileToPos(true);
  }
  //drawShadow();

	// generate a new nextTile
	TheGame.nexttileno = getNextTile();
	TheGame.nextTile = tiles[TheGame.nexttileno];
  showHideNextTile(false);
	return;
}

void redrawGrid(void) {
  char startX = BOWLSTARTX / BLOCKSIZE;
  char startY = BOWLSTARTY / BLOCKSIZE;
  #pragma unroll(full)
  for(char y = 0; y < 20; y++) {
    #pragma unroll(full)
    for(char x = 0; x < 10; x++) {
      int screenPos = 40 * (startY + y) + startX + x;
      if(TheGame.grid[x][y] > 0) {
        Screen[screenPos] = singleBlockCharacter;
        Color[screenPos] = tiles[TheGame.grid[x][y] - 1].color;
      } else {
        Screen[screenPos] = 32;
        Color[screenPos] = VCOL_LT_BLUE;
      }
    }
  }
}

void shift_chars_down(char x1, char y1, char x2, char y2) {
  for(char y = y2; y > y1; y--) {
    for(char x = x1; x <= x2; x++) {
      unsigned offset = y * 40 + x;
      unsigned offset_prev = (y - 1) * 40 + x;
        Screen[offset] = Screen[offset_prev];
        Color[offset] = Color[offset_prev];
    }
  }
  // Clear top row
  for(char x = x1; x <= x2; x++) {
    unsigned offset = y1 * 40 + x;
    Screen[offset] = 0x20;
    Color[offset] = 1;
  }
}

char checklines(void) {
  char count = 0;
  char comp_lines[20];
  char myLines = 0;
  #pragma unroll(full)
	for(char y = 0; y < 20; y++) {
		count = 0;
    #pragma unroll(full)
		for(char x = 0; x < 10; x++) {
			if(TheGame.grid[x][y] != 0) count++;
    }
		if(count == 10) {
			comp_lines[myLines] = y;
			myLines++;
		}
	}
  if(myLines > 0) {
    for(char x = 0; x < myLines; x++) {
      char where = comp_lines[x];

      // only show effects if the game continues normally
      // in theory, while running an effect the game could have been paused or exited
      if(TheGame.state != GS_EXIT && TheGame.state != GS_PANIC) {
        removeLine(where);
      }
      if(where > 0) {
        for(signed char y1 = (where - 1); y1 >= 0; y1--) {
          for(char x2 = 0; x2 < 10; x2++) {
            TheGame.grid[x2][y1 + 1] = TheGame.grid[x2][y1];
          }
        }
      }
      redrawGrid();
      TheGame.number_of_blocks -= 10;
      TheGame.lines++;
    }
    if(TheGame.lines >= 10 * (TheGame.level + 1)) {
      TheGame.level++;
      if(TheGame.level >= 10) {
        TheGame.state = GS_WON;
      }
    }
  }
  return myLines;
}

void moveToNewPos(void) {
  bool sideCollision = false;
  bool moveSide = false;
  char droppedCount = 0;
  bool softDrop = false;
  bool bottomCollision = collision(TheGame.tile_row, TheGame.tile_line + 1, TheGame.rotation);
  int pixY   = (TheGame.tile_line_graph >> FBITS);
  char fracY = (char)(pixY & (BLOCKSIZE - 1));  // 0..7 for BLOCKSIZE=8
  
  // handle sidemove
  if(TheGame.sidemove != 0) {
    char targetRow = TheGame.tile_row + (TheGame.sidemove > 0 ? 1 : -1);

    // collision if we moved sideways at the current grid line
    sideCollision = collision(targetRow, TheGame.tile_line, TheGame.rotation);

    if (sideCollision) {
      // would it fit one row lower (i.e. once we're deeper into the fall)?
      bool col_lower = collision(targetRow, TheGame.tile_line + 1, TheGame.rotation);

      // Only allow this "under an overhang" move in the last pixels
      if (!col_lower && fracY >= SIDE_ALLOW_FROM) {
        sideCollision = false; // allow late
      }
    }

    if (!sideCollision) {
      int newX = TheGame.tile_row_graph + (TheGame.sidemove << FBITS);

      if (TheGame.sidemove > 0) {
        if ((newX >> FBITS) < (TheGame.tile_row + 1) * BLOCKSIZE) {
          TheGame.tile_row_graph = newX;
        } else {
          TheGame.tile_row++;
          TheGame.tile_row_graph = (TheGame.tile_row * BLOCKSIZE) << FBITS;
          TheGame.sidemove = 0;
        }
      } else {
        if ((newX >> FBITS) > (TheGame.tile_row - 1) * BLOCKSIZE) {
          TheGame.tile_row_graph = newX;
        } else {
          TheGame.tile_row--;
          TheGame.tile_row_graph = (TheGame.tile_row * BLOCKSIZE) << FBITS;
          TheGame.sidemove = 0;
        }
      }
    } else {
      // blocked: cancel pending sidemove
      TheGame.sidemove = 0;
    }
  }
  
  if(bottomCollision && TheGame.sidemove != 0) {
    char targetRow = TheGame.tile_row + (TheGame.sidemove > 0 ? 1 : -1);

    // Can we safely be in the target column?
    // (both side-collision at current line, and bottom collision there)
    bool target_side_col = collision(targetRow, TheGame.tile_line, TheGame.rotation);
    bool target_bottom_col = collision(targetRow, TheGame.tile_line + 1, TheGame.rotation);

    // If the target column would NOT collide at the bottom, then we must NOT lock now.
    // Instead, commit the sideways move immediately and continue falling.
    if (!target_side_col && !target_bottom_col) {
      TheGame.tile_row = targetRow;
      TheGame.tile_row_graph = (TheGame.tile_row * BLOCKSIZE) << FBITS;
      TheGame.sidemove = 0;
      bottomCollision = false;   // IMPORTANT: cancel lock decision for this frame
    } else {
      // Can't escape collision by finishing the sidemove -> cancel it and lock at current column
      TheGame.sidemove = 0;
      TheGame.tile_row_graph = (TheGame.tile_row * BLOCKSIZE) << FBITS;
    }
  }
  if(TheGame.drop == 1) {
    char nl = getDropLine(TheGame.tile_row, TheGame.tile_line, TheGame.rotation);
    droppedCount = nl - TheGame.tile_line;
    TheGame.tile_line = nl;
    bottomCollision = true;
    TheGame.drop = 0;
  }
  if(bottomCollision) {
    removeShadow();

    // add the tile to the fixed tiles in the grid
    for(char x = 0; x < TheGame.currentTile.dimx[TheGame.rotation]; x++) {
      for(char y = 0; y < TheGame.currentTile.dimy[TheGame.rotation]; y++) {
        if(TheGame.currentTile.data[TheGame.currentTile.accessNodes[TheGame.rotation] + TheGame.currentTile.dimx[TheGame.rotation] * y + x] == 1) {
          TheGame.grid[TheGame.tile_row + x][TheGame.tile_line + y] = TheGame.currentTile.num + 1;
        }
      }
    }
    redrawGrid();
    if(TheGame.tileCount < 200) {
      TheGame.tileCount++;
    }

    // increase total number of blocks on grid
    TheGame.number_of_blocks += 4;

    // give a small reward to the score for putting a tile on screen
    //TheGame.inc_score = TheGame.inc_score + (int) (sqrt(20 - TheGame.tile_line) * sqrt(TheGame.level + 1));

    
    // disable sprites
    VIC_REG->spr_enable = 0x00;

    // check for completed lines
    char removed = checklines();
    TheGame.lines += removed;

    // redraw the grid with the now fixed sprite (as characters)
    redrawGrid();

    if(TheGame.accel > 0 && droppedCount == 0) {
      droppedCount = 1;
      softDrop = true;
    }
    if(TheGame.state == GS_EXIT || TheGame.state == GS_PANIC || TheGame.state == GS_WON) return;

    // calculate score
    if(TheGame.number_of_blocks == 0) {
      switch(removed) {
        case 1:
        TheGame.score += (TheGame.level + 1) * 800;
        break;
        case 2:
        TheGame.score += (TheGame.level + 1) * 1200;
        break;
        case 3:
        TheGame.score += (TheGame.level + 1) * 1800;
        break;
        case 4:
        TheGame.score += (TheGame.level + 1) * 3200;
        break;
      }
      //TheGame.score += (TheGame.level + 1) * 70;
    } else {
      switch(removed) {
        case 1:
        TheGame.score += (TheGame.level + 1) * 100;
        break;
        case 2:
        TheGame.score += (TheGame.level + 1) * 300;
        break;
        case 3:
        TheGame.score += (TheGame.level + 1) * 500;
        break;
        case 4:
        TheGame.score += (TheGame.level + 1) * 800;
        break;
      }
    }
    TheGame.score += (softDrop ? 1 : 2) * droppedCount;
    statusUpdate();

    // make new tile (if it fails, the top is reached = GAME OVER)
    vic_waitFrame();
    newtile();
    if(TheGame.state == GS_EXIT || TheGame.state == GS_PANIC || TheGame.state == GS_GAME_OVER || TheGame.state == GS_WON) return;
    
    // enable sprites again
    vic_waitFrame();
    VIC_REG->spr_enable = 0xFF;
  } else {

    // simple spritemove
    int newY = TheGame.tile_line_graph + (level_y_inc[TheGame.level] * TheGame.accel);

    if((newY >> FBITS) > (TheGame.tile_line + 1) * BLOCKSIZE) {
      newY = ((TheGame.tile_line + 1) * BLOCKSIZE << FBITS);
      TheGame.tile_line++;
    } 
    TheGame.tile_line_graph = newY;
    moveTileToPos(false);
    drawShadow();
  }
}


void game_keyboard(void)
{
  TheGame.accel = 1;
  unsigned char w_now;
  unsigned char space_now;
  unsigned char z_now;
  unsigned char p_now;

  // keys which continously can be pressed
	keyb_poll();
  w_now = key_pressed(KSCAN_W);
  space_now = key_pressed(KSCAN_SPACE);
  z_now = key_pressed(KSCAN_Z);
  p_now = key_pressed(KSCAN_P);
  switch(TheGame.state) {
    case GS_PLAYING:
      if(key_pressed(KSCAN_S)) {
        TheGame.accel = 10;
      }
      if(key_pressed(KSCAN_A)) {
        TheGame.sidemove = -2;
      }
      if(key_pressed(KSCAN_D)) {
        TheGame.sidemove = 2;
      }
      if (w_now && !prev_w)
        rotate(true);
      if (z_now && !prev_z)
        rotate(false);
      if(p_now && !prev_p)
        game_state(GS_PAUSED);
      if(space_now && ! prev_space)
        TheGame.drop = 1;
      break;
    case GS_PAUSED:
      if(p_now && !prev_p)
        game_state(GS_PLAYING);
      break;
    case GS_EFFECT:
      if(p_now && !prev_p)
        game_state(GS_PAUSED);
        break;
  }
  prev_w = w_now;
  prev_space = space_now;
  prev_z = z_now;
  prev_p = p_now;
}

void game_state(GameState state) {
  GameState oldState = TheGame.state;
  TheGame.state = state;
  switch(state) {
    case GS_READY:
      newtile();
      TheGame.state = GS_PLAYING;
      break;
    case GS_EFFECT:
      //VIC_REG->spr_enable = 0x00;
      hideShadow();
      break;
    case GS_PAUSED:
      VIC_REG->spr_enable = 0x00;
      showHideNextTile(true);
      hideShadow();
      break;
    case GS_PLAYING:
      if(oldState == GS_PAUSED) {
        VIC_REG->spr_enable = 0xFF;
        showHideNextTile(false);
        showShadow();
      } else if(oldState == GS_EFFECT) {
        //VIC_REG->spr_enable = 0xFF;
        showShadow();
      }
      break;
  }
}


void game_loop()
{
  switch (TheGame.state)
	{
    case GS_PLAYING:
      game_keyboard();
      moveToNewPos();
      //status_update();
      //move_tile();
      break;
    case GS_PAUSED:
      game_keyboard();
      break;
  }
}


char * const Tester = (char *) 0xd800;

int main(void)
{
  mmap_trampoline();
  vic_setbank(3);
  mmap_set(MMAP_CHAR_ROM);
  memcpy(Charset, Charset + 2048, 2048);
  memcpy(Charset + 40 * 8, EffectChars, sizeof(EffectChars));
  generate_scroll_chars();
  mmap_set(MMAP_RAM);
  memcpy(Sprite, SpriteImage, 64);
  mmap_set(MMAP_NO_BASIC);
  vic_setmode(VICM_TEXT, Screen, Charset);
  //spr_init(Screen);
  vspr_init(Screen);

  
  vic.color_border = VCOL_BLUE;
	vic.color_back = VCOL_LT_BLUE;
  memset(TheGame.grid, 0, sizeof(TheGame.grid));
  memset(Color, VCOL_BLACK, 25 * 40);
  textcolor(VCOL_BLACK);
  memset(Screen, 0x20, 25 * 40);
  char startX = (BOWLSTARTX / 8) - 1;
  char startY = BOWLSTARTY / 8;
  
  // draw the tetris bowl
  for(char y = 0; y < 20; y++) {
    Screen[40 * (startY + y) + startX] = BOWLCHARACTER;
    Screen[40 * (startY + y) + startX + 11] = BOWLCHARACTER;
    Color[40 * (startY + y) + startX] = 13;
    Color[40 * (startY + y) + startX + 11] = 13;
  }
  for(char x = 0; x < 12; x++) {
    Screen[40 * (startY + 20) + startX + x] = BOWLCHARACTER;
    Color[40 * (startY + 20) + startX + x] = 13;
  }
  
  // draw the lines and the preview field
  for(char x = 0; x < 40; x++) {
    Screen[80 + x] = 64;
    Color[80 + x] = VCOL_LT_GREY;
  }
  Screen[80 + PREVIEWX] = 114; Screen[80 + PREVIEWX + 5] = 114;
  Color[80 + PREVIEWX] = VCOL_LT_GREY; Color[80 + PREVIEWX + 5] = VCOL_LT_GREY;
  Screen[(40 * PREVIEWY) + (40 * 2) + PREVIEWX] = 109; Screen[(40 * PREVIEWY) + (40 * 2) + PREVIEWX + 5] = 125;
  Color[(40 * PREVIEWY) + (40 * 2) + PREVIEWX] = VCOL_LT_GREY; Color[(40 * PREVIEWY) + (40 * 2) + PREVIEWX + 5] = VCOL_LT_GREY;
  for(char y = 0; y < 2; y++) {
    Screen[(40 * PREVIEWY) + (40 * y) + PREVIEWX] = 93;
    Screen[(40 * PREVIEWY) + (40 * y) + (PREVIEWX + 5)] = 93;
    Color[(40 * PREVIEWY) + (40 * y) + PREVIEWX] = VCOL_LT_GREY;
    Color[(40 * PREVIEWY) + (40 * y) + (PREVIEWX + 5)] = VCOL_LT_GREY;
    if(y == 1) {
      for(char x = 0; x < 4; x++) {
        Screen[(40 * PREVIEWY) + (40 * y) + 40 + PREVIEWX + x + 1] = 64;
        Color[(40 * PREVIEWY) + (40 * y) + 40 + PREVIEWX + x + 1] = VCOL_LT_GREY;
      }
    }
  }

  //Screen[120] = 41;
  
  // draw status line
  status_init();
  
  // set game state
  TheGame.level = 0;
  TheGame.score = 0;
  TheGame.lines = 0;
  TheGame.sidemove = 0;
  TheGame.tileCount = 0;
  TheGame.cheat = 0;
  TheGame.drop = 0;
  game_state(GS_READY);
  
  for(;;)		
	{
		game_loop();
		vic_waitFrame();
    if(TheGame.state == GS_EXIT || TheGame.state == GS_PANIC) {
      Screen[480] = 5;
      break;
    }
	}
  vic_setmode(VICM_TEXT, (char *)0x0400, (char *)0x1000);
  mmap_set(MMAP_ROM);
  gotoxy(0, 15);
	return 0;
}


