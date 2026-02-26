#ifndef MUSIC_H
#define MUSIC_H


void musicInit(char tune);
void musicPlay(void);
void musicSilence(void);
void musicToggle(void);
void musicOff(void);
void musicOn(void);

#pragma compile("music.c")

#endif

