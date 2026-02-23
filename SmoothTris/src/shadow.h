#ifndef SHADOW_H
#define SHADOW_H

void drawShadow();
void removeShadow();
void hideShadow();
void showShadow();
char getDropLine(char startX, char startY, char rotation);

#pragma compile("shadow.c")
#endif