// grid array object
// simple graphical layer of 1bit data

#include "sdk/lib/blitter/blitter.h"
#include "tinystrat.h"

#define GRID_COLOR  RGB(156,136,126)<<16 | RGB(136,156,126) // default
#define GRID_SPEED  32

static uint32_t grid_data[32]; // 32 bits per line

static void grid_line (struct object *o)
{
	// no clipping
	const int y = vga_line-o->y;

	// blinking 
	if ((vga_frame/GRID_SPEED)%2) return;

	// squares

	for (int i=0;i<SCREEN_W;i++) {
		uint16_t color = (i+y/16)%2 ? o->a >>16 : o->a&0x7fff;
		if (grid_data[y/16] & (1<<i)) {
			uint16_t *firstpixel = &draw_buffer[o->x+i*16];

			if (y%16==0 || y%16==15 ) {
				for (int k=0;k<16;k++) {
					*(firstpixel+k)=color;
				}
			} else {
				*firstpixel=color;
				*(firstpixel+15)=color;
			}
		}
	}
}

void grid_empty() 
{
	for (int i=0;i<32;i++) grid_data[i]=0;
}

object *grid_new(void)
{
	object *o = blitter_new();
	o->x = 0;
	o->y = 16;
	o->w = SCREEN_W*16;
	o->h = SCREEN_H*16;

	o->data = grid_data;
	o->frame = 0;
	o->line = grid_line;

	o->a = GRID_COLOR; // mode, used as color for now
	grid_empty();
	return o;
}
