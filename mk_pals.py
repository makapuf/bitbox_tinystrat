"""
Make extra couple palettes from sprites
"""
import xml.etree.ElementTree as ET
from PIL import Image
import sys

DEBUG=False

sys.path.append('sdk/lib/blitter/scripts')
from spr2png import Sprite
from utils import rgba2u8
colors = Image.open('colors.png').load()

# load palette from original sprite
units = Sprite('sprites/units_16x16.spr')
print(len(units.palette),'couples in palette')
# build palettes from original
def fade(c) :
	"fade color : average with darker grey"
	return (c[0]+64)//2, (c[1]+64)//2, (c[2]+64)//2, c[3]

newpal = [[] for i in range(8)] # 8 palettes : colors, faded_colors
for a,b in units.palette : # for each couple colors in initial palette
	for c in range(4) : # columns : target colors , blue, ...
		na=a
		nb=b
		for nuance in range(4) : # line : 4 nuances to test
			if a==colors[0,nuance] : na = colors[c,nuance]
			if b==colors[0,nuance] : nb = colors[c,nuance]

		newpal[c].append(na)
		newpal[c].append(nb)
		newpal[c+4].append(fade(na))
		newpal[c+4].append(fade(nb))

# add one couple to each palette to make them 256-couples
for p in newpal :
	p += ((0,0,0,0),)*(512-len(p))

# debug palettes
if DEBUG :
	dpal = Image.new('RGBA',(512,8))
	dpal.putdata(newpal[0]+newpal[1]+newpal[2]+newpal[3]+newpal[4]+newpal[5]+newpal[6]+newpal[7])
	dpal.save('_debug.png')

# save palettes as colors to .pal file containting 4+4 palettes of 256 couples of 1 byte/color each = 8*256=2k
of = open('palettes.bin','wb')
for p in newpal :
	for c in p :
		u8 = rgba2u8(*c)
		of.write(bytes((u8,)))
