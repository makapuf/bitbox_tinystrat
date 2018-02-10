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
