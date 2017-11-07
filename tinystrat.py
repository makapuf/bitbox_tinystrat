import xml.etree.ElementTree as ET
from PIL import Image

tsx=ET.parse('tiles_bg.tsx').getroot()

# export terrains 
terrains = [elt.get('name') for elt in tsx.findall('terraintypes/terrain')]
tile2terrain = {
	int(e.get('id')) : int(e.get('terrain','-1,').split(',')[0]) 
	for e in tsx.findall('tile')
}

colors   = 'blue red yellow green'.split()
units    = 'farmer farmer_f soldier soldier_f archer guard guard2 catapult belier tower horse knight boat'.split() # in order horizontally
cursors  = 'cursor cursor2 left right down up'.split()
flags    = 'flag1 flag2 flag3 bullet1 bullet2 bullet3'.split()

resources = 'gold iron coal wood food'.split()
tags      = 'hurt surrender medal1 medal2 medal3'.split()
misc      = 'skull magic1 magic2 swirl1 swirl2 explo1 explo2 explo3 mouse bzz'.split()

MOV_DEFAULT = 1
def speed(unit, terrain) : 
	mtypes = {
		('farmer','farmer_f','soldier','soldier_f','archer','guard','guard2') : 'foot',
		('belier','catapult','tower') : 'wheels',
		('horse','knight') : 'horse',
		('boat',) : 'boat',
		('guard2',) : 'guard2',
	}
	for k,v in mtypes.items() : 
		if unit in k : 
			mtype = v
			break
		
	elt =tsx.find('terraintypes/terrain[@name="%s"]'%terrain)
	pmov=elt.find('properties/property[@name="move_%s"]'%mtype)
	if pmov is None  : return MOV_DEFAULT
	return int(pmov.get('value'))

def attacks(of,to) : 
	'''
	This function defines the attack level of a unit on another.
	'''
	return 0

def distance(unit, terrain) : # attack distance
	return 1


# Headers 
# ---------------------------------
print '#ifndef TINYSTRAT_DEFINITION'
print '#define TINYSTRAT_DEFINITION'
# -- terrains
print "enum {"
for t in terrains : 
	print '    terrain_%s,'%t
print '};'

# -- units 

print "enum {"
for c in colors : 
	print "    color_%s,"%c
print '};'

print "enum {"
for u in units : 
	print "    unit_%s,"%u
print "};"

print '// ---- Frames'
print '// - sprite units (colored)'

print "enum {"
print '    // units'
for ui,u in enumerate(units) :
	print "    fr_unit_%s = %d,"%(u,ui*8)
print '    // cursors'
for ui,u in enumerate(cursors) :
	print "    fr_unit_%s = %d,"%(u,len(units)*8+ui)
print '    // flags'
for ui,u in enumerate(flags) :
	print "    fr_unit_%s = %d,"%(u,len(units)*8+8+ui)
print "};"

print '// - sprite misc.'
print "enum {"
for ai,a in enumerate(resources) : 
	print "    fr_misc_%s = %d,"%(a,ai)
for ai,a in enumerate(tags) : 
	print "    fr_misc_%s = %d,"%(a,8+ai)
for i in range(16) : 
	print "    fr_misc_%d = %d,"%(i,16+i)
for ai,a in enumerate(misc) : 
	print "    fr_misc_%s = %d,"%(a,24+ai)
print '};'


# other named tiles in tileset
for t in ('wood','zero'):  
	elt = tsx.find('tile[@type="%s"]'%t)
	print "#define tile_%s %s"%(t,elt.get('id'))

print '#endif'

# Implementation 
# ----------------------

print '#ifdef TINYSTRAT_IMPLEMENTATION'

# tile -> terrain
cnt = int(tsx.get('tilecount'))
col = int(tsx.get('columns'))
print 'const uint8_t tile_terrain[%d] = {'%cnt
for i in range((cnt+col-1)//col) : 
	print '    '+''.join('%d,'%(tile2terrain.get(i*col+j,0)) for j in range(col))
print '};'

# terrain defense
print 'const uint8_t terrain_defense[%d] = {'%len(terrains)
for elt in tsx.findall('terraintypes/terrain') : 
	defn = elt.find('properties/property[@name="defense"]').get('value')
	print "    [terrain_%s]=%d, "%(elt.get('name'),int(defn)+1)
print '};'

# terrain/unit movement cost 
print 'const uint8_t terrain_move_cost[%d][%d]={'%(len(units),len(terrains))
for u in units : 
	print '  {%s}, // %s'%(','.join(str(speed(u,t)) for t in terrains),u)
print '};'

# map units 
# --------------------------
tmx=ET.parse('map.tmx').getroot()

units_ts = [x for x in tmx.findall('tileset[image]') if x.find('image').get('source')=="map_units.png"][0]
firstgid_units = int(units_ts.get('firstgid'))
tmap_w = int(tmx.get('width'))
print '// ----'
print '// Units initial positions by level'
print "uint8_t level_units[][32][4] = { // N levels, units, {x,y,type 1-16, color}"
for l in tmx.findall('layer') : 
	if l.get('name').endswith('_units') and l.get('name').startswith('_level'): 
		dat = l.find('data')
		assert dat.get('encoding')=='csv','not a csv file'

		lvl = [int(x) for x in dat.text.replace('\n','').split(',')]
		items = [(n,t-firstgid_units) for n,t in enumerate(lvl) if t]

		print '    {'
		for id,item in items : 
			color = item/16
			unit  = 1+item%16
			x = id%tmap_w
			y = id/tmap_w
			print "        {%d,%d,unit_%s,color_%s},"%(x,y,units[unit],colors[color])
		print '    },'

print "};"

print '#endif'
