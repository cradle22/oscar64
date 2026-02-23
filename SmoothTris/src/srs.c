#include "srs.h"
#include "gamevars.h"


// Returns 1 if collision occurs, 0 if clear
int check_collision(char piece_idx, int rot, char pos_x, char pos_y) {
  __assume(piece_idx < 8);
  __assume(rot < 4);
  __assume(pos_x < 10);
  __assume(pos_y < 20);
  const struct tile *t = &tiles[piece_idx];
  char start_node = t->accessNodes[rot];
  char width = t->dimx[rot];
  char height = t->dimy[rot];

  for (char i = 0; i < (width * height); i++) {
    if (t->data[start_node + i] != 0) {
      // Calculate local x, y within the piece bounding box
      char lx = i % width;
      char ly = i / width;
      
      char gx = pos_x + lx;
      char gy = pos_y + ly;

      // Check boundaries
      if (gx < 0 || gx >= 10 || gy < 0 || gy >= 20) return 1;
      // Check grid (assuming grid[x][y])
      if (TheGame.grid[gx][gy] > 0) return 1;
    }
  }
  return 0;
}

// Attempts to rotate a piece using SRS Wall Kicks
// dir: 1 for clockwise, -1 for counter-clockwise
bool try_rotate(int *current_rot, char piece_idx, char *pos_x, char *pos_y, char dir) {
  __assume(*current_rot < 4);
  __assume(piece_idx < 8);
  __assume(*pos_x < 10);
  __assume(*pos_y < 20);
  __assume(dir < 2);
  if (piece_idx == 1) return false; // O-piece doesn't rotate

  int old_rot = *current_rot;
  int new_rot = (old_rot + dir + 4) % 4;

  // Determine which index in the KICK_DATA table to use
  char table_idx;
  if (dir == 1) { // Clockwise transitions: 0->1, 1->2, 2->3, 3->0
    table_idx = old_rot * 2;
  } else { // CCW transitions: 1->0, 2->1, 3->2, 0->3
    table_idx = (new_rot * 2) + 1;
  }

  const Vector2 (*kicks)[5] = (piece_idx == 0) ? KICK_DATA_I : KICK_DATA_GENERIC;

  for (char i = 0; i < 5; i++) {
    char test_x = *pos_x + kicks[table_idx][i].x;
    char test_y = *pos_y + kicks[table_idx][i].y;

    if (!check_collision(piece_idx, new_rot, test_x, test_y)) {
      // Success! Update piece state
      *pos_x = test_x;
      *pos_y = test_y;
      *current_rot = new_rot;
      return true;
    }
  }
  // If all 5 tests fail, rotation is cancelled
  return false;
}