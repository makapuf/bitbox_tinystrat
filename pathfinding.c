#include <stdint.h>
#include <stdbool.h>

#include "tinystrat.h"
#include "units.h"

// ----------------------------------------------------------------
// - cell

static const struct Cell empty_cell=(struct Cell){.pos=0, .cost=MAX_COST};
bool cell_isempty(struct Cell c)
{
	return c.cost==MAX_COST;
}

struct Cell cost_array[SCREEN_W* SCREEN_H]; // cost, come_from
struct Cell frontier  [FRONTIER_SIZE];      // cost, position

void cost_init(int source)
{
	for (int i=0;i<SCREEN_W*SCREEN_H;i++)
		cost_array[i] = empty_cell; // max_cost+1 !
	cost_array[source] = (struct Cell) {.cost=0,.pos=source};
}

// ----------------------------------------------------------------
// - frontier 

static void frontier_init(int source)
{
	for (int i=1;i<FRONTIER_SIZE;i++)
		frontier[i]=empty_cell;
	frontier[0] = (struct Cell) {0,source};
}

static int frontier_add (int new_cost, int npos) // returns -1 if not found
{
	// could use a free_frontier array
	// append to frontier : find free cell, write to it. 
	
	// message("Add cost:%d x:%d y:%d to frontier\n",new_cost,npos%SCREEN_W,npos/SCREEN_W);
	for (int i=0;i<FRONTIER_SIZE;i++) {
		// free cell found ? 
		if (cell_isempty(frontier[i])) {
			frontier[i] = (struct Cell){.cost=new_cost, .pos=npos};
			return i;
		}
	}
	message("ERROR : not enough room in frontier !\n");
	return -1;
}

// gets minimum cost from frontier
static struct Cell frontier_pop ( void ) 
{
	// find smallest element in frontier
	int mini=0, minicost=MAX_COST;

	for (int i=0;i<FRONTIER_SIZE;i++) {
		if (frontier[i].cost<minicost) {
			mini=i;
			minicost = frontier[i].cost;
		}
	}
	// nothing in frontier and not found : error
	if (minicost==MAX_COST) return empty_cell;

	// get & remove it
	struct Cell current = frontier[mini]; 
	frontier[mini] = empty_cell;
	return current;
}

// ----------------------------------------------------------------
// - main update 

static void try_neighbour(int npos, int from, const int max_cost, const int unit_type)
{
	int new_cost = cost_array[from].cost + terrain_move_cost[unit_type][tile_terrain[game_info.vram[npos]]];
	if (new_cost>MAX_COST) new_cost=MAX_COST;

	if (new_cost < cost_array[npos].cost && new_cost<=max_cost) {
		frontier_add(new_cost, npos);

		// update current table with new_cost, come_from = current
		cost_array[npos] = (struct Cell){.cost=new_cost,.pos=from};
	}
}


// updates cost_array and frontier, from source_unit type and position, and max travel distance
void update_pathfinding ( int source_unit )
{
	// Dijkstra algorithm
	uint16_t source_pos = unit_get_pos(source_unit);
	const uint8_t unit_type = unit_get_type(source_unit);
	uint8_t  max_cost = unit_movement_range_table[unit_type];

	cost_init(source_pos);
	frontier_init(source_pos);

	while (1) 
	{
		struct Cell current = frontier_pop();
		if (cell_isempty(current)) { // not found ?
			break;
		} else {
			int c = current.pos;

			// add cells around to frontier
			int c_x = c%SCREEN_W;
			int c_y = c/SCREEN_W;

			// try possible neighbours 
			if (c_x>0)          try_neighbour(c-1		,c, max_cost, unit_type);
			if (c_x<SCREEN_W-1) try_neighbour(c+1		,c, max_cost, unit_type);
			if (c_y>0)          try_neighbour(c-SCREEN_W,c, max_cost, unit_type);
			if (c_y<SCREEN_H-1) try_neighbour(c+SCREEN_W,c, max_cost, unit_type);
		}
	}
}

// once cost_array has been created, find the path from an array
void reconstruct_path(int src, int dst)
{
	do {
		// message("%d,%d\n", dst%SCREEN_W, dst/SCREEN_W);
		dst = cost_array[dst].pos;
		if (cell_isempty(cost_array[dst])) { 
			// fixme handle errors better ...
			message("Error : cannot recontruct path !\n");
			die(3,2);
		}
	} while(dst != src);
}


void color_map_movement_range(void)
{
	for (int pos=SCREEN_W;pos<SCREEN_H*SCREEN_W;pos++) // start line 1 
		if (!cell_isempty(cost_array[pos]))
			game_info.vram[pos] = tile_mark;
}