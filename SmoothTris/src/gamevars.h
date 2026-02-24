#ifndef GAMEVARS_H
#define GAMEVARS_H

#include <stdbool.h>
#include <c64/vic.h>

#define VIC_REG ((struct VIC*)0xd000)
#define FBITS	4
#define	FMUL	(1 << FBITS)

#define DEBUG 0
#define BOWLSTARTX 80
#define BOWLSTARTY 32
#define BOWLENDX 240
#define BOWLENDY 192
#define BLOCKSIZE 8
#define SIDEFRAMES 20
#define DROPFRAMES 10
#define EFFECT_DELAY_BASE 2
#define EFFECT_DELAY_SKIP 1
#define F_WIDTH 10
#define F_HEIGHT 20
#define SIDE_LATE_PIXELS 3   // how many pixels at the end of the 8px fall
#define SIDE_ALLOW_FROM  (BLOCKSIZE - SIDE_LATE_PIXELS)  // 8-3=5
#define PREVIEWX 30
#define PREVIEWY 3
#define BOWLCHARACTER 160

//typedef unsigned char byte;


extern byte* const Screen;
extern byte* const Color;
extern byte* const Sprite;
extern byte* const Charset;

static unsigned char singleBlockCharacter = 160;

extern const byte SpriteImage[64];

typedef struct t
{
  char num;
	char data[24];
	char accessNodes[4];
	char dimx[4];
	char dimy[4];
	char color;
  char x_spr_offsets[4];
  char y_spr_offsets[4];
} tile;

extern const struct tile tiles[7];

typedef struct {
  signed char x;
  signed char y;
} Vector2;

extern const Vector2 KICK_DATA_GENERIC[8][5];
extern const Vector2 KICK_DATA_I[8][5];


extern const char StatusText[];

extern const unsigned char level_y_inc[10];

static unsigned char prev_w = 0;
static unsigned char prev_z = 0;
static unsigned char prev_space = 0;
static unsigned char prev_p = 0;

typedef enum gs {
	GS_READY,			// Getting ready
	GS_PLAYING,			// Playing the game
  GS_PAUSED,
  GS_WON,
	GS_GAME_OVER,
  GS_EXIT,
  GS_PANIC,
  GS_EFFECT
} GameState;

typedef struct Game
{
	GameState		  state;
	tile			    currentTile;
  tile          nextTile;
  unsigned char grid[10][20];
  unsigned char bag[7];
  unsigned char bagpos;
  unsigned char	tile_line;
  int			      tile_line_graph;
  unsigned char tile_row;
  int			      tile_row_graph;
  signed char   sidemove;
  unsigned char drop;
  unsigned char	rotation;
  unsigned int	number_of_blocks;
  unsigned char	tileno;
  unsigned char	nexttileno;
  unsigned char cheat;
  unsigned int	lines;
  unsigned int	score;
  unsigned char level;
  unsigned char accel;
  int           tileCount;
};
extern struct Game TheGame;

void game_state(GameState state);
void game_keyboard(void);
void redrawGrid(void);
void putTile(void);

#pragma compile("gamevars.c")

#endif