/* blitter object : 2bpp surface, non clipped.

 layout : data : 4x4 couple palette, from 2bpp pixels


 */
#include "surface.h"
extern "C" {
	static void surface_drawline (struct object *o);
}

Surface::Surface (int _w, int _h, void *_data)
{
	w=_w; h=_h; data = _data;
	line = surface_drawline;
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
static void surface_drawline (struct object *o)
{
	couple_t *palette = (couple_t *) o->data;

	// restrict if C
	uint8_t  * src = (uint8_t*) o->data + 16*sizeof(couple_t) + ((vga_line-o->y)*o->w) / 4;
	couple_t * start = (couple_t*) draw_buffer + o->x/2 ;
	couple_t * end   = (couple_t*) draw_buffer + (o->x+o->w)/2;

	// avoid palette fetch if same src
	for (couple_t *dst = start; dst < end; src++, dst+=2) {
	 	const couple_t c1 = palette[(*src>>0) & 0x3] << (sizeof(pixel_t)*8) | palette[(*src>>2) & 0x3];
	 	const couple_t c2 = palette[(*src>>4) & 0x3] << (sizeof(pixel_t)*8) | palette[(*src>>6) & 0x3];
	 	dst[0] = c1;
		dst[1] = c2;
	}
}


// color between 0 and 3
void Surface::fillrect (int x1, int y1, int x2, int y2, uint8_t color)
{
	const uint32_t wc = (color & 3)*0x55555555; // repeat 16 times -> no line not multiple of 16 !
	uint32_t *p = (uint32_t *)data + 16*sizeof(couple_t);

	for (int y=y1; y<y2;y++) {
		for (int i=x1/16;i<x2/16;i++) p[i+y*w/16] = wc;

		/*
		// set last bits of word
		const int nbits = (x1%16) * 2;
		p[x1/16] = (p[x1/16] & (0xffffffff << nbits)) | (wc >> (32-nbits));

		for (int i=x1/16+1;i<x2/16;i++)
			p[i] = wc;

		// set first bits of word
		const int nbits2 = (x2%16) * 2;
		p[x2/16] = (p[x2/16] & (0xffffffff >> nbits2)) | (wc << (32-nbits2));
		*/
	}
}