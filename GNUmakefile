NAME:=tinystrat

all: data.h

COLORS := blue red yellow green
DATAFILES := tiles_bg.tset map.map $(COLORS:%=%.spr) misc.spr wars.spr tiny.spr
GAME_C_FILES = main.c lib/blitter/blitter.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprites3.c 

DEFINES = VGA_MODE=400 VGA_BPP=16

main.c: data.h

-include $(BITBOX)/kernel/bitbox.mk

data.h: $(DATAFILES)
	mkdir -p $(@D)
	python2 $(BITBOX)/tools/embed.py $^ > $@

%.h %.tset : %.tsx
	$(BITBOX)/tools/mk_tset.py $< > $*.h

%.h %.map : %.tmx
	$(BITBOX)/tools/mk_tmap.py -f u16 $< > $*.h

%.spr : %.png
	$(BITBOX)/tools/mk_spr.py $< -o $*.spr -s 16x16 

wars.spr : wars.png
	$(BITBOX)/tools/mk_spr.py $< -o $@

$(NAME)_defs.h blue.png red.png yellow.png green.png misc.png: tinystrat.py tiles_bg.tsx tiles_bg.png
	python tinystrat.py > $(NAME)_defs.h

%.lz4 : %
	lz4 -f -9 --content-size --no-frame-crc --no-sparse $^ $@

clean::
	rm -f palette.png $(DATAFILES) $(COLORS:%=%.png) misc.png _debug.png
	rm -f $(NAME)_defs.h tiles_bg.h data.h map.h $(NAME)_sdl

