#include "gamevars.h"
#include "shadow.h"

static char shadowRotation = 255;
signed char shadowStartX = -1;
signed char shadowStartY = -1;
Vector2 shadowData[6] = { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };

void removeShadow() {
  for (char i = 0; i < 6; i++) {
    if (shadowData[i].x >= 0 && shadowData[i].y >= 0) {
      Screen[40 * shadowData[i].y + shadowData[i].x] = CHAR_EMPTY;
      shadowData[i].x = -1;
      shadowData[i].y = -1;
    }
  }
}

void hideShadow() {
  for (char i = 0; i < 6; i++) {
    if (shadowData[i].x >= 0 && shadowData[i].y >= 0) {
      Screen[40 * shadowData[i].y + shadowData[i].x] = CHAR_EMPTY;
    }
  }
}

void showShadow() {
  for (char i = 0; i < 6; i++) {
    if (shadowData[i].x >= 0 && shadowData[i].y >= 0) {
      if(false && TheGame.currentTile.color == VCOL_ORANGE) {
        Screen[40 * shadowData[i].y + shadowData[i].x] = 133;
      } else {
        Screen[40 * shadowData[i].y + shadowData[i].x] = CHAR_SHADOW;
      }
      Color[40 * shadowData[i].y + shadowData[i].x] = TheGame.currentTile.color;
    }
  }
}

char getDropLine(char startX, char startY, char rotation) {
  __assume(startX < 10);
  __assume(startY < 20);
  __assume(rotation < 5);

  char sl = startY;
  for (char y = sl; y < 20; y++) {
    if (collision(startX, y, TheGame.rotation)) {
      break;
    }
    sl = y;
  }
  return sl;
}

void drawShadow() {
  bool remove = false;
  bool draw = false;
  char oldShadowStartX = shadowStartX;

  // first, calc x position of shadow in bowl
  shadowStartX = TheGame.tile_row;
  int realPos = (TheGame.tile_row * BLOCKSIZE << FBITS);
  if(realPos < TheGame.tile_row_graph) {
    shadowStartX++;
  } else if(realPos > TheGame.tile_row_graph) {
    shadowStartX--;
  }
  if(shadowStartX < 0) {
    shadowStartX = 0;
  }
  if(shadowStartX > 19) {
    shadowStartX = 19;
  }
    
  // only if the x position or the rotation has changed, continue
  if(shadowStartX != oldShadowStartX || TheGame.rotation != shadowRotation) {

    // calculate the y position of the shadow by testing for collisions all the way down
    char sl = getDropLine(shadowStartX, TheGame.tile_line, TheGame.rotation);

    // determine of shadow needs to be (re-)drawn or removed
    if(sl > TheGame.tile_line) {
      shadowStartY = sl;
      remove = true;
      draw = true;
    } else if(sl == TheGame.tile_line) {
      shadowStartY = -1;
      remove = true;
    }
  }
  if(remove) {
    removeShadow();
  }
  if(draw) {
    char c = 0;
    char rot = TheGame.rotation;
    char dimx = TheGame.currentTile.dimx[rot];
    char dimy = TheGame.currentTile.dimy[rot];
    unsigned int dataOffset = TheGame.currentTile.accessNodes[rot];

    for (char y = 0; y < dimy; y++) {
      for (char x = 0; x < dimx; x++) {
        if (TheGame.currentTile.data[dataOffset + y * dimx + x] == 1) {
          char screenX = (BOWLSTARTX / BLOCKSIZE) + shadowStartX + x;
          char screenY = (BOWLSTARTY / BLOCKSIZE) + shadowStartY + y;
          
          if(false && TheGame.currentTile.color == VCOL_ORANGE) {
            Screen[40 * screenY + screenX] = 133;
          } else {
            Screen[40 * screenY + screenX] = CHAR_SHADOW;
          }
          Color[40 * screenY + screenX] = TheGame.currentTile.color;
          shadowData[c].x = screenX;
          shadowData[c].y = screenY;
          c++;
        }
      }
    }
    shadowRotation = TheGame.rotation;
  }
}