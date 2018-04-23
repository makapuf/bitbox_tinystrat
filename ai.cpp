/* test ai */
#include "stdlib.h" // rand : make myrand ?
#include "tinystrat.h"
#include "unit.h"
#include "game.h"

// best action by heuristic
// updates target, pos
void find_next_best(Unit &u, int &npos, Unit * &target)
{
	int original_pos = u.position(); // save position

	// by default stay here
	int best_score = u.can_harvest()<0 ? 0 : 10;
	npos = original_pos;
	target = nullptr;

	// foreach possible move destination
	for (int dest=SCREEN_W;dest<SCREEN_H*SCREEN_W;dest++) {
		if (cell_isempty(cost_array[dest]))
			continue; // cannot reach
		if (game_info.unit_at(dest)!=nullptr)
			continue; // not free

		// "move" it to new destination - should not be visible
		u.position(dest);

		const int harvest_score = u.can_harvest()<0 ? 0 : 10;

		const int proxymity_score = 0; // closer to my own units
		const int target_proximity_score = 0; // closer to castle

		const int score = harvest_score + proxymity_score + target_proximity_score;

		// XXX within direct range of being attacked (sum of damage possibly taken) ?

		// try doing nothing
		if ( score > best_score ) {
			message (
				"best so far : harvest %d, proxymity %d, target %d \n",
				harvest_score,
				proxymity_score,
				target_proximity_score
				);

			best_score=score;
			npos=dest;
			target=nullptr;
		}

		// -- for each target in range : try to attack
		for (int i=0 ; i<MAX_UNITS ; i++) {
			Unit& tgt = game_info.units[i];
			if (!tgt) continue; // unused unit, skip
			if (tgt.player() == game_info.current_player) continue; // same player

			int damage=0;
			if (u.can_attack(tgt)) {   // unit within reach ?
				// get score of potential attack
				damage = u.attack_damage(tgt) * u.health() / 256;
				// retaliate
				if (tgt.health()>damage && tgt.can_attack(u)) {   // unit within reach ?
					damage -= u.attack_damage(tgt) * (u.health()-damage) / 256; // anticipate future damage
				}
			}

			// destroyed ? bad
			if (score + damage*3 > best_score ) {
				best_score = score + damage;
				npos=dest;
				target=&tgt;
			}
		}
	}

	// restore unit position
	u.position(original_pos);
}


// CPU play
void play_CPU ()
{
	char path[MAX_PATH]; // merge & remove ?

	for (int i=0;i<MAX_UNITS;i++) {
		Unit& u = game_info.units[i];
		if (!!u) u.moved(false);
	}
	game_info.harvest(); // to main ?

	for ( Unit *u = game_info.myunits(0) ; u ; u=game_info.myunits(u) ) {

		wait_vsync(); // thinking ...
		update_pathfinding(*u);

		int next_pos=0; // target position found by AI
		Unit *target;   // target attack found by AI (or null if pause here).

		find_next_best(*u, next_pos, target);

		// go there
		reconstruct_path(next_pos, path);
		if (*path=='!') continue; // error, cannot move there
		u->moveto(path);

		for (int i=0;i<20;i++) wait_vsync(); // little pause
		if (target)
			combat(*u,*target);

		u->moved(true);
	}
}



