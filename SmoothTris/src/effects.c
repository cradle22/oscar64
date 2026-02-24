#include "effects.h"
#include <stdlib.h> 

static inline byte raster_read(void) {
    return *(volatile unsigned char*)0xD012;
}

static void wait_vblank(void)
{
    while (raster_read() < 250) { }   // wait until we enter lower border/vblank-ish
    while (raster_read() >= 250) { }  // wait until we wrap back to top
}

void fadeLine(char line) {
  __assume(line < 20);
  char startY = BOWLSTARTY / BLOCKSIZE;
  char startX = BOWLSTARTX / BLOCKSIZE;
  char ec[5] = {singleBlockCharacter, 40, 41, 42, 32 };

  bool stop = false;
  for(char x = 0; x < 5; x++) {
    for(char e = 0; e < 5; e++) {
      char delay = ((e & 1) == 0) ? EFFECT_DELAY_BASE : (EFFECT_DELAY_BASE - EFFECT_DELAY_SKIP); // alternate: 2, 1, 2, 1, 2 frames
      for(char w = 0; w < delay; w++) {
        wait_vblank();
      }
      
      Screen[40 * (startY + line) + startX + x] = ec[e];
      Screen[40 * (startY + line) + startX + 9 - x] = ec[e];
      if(e == 4) {
        Screen[40 * (startY + line) + startX + x + 1] = ec[e];
      }
    }
    if(TheGame.state == GS_EFFECT) {
      game_keyboard();
    }
    if(TheGame.state != GS_EFFECT) {
      stop = true;
    }
    if(stop) break;
  }
}

typedef struct {
  bool first = true;
  int x_fixed;
  int y_fixed;
  int dx;
  int dy;
  char oldChar;
  char oldColor;
  char color = 0;
  int last_ptr;
} ex_data;

void explodeLineSpr(char line) {
  __assume(line < 20);
  int startY = BOWLSTARTY / BLOCKSIZE;
  int startX = BOWLSTARTX / BLOCKSIZE;
  ex_data effectData[10];

  // Random permutation of tile indices for vertical direction assignment
  char perm[10];
  for (char i = 0; i < 10; i++) perm[i] = i;
  for (char i = 9; i > 0; i--) {
    char j = rand() % (i + 1);
    char tmp = perm[i]; 
    perm[i] = perm[j]; 
    perm[j] = tmp;
  }

  // Deterministic horizontal fan spread: 8 fixed positions (spaced 12 apart) + 2 alternating
  static const int dx_fan[8] = {-55, -43, -31, -19, 19, 31, 43, 55};

  // calculate sprite starting positions and velocity
  for(char x = 0; x < 10; x++) {
    effectData[x].first = true;
    effectData[x].color = Color[40 * (startY + line) + startX + x];
    effectData[x].x_fixed = (BOWLSTARTX + (x * BLOCKSIZE)) << FBITS;
    effectData[x].y_fixed = (BOWLSTARTY + (line * BLOCKSIZE)) << FBITS;
    effectData[x].oldChar = 0;
    effectData[x].last_ptr = 40 * (startY + line) + startX + x;
    effectData[x].dx = (x < 8) ? dx_fan[x] : ((rand() & 1) ? 8 : -8);
    vspr_set(x, 23 + (effectData[x].x_fixed >> FBITS),
      49 + (effectData[x].y_fixed >> FBITS), 
      (unsigned)Sprite / 64, effectData[x].color);
    /*
    vspr_set(x + 1, 24 + (effectData[x].x_fixed >> FBITS),
      50 + (effectData[x].y_fixed >> FBITS), 
      (unsigned)Sprite / 64, effectData[x].color);
    */
  }

  // Assign vertical velocities: first 5 in permutation go up (-35 to -55), last 5 go down (+35 to +55)
  for (char i = 0; i < 5; i++)
    effectData[perm[i]].dy = -(35 + rand() % 21);
  for (char i = 5; i < 10; i++)
    effectData[perm[i]].dy = 35 + rand() % 21;

  // print to screen - will cause flickering on the same line
  vspr_sort();
	vspr_update();
  
  for(;;) {
    char active_particles = 0;
    for(char x = 0; x < 10; x++) {
      if(! effectData[x].first && effectData[x].last_ptr >= 0) { 
        effectData[x].last_ptr = -1;
      }

      // clear the single tetris block from the grid on first move
      if(effectData[x].first) {
        effectData[x].first = false;
        Screen[effectData[x].last_ptr] = 0x20;
      }

      // move particles
      effectData[x].x_fixed += effectData[x].dx;
      effectData[x].y_fixed += effectData[x].dy;

      // Calculate integer coordinates
      int cx = effectData[x].x_fixed >> FBITS;
      int cy = effectData[x].y_fixed >> FBITS;

      // Draw ONLY if in bounds
      if(cx >= 0 && cy >= -7 && cx < 351 && cy < 257) {
        int ptr = (cy * 40) + cx;
        effectData[x].last_ptr = ptr;
        vspr_move(x, 23 + cx, 49 + cy);
        //vspr_move(x + 1, 24 + cx, 50 + cy);
        vspr_sort();
	      vspr_update();
        active_particles++;
      } else {
        vspr_hide(x);
        //vspr_hide(x + 1);
      }
    }
    if(active_particles == 0) break;
    game_keyboard();
    if(TheGame.state != GS_EFFECT) {
      break;
    }
  }
  putTile();
}

void explodeLine(char line) {
  __assume(line < 20);
  int startY = BOWLSTARTY / BLOCKSIZE;
  int startX = BOWLSTARTX / BLOCKSIZE;
  char screen_backup[1000];
  char color_backup[1000];
  ex_data effectData[10];
  memcpy(screen_backup, Screen, 1000);
  memcpy(color_backup, Color, 1000);

  // set initial settings for each particle and backup the line's characters/colors, then clear the line
  for(int x = 0; x < 10; x++) {
    effectData[x].first = true;
    effectData[x].color = Color[40 * (startY + line) + startX + x];
    effectData[x].x_fixed = (startX + x) << FBITS;
    effectData[x].y_fixed = (startY + line) << FBITS;
    effectData[x].oldChar = 0;
    effectData[x].last_ptr = 40 * (startY + line) + startX + x;
    int magX = 8 + (rand() % 17);
    effectData[x].dx = (rand() & 1) ? magX : -magX;
    int magY = 8 + (rand() % 17);
    if(startY + line > 14) {
      effectData[x].dy = -magY;
    } else {
      effectData[x].dy = (rand() & 1) ? magY : -magY;
    }
    screen_backup[40 * (startY + line) + startX + x] = 0x20;
  }
  
  for(;;) {
    char active_particles = 0;
    for(char x = 0; x < 10; x++) {

      // restore previous character if it exists
      if(! effectData[x].first && effectData[x].last_ptr >= 0) { 
        Screen[effectData[x].last_ptr] = effectData[x].oldChar;
        Color[effectData[x].last_ptr] = effectData[x].oldColor;
        effectData[x].last_ptr = -1;
      }

      // clear the single tetris block from the grid on first move
      if(effectData[x].first) {
        effectData[x].first = false;
        Screen[effectData[x].last_ptr] = 0x20;
        //redrawGrid();
      }
      // move particles
      effectData[x].x_fixed += effectData[x].dx;
      effectData[x].y_fixed += effectData[x].dy;

      // Calculate integer coordinates
      int cx = effectData[x].x_fixed >> FBITS;
      int cy = effectData[x].y_fixed >> FBITS;

      // Draw ONLY if in bounds
      if(cx >= 0 && cy >= 0 && cx < 40 && cy < 25) {
        int ptr = (cy * 40) + cx;
        effectData[x].oldChar = screen_backup[ptr];
        effectData[x].oldColor = color_backup[ptr];
        effectData[x].last_ptr = ptr;
        Screen[ptr] = singleBlockCharacter;
        Color[ptr] = effectData[x].color;
        active_particles++;
      }
    }
    if(active_particles == 0) break;
    game_keyboard();
    if(TheGame.state != GS_EFFECT) {
      break;
    }
    for(char w = 0; w < EFFECT_DELAY; w++) {
      wait_vblank();
    }
  }
  
  // Final cleanup (restore the screen for the last shown particles)
  for(char x = 0; x < 10; x++) {
    if(effectData[x].last_ptr >= 0) {
      Screen[effectData[x].last_ptr] = effectData[x].oldChar;
      Color[effectData[x].last_ptr] = effectData[x].oldColor;
    }
  }
}

void cascadeLineSpr(char line) {
  __assume(line < 20);
  int startY = BOWLSTARTY / BLOCKSIZE;
  int startX = BOWLSTARTX / BLOCKSIZE;
  ex_data effectData[10];

  for(int x = 0; x < 10; x++) {
    effectData[x].first = true;
    effectData[x].color = Color[40 * (startY + line) + startX + x];
    effectData[x].x_fixed = (BOWLSTARTX + (x * BLOCKSIZE)) << FBITS;
    effectData[x].y_fixed = (BOWLSTARTY + (line * BLOCKSIZE)) << FBITS;
    effectData[x].oldChar = 0;
    effectData[x].last_ptr = 40 * (startY + line) + startX + x;
    effectData[x].dx = 0;
    // outer tiles fall fastest, mid-outer medium, center slowest
    if(x == 0 || x == 1 || x == 8 || x == 9)
      effectData[x].dy = 60 + rand() % 21;
    else if(x == 2 || x == 3 || x == 6 || x == 7)
      effectData[x].dy = 40 + rand() % 16;
    else
      effectData[x].dy = 20 + rand() % 11;
    vspr_set(x, 23 + (effectData[x].x_fixed >> FBITS),
      49 + (effectData[x].y_fixed >> FBITS),
      (unsigned)Sprite / 64, effectData[x].color);
  }

  vspr_sort();
  vspr_update();

  for(;;) {
    char active_particles = 0;
    for(char x = 0; x < 10; x++) {
      if(! effectData[x].first && effectData[x].last_ptr >= 0) {
        effectData[x].last_ptr = -1;
      }
      if(effectData[x].first) {
        effectData[x].first = false;
        Screen[effectData[x].last_ptr] = 0x20;
      }
      effectData[x].x_fixed += effectData[x].dx;
      effectData[x].y_fixed += effectData[x].dy;
      int cx = effectData[x].x_fixed >> FBITS;
      int cy = effectData[x].y_fixed >> FBITS;
      if(cx >= 0 && cy >= -7 && cx < 351 && cy < 257) {
        int ptr = (cy * 40) + cx;
        effectData[x].last_ptr = ptr;
        vspr_move(x, 23 + cx, 49 + cy);
        vspr_sort();
        vspr_update();
        active_particles++;
      } else {
        vspr_hide(x);
      }
    }
    if(active_particles == 0) break;
    game_keyboard();
    if(TheGame.state != GS_EFFECT) {
      break;
    }
  }
  putTile();
}

void spiralLineSpr(char line) {
  __assume(line < 20);
  int startY = BOWLSTARTY / BLOCKSIZE;
  int startX = BOWLSTARTX / BLOCKSIZE;
  ex_data effectData[10];

  // Pre-calculated velocity vectors for 10 evenly-spaced angles on a circle (radius ~50)
  static const int spiral_dx[10] = { 50,  40,  15, -15, -40, -50, -40, -15,  15,  40};
  static const int spiral_dy[10] = {  0,  29,  47,  47,  29,   0, -29, -47, -47, -29};

  for(int x = 0; x < 10; x++) {
    effectData[x].first = true;
    effectData[x].color = Color[40 * (startY + line) + startX + x];
    effectData[x].x_fixed = (BOWLSTARTX + (x * BLOCKSIZE)) << FBITS;
    effectData[x].y_fixed = (BOWLSTARTY + (line * BLOCKSIZE)) << FBITS;
    effectData[x].oldChar = 0;
    effectData[x].last_ptr = 40 * (startY + line) + startX + x;
    effectData[x].dx = spiral_dx[x] + (rand() % 11) - 5;
    effectData[x].dy = spiral_dy[x] + (rand() % 11) - 5;
    vspr_set(x, 23 + (effectData[x].x_fixed >> FBITS),
      49 + (effectData[x].y_fixed >> FBITS),
      (unsigned)Sprite / 64, effectData[x].color);
  }

  vspr_sort();
  vspr_update();

  for(;;) {
    char active_particles = 0;
    for(char x = 0; x < 10; x++) {
      if(! effectData[x].first && effectData[x].last_ptr >= 0) {
        effectData[x].last_ptr = -1;
      }
      if(effectData[x].first) {
        effectData[x].first = false;
        Screen[effectData[x].last_ptr] = 0x20;
      }
      effectData[x].x_fixed += effectData[x].dx;
      effectData[x].y_fixed += effectData[x].dy;
      int cx = effectData[x].x_fixed >> FBITS;
      int cy = effectData[x].y_fixed >> FBITS;
      if(cx >= 0 && cy >= -7 && cx < 351 && cy < 257) {
        int ptr = (cy * 40) + cx;
        effectData[x].last_ptr = ptr;
        vspr_move(x, 23 + cx, 49 + cy);
        vspr_sort();
        vspr_update();
        active_particles++;
      } else {
        vspr_hide(x);
      }
    }
    if(active_particles == 0) break;
    game_keyboard();
    if(TheGame.state != GS_EFFECT) {
      break;
    }
  }
  putTile();
}

// Smooth pixel-by-pixel scroll: shifts the bowl content above `line` down
// one pixel per frame over 7 frames, bridging the visual gap left after a
// line-removal effect.  Must be called before the logical grid is updated
// (while MMAP_NO_BASIC is active — no charset writes needed here).
//
// Scroll character layout (pre-generated by generate_scroll_chars()):
//   220       : solid  (above block + current block)
//   221-227   : top-heavy offset N  (above block, no current block)
//   228-234   : bottom-heavy offset N  (no above block, current block)
//   235       : empty  (no above block, no current block)
void scroll_line_down(char line) {
  __assume(line < 20);
  char startY = BOWLSTARTY / BLOCKSIZE;
  char startX = BOWLSTARTX / BLOCKSIZE;

  for(char offset = 1; offset <= 7; offset++) {
    wait_vblank();

    for(signed char y = line; y >= 0; y--) {
      for(char x = 0; x < 10; x++) {
        bool above   = (y > 0) && (TheGame.grid[x][y - 1] != 0);
        bool current = (y < line) && (TheGame.grid[x][y] != 0);

        char ch;
        char col;
        if(above && current) {
          ch  = 220;
          col = tiles[TheGame.grid[x][y] - 1].color;
        } else if(above) {
          ch  = 220 + offset;
          col = tiles[TheGame.grid[x][y - 1] - 1].color;
        } else if(current) {
          ch  = 227 + offset;
          col = tiles[TheGame.grid[x][y] - 1].color;
        } else {
          ch  = 235;
          col = VCOL_LT_BLUE;
        }

        int screenPos = 40 * (startY + y) + startX + x;
        Screen[screenPos] = ch;
        Color[screenPos]  = col;
      }
    }

    game_keyboard();
    if(TheGame.state != GS_EFFECT) break;
  }

  // Clear the top row of the bowl (now visually vacated)
  for(char x = 0; x < 10; x++) {
    int screenPos = 40 * startY + startX + x;
    Screen[screenPos] = 0x20;
    Color[screenPos]  = VCOL_LT_BLUE;
  }
}

void removeLine(char line) {
  __assume(line < 20);
  GameState oldState = TheGame.state;
  game_state(GS_EFFECT);
  char randEffect = rand() % 4;
  switch(randEffect) {
    case 0:
      VIC_REG->spr_enable = 0x00;
      fadeLine(line);
      VIC_REG->spr_enable = 0xFF;
      break;
    case 1:
      #if EXPLODELINEALGO == 0
        VIC_REG->spr_enable = 0x00;
        explodeLine(line);
        VIC_REG->spr_enable = 0xFF;
      #else
        explodeLineSpr(line);
      #endif
      break;
    case 2:
      cascadeLineSpr(line);
      break;
    case 3:
      spiralLineSpr(line);
      break;
  }
  if(TheGame.state == GS_EFFECT) {
    scroll_line_down(line);
  }
  if(TheGame.state != GS_EXIT && TheGame.state != GS_PANIC) {
    game_state(oldState);
  }
}