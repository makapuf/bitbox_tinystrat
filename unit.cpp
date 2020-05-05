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
            #ifdef FLAG_WALK_ANIMATION
            wait_vsync();
            #endif
        }
    }
    // reset to rest frame
    fr &= ~7;
}


void Unit::die() {
    message ("unit died\n");

    if (type() == unit_flag) {
        wait_vsync(); // avoid unit just moved
    } else {
        // XXX sfx ?

        // -- animation
        // turn on itself
        for (int i=0;i<12;i++) {
            fr = (fr&~7) | (uint8_t []){0,4,2,6}[i%4];
            for (int j=0;j<8;j++) {
                game_info.bg_frame();
                wait_vsync();
            }
        }

        // cross fall animation
        fr = fr_unit_cross;
        const int y0 = y;
        const int n = SZ(anim_cross_fall);

        for (int j=n/2;j<n;j++) {
            y = y0 - anim_cross_fall[j];
            wait_vsync(); game_info.bg_frame();
        }
        for (int j=0;j<n;j++) {
            y = y0 - anim_cross_fall[j]/2;
            wait_vsync(); game_info.bg_frame();
        }
        for (int j=0;j<n;j++) {
            y = y0 - anim_cross_fall[j]/6;
            wait_vsync(); game_info.bg_frame();
        }
        wait_vsync(32);
    }


    // count units of died player, stop at 2 since we just check =1
    int n=0;
    for (int i=0;i<MAX_UNITS && n<2 ;i++) {
        Unit *u = &game_info.units[i];
        if (!!*u && u->player() == player()) {
            n++;
        }
    }

    game_info.unit_remove(this);
    if (n==1 || type()==unit_flag) // only this one left ?
        game_info.eliminate_player(player());
}
