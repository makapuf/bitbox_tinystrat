/* blitter object : 2bpp surface, non clipped.

 layout : data : 4x4 interleaved couples palette (aa,ab,ac,ad,ba,...) , then 2bpp pixels

 */
#include "surface.h"

Surface::Surface (int _w, int _h, void *_data)
{
	w=_w; h=_h; data = _data;
	line = drawline;
	frame=nullptr;
}

void Surface::setpalette (pixel_t *pal)
{
	couple_t *p = (couple_t*) data;
	for (int i=0;i<4;i++)
		for (int j=0;j<4;j++) {
			p[j+4*i] = pal[i] << 16 | pal[j];
		}
}

// no H zoom, not clipped yet
void Surface::drawline (struct object *o)
{
	couple_t *palette = (couple_t *) o->data;

	// would use restrict if C
	uint8_t  * src = (uint8_t*) o->data + 16*sizeof(couple_t) + ((vga_line-o->y)*o->w) / 4;
	couple_t * start = (couple_t*) draw_buffer + o->x/2 ;
	couple_t * end   = (couple_t*) draw_buffer + (o->x+o->w)/2;

	uint8_t oldsrc = *src+1; // not src
	couple_t c1,c2;
	for (couple_t *dst = start; dst < end; src++, dst+=2) {
		if (*src != oldsrc) { // avoid palette fetch if same src
		 	c1 = palette[*src & 0xf];
		 	c2 = palette[*src >> 4];
		 	oldsrc = *src;
		}
	 	dst[0] = c1;
		dst[1] = c2;
	}
}

// color between 0 and 3
void Surface::fillrect (int x1, int y1, int x2, int y2, uint8_t color)
{
	const uint32_t wc = (color & 3)*0x55555555; // repeat 16 times -> no line not multiple of 16 !
	uint32_t *p = (uint32_t *)data + 4*sizeof(couple_t) + w/16*y1; // start of line, in words

	for (int y=y1; y<y2;y++) {

		// set last bits of word
		const int nbits = (x1%16) * 2;
		p[x1/16] = (p[x1/16] & (0xffffffff >> (32-nbits))) | (wc << nbits);

		// fill whole words
		for (int i=x1/16+1;i<x2/16;i++) p[i] = wc;
		//message("%d %x %x\n",y,wc,(0xffffffff >> (32-nbits)));

		// set first bits of word
		const int nbits2 = (x2%16) * 2;
		p[x2/16] = (p[x2/16] & (0xffffffff << nbits2)) | (wc >> (32-nbits2));

		p += w/16; // next line
	}
}

struct Font {
	uint8_t height, bytes_per_line;
	uint8_t char_width [128]; // width in pixel of each character.
	uint8_t data[]; // 2bpp, integer number of bytes by line of character
};

int Surface::chr (const char c, int x, int y, const void *fontdata)
{
	const Font* font = (const Font*) fontdata;
	uint8_t *p = (uint8_t *)data + 16*sizeof(couple_t); // buffer start

	const uint8_t ch = (uint8_t)c-' ';
	const int cw = font->char_width[ch];
	for (int j=0;j<font->height;j++) {
		const uint8_t *cp = &font->data[(ch*font->height + j)*font->bytes_per_line]; // source word address
		uint32_t *dst = (uint32_t *)&p[(w/4)*(y+j)+x/4]; // existing word, byte aligned
		uint32_t pw = *dst; // read existing pixels
		// pw &= 0xffffffff << (cw*2); // fixme mask it or not if transparent render
		pw |= *(uint32_t *)cp << ((x%4)*2); // read 32 bits, shift them right
		*dst = pw; // write it back
	}
	return cw;
}

// opaque text, non wrapped, non clipped.
void Surface::text (const char *text, int x, int y,const void *fontdata)
{
	const Font* font = (const Font*) fontdata;
	int cx = x; // current X
	uint8_t *p = (uint8_t *)data + 16*sizeof(couple_t); // buffer start

	for (const char*c=text ; *c;c++) {
		if (*c =='\n') {
			y += font->height+1;
			cx = x;
		} else {
			cx += chr(*c, cx, y, fontdata) +1 ;
		}
	}
}
