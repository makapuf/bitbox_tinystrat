#pragma once
/* blitter object : 2bpp surface, non clipped.

 layout : data : 4x4 couple palette, from 2bpp pixels
 */
extern "C" {
#include "lib/blitter/blitter.h"
}

#define SURFACE_BUFSZ(w,h) ( (w+15)/16*4*h + 16*sizeof(couple_t) )

struct Surface : public object
{
	Surface(int w,int h,void *data);
	void setpalette (pixel_t *pal);
	// color between 0 and 3
	void fillrect (int x1, int y1, int x2, int y2, uint8_t color);
	// draw a single char, return its width
	int chr (const char c, int x, int y, const void *fontdata);

	/**
  \brief draws a text on a surface
  \param surface the surface to draw on
  \param text the text to draw
  \param x,y the coordinates of the text on the surface. must be even.
  \param basecolor : the first color to draw with, will use the 4 next colors.

  Beware, this implementation will not do clipping (yet)
 */
	void text(const char *text, int x, int y, const void *font);

private:
	static void drawline (struct object *o);
};
