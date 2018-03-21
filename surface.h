#pragma once
/* blitter object : 2bpp surface, non clipped.

 layout : data : 4x4 couple palette, from 2bpp pixels
 */
extern "C" {
#include "lib/blitter/blitter.h"
}

#define SURFACE_BUFSZ(w,h) ( (w+3)/4*h + 16*sizeof(couple_t) )

struct Surface : public object
{
	Surface(int w,int h,void *data);
	void setpalette (pixel_t *pal);
	// color between 0 and 3
	void fillrect (int x1, int y1, int x2, int y2, uint8_t color);
};
