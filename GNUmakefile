NAME := tinystrat

all: data.h
BITBOX:=sdk

COLORS := blue red yellow green
TERRAINS :=  mountains forest town fields stable sea beach \
	castle camp road plain river

DATAFILES := tiles_bg.tset map.map palettes.bin \
	sprites/units_16x16.spr sprites/misc_16x16.spr sprites/faces_26x26.spr \
	sprites/fight_200x200.spr sprites/bignum_16x24.spr sprites/next_player.spr
INTRO := tiny bg horse_left horse_right objects_left objects_right wars 

DATAFILES += $(TERRAINS:%=sprites/bg_%.spr)
DATAFILES += $(INTRO:%=sprites/intro_%.spr)
DATAFILES += sprites/menus_88x82.spr sprites/main_menu.spr 


GAME_C_FILES = main.c faces.c pathfinding.c \
	lib/blitter/blitter.c \
	lib/blitter/blitter_tmap.c \
	lib/blitter/blitter_sprites3.c 

DEFINES = VGA_MODE=400 VGA_BPP=16

# graphical scripts path
GRSCRIPTS = sdk/lib/blitter/scripts

main.c: data.h defs.h 

-include sdk/kernel/bitbox.mk

data.h: $(DATAFILES)
	mkdir -p $(@D)
	sdk/lib/resources/embed.py $^ > $@

%.h %.tset : %.tsx %.png
	$(GRSCRIPTS)/mk_tset.py $< > $*.h

%.h %.map : %.tmx
	$(GRSCRIPTS)/mk_tmap.py -f u16 $< > $*.h

%.spr : %.png
	$(GRSCRIPTS)/mk_spr.py $< -p COUPLES -o $*.spr

palettes.bin : sprites/units_16x16.spr
	python mk_pals.py 

defs.h: mk_defs.py tiles_bg.tsx map.tmx
	python mk_defs.py > $@

%.lz4 : %
	lz4 -f -9 --content-size --no-frame-crc --no-sparse $^ $@

clean::
	rm -f $(DATAFILES) _debug.png 
	rm -f defs.h tiles_bg.h data.h map.h $(NAME)_sdl palettes.bin

