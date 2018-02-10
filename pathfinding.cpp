#include <stdint.h>
#include <stdbool.h>

#include "tinystrat.h"
#include "unit.h"
#include "game.h"

// ----------------------------------------------------------------
// - cell

static const struct Cell empty_cell=(struct Cell){.cost=MAX_COST, .pos=0};

struct Cell cost_array[SCREEN_W* SCREEN_H]; // cost, come_from
struct Cell frontier  [FRONTIER_SIZE];      // cost, position

void cost_init(uint16_t source)
{
	for (int i=0;i<SCREEN_W*SCREEN_H;i++)
		cost_array[i] = empty_cell;
	cost_array[source] = (struct Cell) {.cost=0,.pos=source};
}

bool cell_isempty(struct Cell c) {	return c.cost==MAX_COST; }
// ----------------------------------------------------------------
// - frontier

static void frontier_init(uint16_t source)
{
	for (int i=1;i<FRONTIER_SIZE;i++)
		frontier[i]=empty_cell;
	frontier[0] = (struct Cell) {0,source};
}

static int frontier_add (uint8_t new_cost, uint16_t npos) // returns -1 if not found
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

static void try_neighbour(uint8_t npos, uint16_t from, const int max_cost, const int unit_type)
{
	uint8_t new_cost = cost_array[from].cost + terrain_move_cost[unit_type][tile_terrain[game_info.vram[npos]]];
	if (new_cost>MAX_COST) new_cost=MAX_COST;

	if (new_cost < cost_array[npos].cost && new_cost<=max_cost) {
		frontier_add(new_cost, npos);

		// update current table with new_cost, come_from = current
		cost_array[npos] = (struct Cell){.cost=new_cost,.pos=from};
	}
}


// updates cost_array and frontier, from source_unit type and position, and max travel distance
void update_pathfinding ( const Unit &source )
{
	// Dijkstra algorithm
	uint16_t source_pos = source.position();
	const uint8_t unit_type = source.type();
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
			if (c_y>1)          try_neighbour(c-SCREEN_W,c, max_cost, unit_type);
			if (c_y<SCREEN_H-1) try_neighbour(c+SCREEN_W,c, max_cost, unit_type);
		}
	}
}

// once cost_array has been created, find the path to a given point.
// source has already been given by building the cost array
void reconstruct_path(int dst, char *path)
{
	int len=0;
	char *p=&path[MAX_PATH-1]; // start at the end

	while(cost_array[dst].cost) { // stop if/when cost is zero  {
		// error ?
		if (cell_isempty(cost_array[dst]) || len==MAX_PATH) {
			*path='!';
			return;
		}

		// message("%d,%d\n", dst%SCREEN_W, dst/SCREEN_W);
		int new_dst = cost_array[dst].pos;
		switch(new_dst-dst) {
			case  1 : *p-- = 'W'; break;
			case -1 : *p-- = 'E'; break;
			case  SCREEN_W : *p-- = 'N'; break;
			case -SCREEN_W : *p-- = 'S'; break;
		}
		dst=new_dst;
		len++;
	}

	// now pad string left
	for (int i=0;i<=len;i++)
		path[i] = path[MAX_PATH-len+i];
    path[len]='\0';
    message("resulting path : %s\n",path);
}

