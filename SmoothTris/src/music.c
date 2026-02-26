#include "music.h"
#include <c64/sid.h>


bool	music_off = false;

#pragma section( music, 0)
#pragma region( music, 0xa000, 0xc000, , , {music} )
#pragma data(music)
__export const char music[] = {
	#embed 0x2000 0x7e "../resources/gtu_a000_1x.sid" 
};
#pragma data(data)



void musicInit(char tune) {
	__asm
	{
		lda		tune
		jsr		$a000
	}
}

void musicPlay(void) {
  if(music_off) return;
	__asm
	{
		jsr		$a003
	}
}

void musicSilence(void) {
	sid.voices[0].ctrl = 0;
	sid.voices[0].susrel = 0;
	sid.voices[1].ctrl = 0;
	sid.voices[1].susrel = 0;
	sid.voices[2].ctrl = 0;
	sid.voices[2].susrel = 0;
}

void musicToggle(void) {
	if (music_off) {
		music_off = false;
  }	else{
		music_off = true;
		musicSilence();
	}
}

void musicOff(void) {
  music_off = true;
  musicSilence();
}

void musicOn(void) {
  music_off = false;
}