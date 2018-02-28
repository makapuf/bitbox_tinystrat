#!/usr/bin/env python3
import xml.etree.ElementTree as ET
from PIL import Image

tsx=ET.parse('tiles_bg.tsx').getroot()

# export terrains
terrains = [elt.get('name') for elt in tsx.findall('terraintypes/terrain')]
tile2terrain = {
	int(e.get('id')) : int(e.get('terrain','255,').split(',')[0])
	for e in tsx.findall('tile')
}

colors   = 'blue red yellow green'.split()
units    = 'farmer farmer_f soldier soldier_f archer guard guard2 catapult belier tower horse knight boat'.split() # in order horizontally
cursors  = 'cursor cursor2 left right down up'.split()
flags    = 'flag1 flag2 flag3 flag4 flag5 bullet1 bullet2 bullet3'.split()

resources = 'food gold wood stone'.split()
tags      = 'hurt surrender medal1 medal2 medal3'.split()
misc      = 'skull magic1 magic2 swirl1 swirl2 explo1 explo2 explo3 mouse bzz'.split()
menus 	  = 'bg harvest attack empty'.split()
MOV_DEFAULT = 1
ATTACK_DEFAULT=0

def unit_type(unit) :
	MTYPES = {
		('farmer','farmer_f','soldier','soldier_f','archer','guard','guard2') : 'foot',
		('belier','catapult','tower') : 'wheels',
		('horse','knight') : 'horse',
		('boat',) : 'boat',
	}
	for k,v in list(MTYPES.items()) :
		if unit in k :
			return v
	raise ValueError("no known type for unit %s"%u)

def speed(unit, terrain) :
	mtype=unit_type(unit)
	elt =tsx.find('terraintypes/terrain[@name="%s"]'%terrain)
	pmov=elt.find('properties/property[@name="move_%s"]'%mtype)
	return int(pmov.get('value')) if pmov is not None else MOV_DEFAULT

def damage(of,to) :
	'''
	This function defines the attack level of a unit on another.
	'''
	mtype=unit_type(to)
	of = of.rsplit('_',1)[0] # removes _f
	att = {
		('soldier','foot') : 3,
		('soldier','wheels') : 3,
		('soldier','horse') : 3,
		('soldier','boat') : 3,

		('archer','foot') : 5,
		('boat','boat') : 3,
		('guard','foot') : 6,
	}
	return att.get((of,mtype),ATTACK_DEFAULT)

def unit_attack_range(u) :
	u2 = u.split('_')[0]
	if u2=='farmer' : return (0,0)
	if u2=='catapult' : return (2,8)
	return (0,1)

def unit_distance_range(u) :
	return {'foot':5,'wheels':4,'horse':10,'boat':10}[unit_type(u)]

resources_terrains = [
    ('food', 'fields'),
    ('gold', 'town'),
    ('wood', 'forest'),
    ('stone','mountains'),
]

# Headers
# ---------------------------------
print('#include <stdint.h>')
print('#include "data.h"')
print('#ifndef DEFS_DEFINITION')
print('#define DEFS_DEFINITION')

# -- terrains
print("enum {")
for t in terrains :
	print('    terrain_%s,'%t)
print('};')
print('#define NB_TERRAINS %d'%len(terrains))

# -- resources
print("enum {")
for res in resources :
	print("    resource_%s,"%res)
print('};')

# -- units
print("enum {")
for c in colors :
	print("    color_%s,"%c)
print('};')

print("enum {")
for u in units :
	print("    unit_%s,"%u)
print("};")
print('#define NB_UNITS %d'%len(units))

print('// ---- Frames')
print('// - sprite units (colored)')

print("enum {")
print('    // units')
for ui,u in enumerate(units) :
	print("    fr_unit_%s = %d,"%(u,ui*8))
print('    // cursors')
for ui,u in enumerate(cursors) :
	print("    fr_unit_%s = %d,"%(u,len(units)*8+ui))
print('    // flags')
for ui,u in enumerate(flags) :
	print("    fr_unit_%s = %d,"%(u,len(units)*8+8+ui))
print("};")

print('// - sprite misc.')
print("enum {")
for ai,a in enumerate(resources) :
	print("    fr_misc_%s = %d,"%(a,ai))
for ai,a in enumerate(tags) :
	print("    fr_misc_%s = %d,"%(a,8+ai))
for i in range(16) :
	print("    fr_misc_%d = %d,"%(i,16+i))
for ai,a in enumerate(misc) :
	print("    fr_misc_%s = %d,"%(a,24+ai))
print('};')


# other named tiles in tileset
for t in ('wood','zero','P1','mark'):
	elt = tsx.find('tile[@type="%s"]'%t)
	print("#define tile_%s %d"%(t,int(elt.get('id'))+1))

# menus
for i,m in enumerate(menus) :
	print("#define MENU_%s %d"%(m.upper(),i))

print('extern const uint8_t tile_terrain[];')
print('extern const char * unit_names[];')
print('extern const char * terrain_names[];')
print('extern const uint8_t unit_damage_table[][NB_UNITS];')
print('extern const uint8_t terrain_defense[];')
print('extern const uint8_t terrain_move_cost[][NB_TERRAINS];')
print('extern const uint8_t terrain_move_cost[][NB_TERRAINS];')
print('extern const uint8_t unit_attack_range_table[][2];')
print('extern const uint8_t unit_movement_range_table[];')
print('extern const uint8_t level_units[][32][4];')
print('extern const char *terrain_bg_table[];')
print('extern const uint8_t resource_terrain[];')
print('#endif\n')


# Implementation
# ----------------------

print('#ifdef DEFS_IMPLEMENTATION')

# tile -> terrain
cnt = int(tsx.get('tilecount'))
col = int(tsx.get('columns'))
print('const uint8_t tile_terrain[%d] = { 255,'%(cnt+1)) # tile zero is whatever
for i in range((cnt+col-1)//col) :
	print('    '+''.join('%d,'%(tile2terrain.get(i*col+j,255)) for j in range(col)))
print('};')

# terrain names
print("const char *terrain_names[] = {")
for t in terrains :
	print("  \"%s\","%t)
print('};')
# unit names
print("const char *unit_names[] = {")
for u in units :
	print("  \"%s\","%u)
print('};')

# terrain defense
print('const uint8_t terrain_defense[%d] = {'%len(terrains))
for elt in tsx.findall('terraintypes/terrain') :
	defn = elt.find('properties/property[@name="defense"]').get('value')
	print("    [terrain_%s]=%d,"%(elt.get('name'),int(defn)+1))
print('};')

# terrain backgrounds
print("const char *terrain_bg_table[] = {")
for t in terrains :
	print("    [terrain_%s] = &_binary_sprites_bg_%s_spr_start,"%(t,t))
print("};")

# terrain/unit movement cost
print('const uint8_t terrain_move_cost[%d][%d]={'%(len(units),len(terrains)))
for u in units :
	print('    {%s}, // %s'%(','.join(str(speed(u,t)) for t in terrains),u))
print('};')

# unit/unit attack efficiency 0-16
print('const uint8_t unit_damage_table[%d][%d]={'%(len(units),len(units)))
for u_of in units :
	print('    {%s}, // %s'%(','.join(str(damage(u_of,u_to)) for u_to in units),u_of))
print('};')

# unit/unit attack range in tiles
print('const uint8_t unit_attack_range_table[%d][2]={'%len(units))
for u in units :
	r = unit_attack_range(u)
	print('    {%d, %d}, // %s'%(r[0],r[1],u))
print('};')

# unit travel distance
print('const uint8_t unit_movement_range_table[%d]={'%len(units))
for u in units :
	r = unit_distance_range(u)
	print('    %d, // %s'%(r,u))
print('};')


print('const uint8_t resource_terrain[4]={ // per resource')
for resource,terrain in resources_terrains :
	print('    [resource_{0}]  = terrain_{1},'.format(resource,terrain))
print('};')


# map units
# --------------------------
tmx=ET.parse('map.tmx').getroot()

units_ts = [x for x in tmx.findall('tileset[image]') if x.find('image').get('source')=="map_units.png"][0]
firstgid_units = int(units_ts.get('firstgid'))
tmap_w = int(tmx.get('width'))
print('// ----')
print('// Units initial positions by level')
print("const uint8_t level_units[][32][4] = { // N levels, units, {x,y,type 1-16, color}")
for l in tmx.findall('layer') :
	if l.get('name').endswith('_units') and l.get('name').startswith('_level'):
		dat = l.find('data')
		assert dat.get('encoding')=='csv','not a csv file'

		lvl = [int(x) for x in dat.text.replace('\n','').split(',')]
		items = [(n,t-firstgid_units) for n,t in enumerate(lvl) if t]

		print('    {')
		for id,item in items :
			color = item//16
			unit  = item%16
			if unit==15 : continue # flag : fixme special
			x = id%tmap_w
			y = id//tmap_w
			print("        {%d,%d,unit_%s+1,color_%s},"%(x,y,units[unit],colors[color]))
		print('    },')

print("};")

print('#endif')
