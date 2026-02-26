#include "gameirq.h"
#include "gamevars.h"
#include <c64/rasterirq.h>

RIRQCode	rirqlow, rirqup, rirqmenu;


static bool even = false;

__interrupt void irq_lower(void) {
  //Screen[160] = 'L';
  //Color[160] = VCOL_BLUE;
}

__interrupt void irq_upper(void) {
  //music_play();
  //Screen[160] = 'U';
  //Color[160] = VCOL_RED;
	/*
	if(even) {
		Screen[160] = 134;
		Color[160] = VCOL_RED;

		even = false;
	} else {
		Screen[160] = 135;
		Color[160] = VCOL_YELLOW;
		even = true;
	}
	*/
}


void gameirq_init(void)
{
  Screen[160] = 134;
	Color[160] = VCOL_RED;
  rirq_init_kernal();

	rirq_build(&rirqlow, 1);
	rirq_call(&rirqlow, 0, irq_lower);
	rirq_set(8, 250, &rirqlow);

	rirq_build(&rirqup, 1);
	rirq_call(&rirqup, 0, irq_upper);
	rirq_set(9, 10, &rirqup);

  /*
	rirq_build(&rirqmenu, 1);
	rirq_write(&rirqmenu, 0, &vic.spr_priority, 0xff);
	rirq_set(10, 241, &rirqmenu);
  */

	vspr_sort();
	vspr_update();
	rirq_sort();

	// start raster IRQ processing

	rirq_start();
}