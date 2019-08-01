NAME := tinystrat

all: data.h

COLORS := blue red yellow green
TERRAINS :=  mountains forest town fields stable sea beach \
	castle camp road plain river
INTRO := tiny bg horse_left horse_right objects_left objects_right wars

SPRITES := units_16x16 misc_16x16 faces_26x26 fight_200x200 bignum_16x24 \
	next_player main_menu menu_border text_border \
	$(TERRAINS:%=bg_%) $(INTRO:%=intro_%)

BINARY_FILES := tiles_bg.tset map.map palettes.bin song.mod font.fon font_mini.fon\
	$(SPRITES:%=sprites/%.spr)

GAME_C_FILES = main.cpp pathfinding.cpp player.cpp game.cpp grid.cpp ai.cpp combat.cpp unit.cpp\
	implems.c \
	sdk/lib/blitter/blitter.c \
	sdk/lib/blitter/blitter_tmap.c \
	sdk/lib/blitter/blitter_sprites3.c \
	sdk/lib/blitter/blitter_surface.c \
	sdk/lib/mod/mod32.c

DEFINES = VGA_MODE=400 VGA_BPP=8 MOD_CHANNELS=6

# graphical scripts path
GRSCRIPTS = sdk/lib/blitter/scripts

main.cpp: data.h defs.h

data.h: $(BINARY_FILES)
	sdk/lib/resources/embed.py  $^ -r spr map tset> $@

-include sdk/kernel/bitbox.mk

font_mini.fon: $(GRSCRIPTS)/font_mini.png
	python3 $(GRSCRIPTS)/mk_font.py $^ $@

%.fon: %.png
	python3 $(GRSCRIPTS)/mk_font.py $^ $*.fon

%.h %.tset : %.tsx %.png
	$(GRSCRIPTS)/mk_tset.py $< -p MICRO > $*.h

%.h %.map : %.tmx
	$(GRSCRIPTS)/mk_tmap.py $< > $*.h

%.spr : %.png
	$(GRSCRIPTS)/mk_spr.py $< -p MICRO -o $*.spr

# Those need couples palettes
sprites/units_16x16.spr: sprites/units_16x16.png
	$(GRSCRIPTS)/mk_spr.py $< -p COUPLES -o $@

sprites/intro_bg.spr: sprites/intro_bg.png
	$(GRSCRIPTS)/mk_spr.py $< -p COUPLES -o $@ 

sprites/main_menu.spr: sprites/main_menu.png
	$(GRSCRIPTS)/mk_spr.py $< -p COUPLES -o $@ 

palettes.bin : sprites/units_16x16.spr
	python3 mk_pals.py

defs.h: mk_defs.py tiles_bg.tsx map.tmx
	python3 mk_defs.py > $@

%.lz4 : %
	lz4 -f -9 --content-size --no-frame-crc --no-sparse $^ $@

clean::
	rm -f defs.h $(NAME)_sdl

clean_assets:
	rm -f tiles_bg.tset tiles_bg.h map.map map.h palettes.bin _debug.png *.fon
	rm -f $(SPRITES:%=sprites/%.spr)
	rm -f palettes.bin data.h
