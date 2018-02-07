NAME := tinystrat

all: data.h

COLORS := blue red yellow green
TERRAINS :=  mountains forest town fields stable sea beach \
	castle camp road plain river	
INTRO := tiny bg horse_left horse_right objects_left objects_right wars 

SPRITES := units_16x16 misc_16x16 faces_26x26 fight_200x200 bignum_16x24 \
	next_player menus_88x82 main_menu \
	$(TERRAINS:%=bg_%) $(INTRO:%=intro_%)

DATAFILES := tiles_bg.tset map.map palettes.bin music/song.mod \
	$(SPRITES:%=sprites/%.spr)

GAME_C_FILES = main.c pathfinding.c player.c grid.c ai_0.c sfx.c \
	sdk/lib/blitter/blitter.c \
	sdk/lib/blitter/blitter_tmap.c \
	sdk/lib/blitter/blitter_sprites3.c \
	sdk/lib/mod/mod32.c 

DEFINES = VGA_MODE=400 VGA_BPP=16 MOD_CHANNELS=6

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
	python3 mk_pals.py

defs.h: mk_defs.py tiles_bg.tsx map.tmx
	python3 mk_defs.py > $@

%.lz4 : %
	lz4 -f -9 --content-size --no-frame-crc --no-sparse $^ $@

clean::
	rm -f defs.h $(NAME)_sdl 

clean_assets: 
	rm -f tiles_bg.tset tiles_bg.h map.map map.h palettes.bin _debug.png 
	rm -f $(SPRITES:%=sprites/%.spr)
	rm -f palettes.bin data.h
