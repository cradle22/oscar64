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
      for(char w = 0; w < EFFECT_DELAY; w++) {
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

void explodeLine(char line) {
  __assume(line < 20);
  int startY = BOWLSTARTY / BLOCKSIZE;
  int startX = BOWLSTARTX / BLOCKSIZE;
  char screen_backup[1000];
  char color_backup[1000];
  ex_data effectData[10];
  memcpy(screen_backup, Screen, 1000);
  memcpy(color_backup, Color, 1000);
  for(int x = 0; x < 10; x++) {
    effectData[x].first = true;
    effectData[x].color = Color[40 * (startY + line) + startX + x];
    effectData[x].x_fixed = (startX + x) << FBITS;
    effectData[x].y_fixed = (startY + line) << FBITS;
    effectData[x].oldChar = 0;
    effectData[x].last_ptr = 40 * (startY + line) + startX + x;  // Change: use -1 as "invalid" marker
    int magX = 8 + (rand() % 17);
    effectData[x].dx = (rand() & 1) ? magX : -magX;
    int magY = 8 + (rand() % 17);
    if(startY + line > 14) {
      effectData[x].dy = -magY;
    } else {
      effectData[x].dy = (rand() & 1) ? magY : -magY;
    }
    //Screen[40 * (startY + line) + startX + x] = 0x20;
    screen_backup[40 * (startY + line) + startX + x] = 0x20;
  }
  
  for(;;) {
    char active_particles = 0;
    for(char x = 0; x < 10; x++) {
      if(! effectData[x].first && effectData[x].last_ptr >= 0) { 
        Screen[effectData[x].last_ptr] = effectData[x].oldChar;
        Color[effectData[x].last_ptr] = effectData[x].oldColor;
        effectData[x].last_ptr = -1;
      }
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
  
  // Final cleanup
  for(char x = 0; x < 10; x++) {
    if(effectData[x].last_ptr >= 0) {  // Change: check >= 0
      Screen[effectData[x].last_ptr] = effectData[x].oldChar;
      Color[effectData[x].last_ptr] = effectData[x].oldColor;
    }
  }
}

void removeLine(char line) {
  __assume(line < 20);
  GameState oldState = TheGame.state;
  game_state(GS_EFFECT);
  char randEffect = rand() % 2;
  switch(randEffect) {
    case 0:
      fadeLine(line);
      break;
    case 1:
      explodeLine(line);
      break;
  }
  if(TheGame.state != GS_EXIT && TheGame.state != GS_PANIC) {
    game_state(oldState);
  }
}