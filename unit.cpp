// Units
#include <stdlib.h> // abs
#include "tinystrat.h"
#include "defs.h"

#include "unit.h"
#include "game.h"

// get the terrain type of this position
uint8_t Unit::terrain() const
{
    return tile_terrain[game_info.vram[position()]];
}


// return resource type if can harvest, -1 if not
int Unit::can_harvest() const
{
    if ( type() != unit_farmer && type() != unit_farmer_f ) return -1;
    const int pos = position();
    // find resource for terrain
    for (int res=0;res<4;res++) {
        // terrain on destination
        if (resource_terrain[res]==tile_terrain[game_info.vram[pos]])
        	return res;
    }

    return -1;
}

void Unit::moveto(const char *path)
{
    // animate move to dest
    for (;*path;path++) {
        for (int i=0;i<16;i++){
            fr &= ~7;
            switch(*path) {
                case 'N' : y-=1; fr+=6; break;
                case 'S' : y+=1; fr+=4; break;
                case 'E' : x+=1; fr+=0; break;
                case 'W' : x-=1; fr+=2; break;
                default : message ("unknown character : %c\n",*path); break;
            }
            fr += (vga_frame/16)%2;
            game_info.bg_frame();
            wait_vsync();
        }
    }
    // reset to rest frame
    fr &= ~7;
}
