// grid array object
// simple graphical layer of 1bit data

extern "C" {
#include "sdk/lib/blitter/blitter.h" // object
}

#include "tinystrat.h"
#include "unit.h"
#include "game.h"

#include "grid.h"

#define GRID_COLOR  RGB(100,100,180)<<16 | RGB(80,80,150) // default
#define GRID_SPEED  32

extern Game game_info;

Grid::Grid(void)
{
	o.x = 0;
	o.y = 16;
	o.w = SCREEN_W*16;
	o.h = SCREEN_H*16;
	o.z = 50;

	o.data = data;
	o.frame = 0; // check grid empty & skip lines ? line by line ?
	o.line = grid_line;

	o.a = GRID_COLOR; // mode, used as color for now

	blitter_insert(&o);

	clear();
}

void Grid::clear() {
	for (int i=0;i<32;i++) data[i]=0;
}

extern "C" {
void Grid::grid_line (struct object *o)
{
	uint32_t *data=(uint32_t*)o->data;
	// no clipping
	const int y = vga_line-o->y;

	// blinking
	if ((vga_frame/GRID_SPEED)%2) return;

	// squares

	for (int i=0;i<SCREEN_W;i++) {
		uint16_t color = (i+y/16)%2 ? o->a >>16 : o->a&0x7fff;
		if (data[y/16] & (1<<i)) {
			uint16_t *firstpixel = &draw_buffer[o->x+i*16];

			if (y%16==0 || y%16==15 ) {
				for (int k=1;k<6;k++) {
					*(firstpixel+k)=color;
					*(firstpixel+15-k)=color;
				}
			} else if (y%16 != 7 && y%16!=8) {
				*firstpixel=color;
				*(firstpixel+15)=color;
			}
		}
	}
}
}

void Grid::color_movement_range(void)
{
	for (int y=0;y<SCREEN_H;y++) {
		data[y]=0;
		for (int x=0;x<SCREEN_W;x++)
			if (!cell_isempty(cost_array[(y+1)*SCREEN_W+x])) // start line 1
				data[y] |= 1<<x;
	}
}


// mark grid for all current player selectable units
void Grid::color_units(void)
{
	clear();
	for (int i=0;i<MAX_UNITS;i++) {
        Unit &u = game_info.units[i];

		if (!!u && u.player() == game_info.current_player)
			data[u.y/16-1] |= 1<<(u.x/16);
	}
}

void Grid::color_targets(void)
{
	clear();
	for (int i=0;i<game_info.nbtargets;i++) {
        Unit *u = game_info.targets[i];
		data[u->y/16-1] |= 1<<(u->x/16);
	}
}
