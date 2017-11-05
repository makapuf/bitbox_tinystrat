"""
Make extra couple palettes from sprites
"""
import xml.etree.ElementTree as ET
from PIL import Image
import sys

sys.path.append('sdk/lib/blitter/scripts')
from spr2png import Sprite
from utils import rgba2u16

colors = Image.open('colors.png').load()

# load palette from original sprite
units = Sprite('units.spr')

# build palettes from original
newpal = [[],[],[],[],[],[],[],[]] # 8 palettes : colors, faded_colors
def fade(c) : 
	"fade color : average with grey"
	return c[0]+64//2, c[1]+64//2, c[2]+64//2, c[3]

for a,b in units.palette : 
	for c in range(4) : # blue, ...
		for i in range(4) : # line
			if a==colors[0,i] : a = colors[c,i]
			if b==colors[0,i] : b = colors[c,i]
		newpal[c].append(a)
		newpal[c].append(b)

		newpal[c+4].append(fade(a))
		newpal[c+4].append(fade(b))

# add one couple to each palette to make them 256-colors
for p in newpal : 
	p.append((0,0,0,0))
	p.append((0,0,0,0))

# save palettes as colors to .pal file containting 4+4 palettes of 256 couples of 2 bytes/color each.
of = open('palettes.bin','wb')
for p in newpal : 
	for c in p :
		u16 = rgba2u16(*c)
		of.write(chr(u16&0xff)+chr(u16>>8))

