#ifndef EFFECTS_H
#define EFFECTS_H
#include "gamevars.h"

#define EFFECT_DELAY 2

void cascadeLineSpr(char line);
void spiralLineSpr(char line);
void removeLine(unsigned char line);

#pragma compile("effects.c")

#endif