// grid array object
// simple graphical layer of 1bit data

#include "sdk/lib/blitter/blitter.h"
#include "tinystrat.h"
#include "units.h"

#define GRID_COLOR  RGB(100,100,180)<<16 | RGB(80,80,150) // default
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
	o->z = 50;

	o->data = grid_data;
	o->frame = 0; // check grid empty & skip lines ? line by line ? 
	o->line = grid_line;

	o->a = GRID_COLOR; // mode, used as color for now
	grid_empty();
	return o;
}


void color_grid_movement_range(void)
{
	uint32_t *grid = (uint32_t*) game_info.grid->data;
	for (int y=0;y<SCREEN_H;y++) {
		grid[y]=0;
		for (int x=0;x<SCREEN_W;x++) 
			if (!cell_isempty(cost_array[(y+1)*SCREEN_W+x])) // start line 1 
				grid[y] |= 1<<x;
	}
}


// mark grid for all current player selectable units
void color_grid_units(void)
{
	uint32_t *grid = (uint32_t*) game_info.grid->data;
	grid_empty();
	for (int u=0;u<MAX_UNITS;u++) {		
		if (unit_empty(u) || unit_get_player(u) != game_info.current_player)
			continue;
        object *obj = game_info.units[u];
		grid[obj->y/16-1] |= 1<<(obj->x/16);
	}
}

void color_grid_targets(void)
{
	uint32_t *grid = (uint32_t*) game_info.grid->data;
	grid_empty();
	for (int i=0;i<game_info.nbtargets;i++) {		
        object *obj = game_info.units[game_info.targets[i]];
		grid[obj->y/16-1] |= 1<<(obj->x/16);
	}
}