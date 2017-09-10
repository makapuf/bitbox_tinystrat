import xml.etree.ElementTree as ET
from PIL import Image

tsx=ET.parse('tiles_bg.tsx').getroot()

# export terrains 
terrains = [elt.get('name') for elt in tsx.findall('terraintypes/terrain')]
tile2terrain = {
	int(e.get('id')) : int(e.get('terrain','-1,').split(',')[0]) 
	for e in tsx.findall('tile')
}

colors  = 'blue red yellow green'.split()
units   = 'farmer farmer_f peon peon_f archer guard guard2'.split()
units2  = 'catapult belier tower horse knight boat'.split() # in order horizontally
cursors  = 'cursor cursor2 left right down up flag1 flag2 flag3'.split()
resources ='gold iron coal wood food'.split()
tags    = 'hurt surrender medal1 medal2 medal3'.split()
misc    = 'skull magic1 magic2 swirl1 swirl2 explo1 explo2 explo3 mouse'.split()

MOV_DEFAULT = 1
def speed(unit, terrain) : 
	mtypes = {
		('person','farmer','farmer_f','peon','peon_f','archer','guard') : 'foot',
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




class Frameset : 
	def __init__(self) : 
		self.frames=[] # frames as a small images list
		self.frameimg=Image.open('sprites.png') # should be static
		# read replacement colors

		px = self.frameimg.load()
		self.colors = [] # each color, each shade
		for base in range(0,16,4) : 
			self.colors.append([px[15*16+base,10*16+ shade] for shade in range(4)])

	def len(self) : return len(self.frames)

	def add_frame(self, tileid, color=0, mirror=False) : 
		# mapping of new frame_id : frame original tileid, horizontal mirroring, remap color 0(none) or 1-3 
		x=(tileid%16)*16
		y=(tileid//16)*16
		im=self.frameimg.crop((x,y,x+16,y+16))
		if mirror : 
			im=im.transpose(Image.FLIP_LEFT_RIGHT)
		if color : 
			color = colors.index(color)
			px=im.load()
			for x in range(16) : 
				for y in range(16) : 
					for ori,dest in zip(self.colors[0],self.colors[color]) : 
						if px[x,y] == ori : px[x,y] = dest

		self.frames.append(im)

	def write(self, filename) : 
		img2 = Image.new('RGBA',(16,16*len(self.frames)))
		for ifr,fr in enumerate(self.frames) :
			img2.paste(fr,(0,ifr*16))
		img2.save(filename)

	def frline(self, name, list, line,col=None) : 
		print "enum {"
		for ri,r in enumerate(list) : 
			print "    %s_%s=%d,"%(name,r,self.len())
			self.add_frame(line*16+ri,col);
		print '};'

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
# one per color to achieve 256 frames max per spritesheet
frset = {}

print "enum {"
for c in colors : 
	print "    color_%s,"%c
	frset[c]=Frameset()
print '};'
frset['misc']=Frameset()

print "enum {"
for ln, uni_set in [(0,units), (3,units2)]:
	for ui,u in enumerate(uni_set) : 
		print "    unit_%s = %d,"%(u,frset['blue'].len())
		for c in colors : 
			frset[c].add_frame(ln*16+ui*2     ,color=c) # normal
			frset[c].add_frame(ln*16+ui*2+1   ,color=c) # normal frame 2 
			frset[c].add_frame(ln*16+ui*2     ,color=c, mirror=True) # left
			frset[c].add_frame(ln*16+ui*2+1   ,color=c, mirror=True) # left frame 2 
			frset[c].add_frame((ln+1)*16+ui*2 ,color=c) # down
			frset[c].add_frame((ln+1)*16+ui*2+1,color=c) # down frame 2 
			frset[c].add_frame((ln+2)*16+ui*2,color=c) # up
			frset[c].add_frame((ln+2)*16+ui*2+1,color=c) # up frame 2 
print '};'

print "enum {"
for ai,a in enumerate(cursors) : 
	print "    cursor_%s = %d,"%(a,frset['misc'].len())
	for c in colors : 
		frset['misc'].add_frame(6*16+ai,color=c)
print '};'


# non colored resources
frset['misc'].frline('resource',resources,7)
frset['misc'].frline('tag',tags,8)
frset['misc'].frline('number',range(16),9)
frset['misc'].frline('misc',misc,10)

for k,v in frset.items() : 
	frset[k].write(k+'.png')

# other named tiles
for t in ('zero','tiny'):  
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
# tile as name (FR)
print 'const uint16_t terrain_tiledef[%d]={'%len(terrains)
for t in terrains : 
	elt = tsx.find('tile[@type="%s"]'%t)
	print "    [terrain_%s]=%d, "%(t,(1+int(elt.get('id'))) if elt is not None else 0)
print '};'

# terrain/unit movement cost 
print 'const uint8_t terrain_move_cost[%d][%d]={'%(len(units+units2),len(terrains))
for u in units+units2 : 
	print '  {%s}, // %s'%(','.join(str(speed(u,t)) for t in terrains),u)
print '};'

# map units 
# --------------------------
tmx=ET.parse('map.tmx').getroot()

units_ts = [x for x in tmx.findall('tileset[image]') if x.find('image').get('source')=="units.png"][0]
firstgid_units = int(units_ts.get('firstgid'))
tmap_w = int(tmx.get('width'))
print '// ----'
print '// Units initial positions by level'
print "uint8_t level_units[][4][16][3] = { // N levels, 4 colors, 16 units, {x,y,type 1-16}"
for l in tmx.findall('layer') : 
	if l.get('name').endswith('_units') and l.get('name').startswith('_level'): 
		dat = l.find('data')
		assert dat.get('encoding')=='csv','not a csv file'

		lvl = [int(x) for x in dat.text.replace('\n','').split(',')]
		items = [(n,t-firstgid_units) for n,t in enumerate(lvl) if t]
		# transform into 4 starting elements
		it_color = [[],[],[],[]]
		for id,item in items : 
			color = item/16
			it_color[color].append((id, 1+item%16))
		print '    {'
		for i in it_color : 
			print "        {%s},"%(','.join('{%d,%d,%d}'%(id%tmap_w,id/tmap_w,unit) for id,unit in i))
		print '    },'

print "};"


print '#endif'


