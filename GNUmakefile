NAME:=tinystrat

all: data.h

COLORS := blue red yellow green
DATAFILES := tiles_bg.tset map.map $(COLORS:%=%.spr) misc.spr wars.spr 
DATAFILES += intro_tiny.spr intro_bg.spr intro_horse_left.spr intro_horse_right.spr intro_objects_left.spr intro_objects_right.spr intro_wars.spr
GAME_C_FILES = main.c lib/blitter/blitter.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprites3.c 

DEFINES = VGA_MODE=400 VGA_BPP=16

main.c: data.h

-include $(BITBOX)/kernel/bitbox.mk

data.h: $(DATAFILES)
	mkdir -p $(@D)
	$(BITBOX)/tools/embed.py $^ > $@

%.h %.tset : %.tsx %.png
	$(BITBOX)/tools/mk_tset.py $< > $*.h

%.h %.map : %.tmx
	$(BITBOX)/tools/mk_tmap.py -f u16 $< > $*.h

%.spr : %.png
	$(BITBOX)/tools/mk_spr.py $< --min_match=800 -p COUPLES -o $*.spr -s 16x16 

intro_%.spr : intro_%.png
	$(BITBOX)/tools/mk_spr.py $^ --min_match=800 -p COUPLES -o $@

wars.spr : wars.png
	$(BITBOX)/tools/mk_spr.py $^ --min_match=800 -p COUPLES -o $@ 

$(NAME)_defs.h blue.png red.png yellow.png green.png misc.png: tinystrat.py tiles_bg.tsx tiles_bg.png map.tmx
	python tinystrat.py > $(NAME)_defs.h

%.lz4 : %
	lz4 -f -9 --content-size --no-frame-crc --no-sparse $^ $@

clean::
	rm -f palette.png $(DATAFILES) $(COLORS:%=%.png) misc.png _debug.png
	rm -f $(NAME)_defs.h tiles_bg.h data.h map.h $(NAME)_sdl

