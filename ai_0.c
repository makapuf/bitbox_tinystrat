/* test ai */
#include "stdlib.h" // rand : make myrand ?
#include "tinystrat.h"
#include "units.h"


static const int8_t neighbours[]={
	+1,-1,
	+SCREEN_W, -SCREEN_W,
	SCREEN_W+1,SCREEN_W-1,
	-SCREEN_W+1,-SCREEN_W-1
};

// dummy CPU play
void play_CPU0 ()
{
    char path[MAX_PATH]; // merge & remove ?

    for (int i=0;i<MAX_UNITS;i++) {
        if (game_info.units[i])
            unit_reset_moved(i);
    }
    harvest();

	for (int unit_id=0;unit_id<NB_UNITS;unit_id++) { // randomize order ?		

		if (unit_empty(unit_id) || unit_get_player(unit_id) != game_info.current_player)
			continue;
		update_pathfinding(unit_id);

		// find an enemy player in range, go towards it
		int tgt_unit;
		for (tgt_unit=0;tgt_unit<MAX_UNITS;tgt_unit++) {
			if (unit_empty(tgt_unit) || unit_get_player(tgt_unit)== game_info.current_player)
				continue;

			int pos = unit_get_pos(tgt_unit);

			// find free neighbour place 
			for (int n=0;n<8;n++) {
				const int dest = pos+neighbours[n]; 
				// fixme not if range is not 1!
				// not if cannot attack !

				if (!cell_isempty(cost_array[dest]) && unit_at(dest)==-1) {
					// go there 
			        reconstruct_path(dest, path);
			        if (*path=='!') continue; // error, cannot move there
			        unit_moveto(unit_id, path);
			        wait_vsync(20);
					combat(unit_id,tgt_unit);
					// done for this unit
					goto end_unit_turn;
				}
			}
		}

		// no unit in sight
		if (tgt_unit==MAX_UNITS) {
			// do nothing
		}

end_unit_turn:

		unit_set_moved(unit_id);
	}

}