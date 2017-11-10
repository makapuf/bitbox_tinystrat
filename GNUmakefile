NAME:=tinystrat

all: data.h
BITBOX:=sdk

COLORS := blue red yellow green
DATAFILES := tiles_bg.tset map.map units_16x16.spr misc_16x16.spr palettes.bin faces_26x26.spr
DATAFILES += intro_tiny.spr \
	intro_bg.spr \
	intro_horse_left.spr \
	intro_horse_right.spr \
	intro_objects_left.spr \
	intro_objects_right.spr \
	intro_wars.spr \
	bg_plain.spr \
	bg_town.spr \
	bg_forest.spr

GAME_C_FILES = main.c \
	lib/blitter/blitter.c \
	lib/blitter/blitter_tmap.c \
	lib/blitter/blitter_sprites3.c 

DEFINES = VGA_MODE=400 VGA_BPP=16

# graphical scripts path
GRSCRIPTS = sdk/lib/blitter/scripts

main.c: data.h tinystrat_defs.h 

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

palettes.bin : units_16x16.spr
	python mk_pals.py 

$(NAME)_defs.h: tinystrat.py tiles_bg.tsx map.tmx
	python tinystrat.py > $(NAME)_defs.h

%.lz4 : %
	lz4 -f -9 --content-size --no-frame-crc --no-sparse $^ $@

clean::
	rm -f $(DATAFILES) _debug.png 
	rm -f $(NAME)_defs.h tiles_bg.h data.h map.h $(NAME)_sdl

