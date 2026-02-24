#include "gameirq.h"
#include "gamevars.h"
#include <c64/rasterirq.h>

RIRQCode	rirqlow, rirqup, rirqmenu;


__interrupt void irq_lower(void)
{
//	vic.color_border = VCOL_GREEN;
  /*
	vspr_update();
	if (irqphase == IRQP_UPDATE_SPRITE || irqphase == IRQP_INTRO)
	{
//		vic.color_border = VCOL_CYAN;
		rirq_sort(true);
	}
//	vic.color_border = VCOL_BLACK;

	if (irqphase == IRQP_WINDOW)
		return;
	
	if (irqphase == IRQP_INTRO)
		;
	else if (irqphase == IRQP_USER_INPUT)
	{
		irqphase = IRQP_MOVE_DIGGER;
		irqcount++;
	}
	else
		irqphase++;

	vic.spr_priority = 0x00;
  */
 Screen[160] = 'L';
 Color[160] = VCOL_BLUE;
}

__interrupt void irq_upper(void)
{
  /*
//	vic.color_border = VCOL_YELLOW;
	music_play();
//	vic.color_border = VCOL_PURPLE;
	
	switch(irqphase)
	{
	case IRQP_MOVE_DIGGER:
		diggers_move();
		enemies_move();
		key_scan();
		break;
	case IRQP_UPDATE_SPRITE:
//		vic.color_border = VCOL_YELLOW;
		{
			char si = diggers_sprites(0, mapx, mapy);
			si = enemies_sprites(si, mapx, mapy);

			while (si < 16)
			{
				vspr_move(si, 0, 255);
				si++;
			}
		}

//		vic.color_border = VCOL_CYAN;
		vspr_sort();
		key_scan();
		break;
	case IRQP_USER_INPUT:
		key_scan();
		user_interaction();
		break;
	case IRQP_WINDOW:
		for(char i=0; i<16; i++)
			vspr_move(i, 0, 255);
		vspr_sort();
		break;
	case IRQP_INTRO:
		break;
	}

	sidfx_loop_2();
  */
	
//	vic.color_border = VCOL_BLACK;
  Screen[160] = 'U';
  Color[160] = VCOL_RED;
}


void gameirq_init(void)
{

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