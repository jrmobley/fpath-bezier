
#include "fpath.h"
#include <stdlib.h>

/*
 * Credit where credit is due:
 *
 * The functions fceil, floorDivMod, edge_init, and edge_step
 * are derived from Chris Hecker's "Perspective Texture Mapping"
 * series of articles in Game Developer Magazine (1995).  See
 * http://chrishecker.com/Miscellaneous_Technical_Articles
 *
 * The functions fpath_plot_edge_aa and fpath_end_fill_aa are derived
 * from:
 *   Scanline edge-flag algorithm for antialiasing
 *   Copyright (c) 2005-2007 Kiia Kallio <kkallio@uiah.fi>
 *   http://mlab.uiah.fi/~kkallio/antialiasing/
 *
 * The Edge Flag algorithm as used in both the black & white and
 * antialiased rendering functions here was presented by Bryan D. Ackland
 * and Neil H. Weste in "The Edge Flag Algorithm-A Fill Method for
 * Raster Scan Displays" (January 1981).
 *
 * The function countBits is Brian Kernighan's alorithm as presented
 * on Sean Eron Anderson's Bit Twiddling Hacks page at
 * http://graphics.stanford.edu/~seander/bithacks.html
 *
 */

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

// --------------------------------------------------------------------------
// FPath drawing support that is shared between bw and aa.
// --------------------------------------------------------------------------

bool fpoint_equal(const FPoint * const point_a, const FPoint * const point_b) {
	return point_a->x == point_b->x && point_a->y == point_b->y;
}

void fpath_destroy(FPath* fpath) {
	free(fpath);
}

void fpath_rotate_to(FPath* fpath, int32_t angle) {
	fpath->rotation = angle;
}

void fpath_move_to(FPath* fpath, FPoint point) {
	fpath->offset = point;
}

void floorDivMod(int32_t numerator, int32_t denominator, int32_t* floor, int32_t* mod ) {
	Assert(denominator > 0); // we assume it's positive
	if (numerator >= 0) {
		// positive case, C is okay
		*floor = numerator / denominator;
		*mod = numerator % denominator;
	} else {
		// numerator is negative, do the right thing
		*floor = -((-numerator) / denominator);
		*mod = (-numerator) % denominator;
		if (*mod) {
			// there is a remainder
			--*floor;
			*mod = denominator - *mod;
		}
	}
}

typedef struct Edge {
	int32_t x;
	int32_t xStep;
	int32_t numerator;
	int32_t denominator;
	int32_t errorTerm; // DDA info for x
	int32_t y;         // current y
	int32_t height;    // vertical count
} Edge;

int32_t edge_step(Edge* e) {
	e->x += e->xStep;
	++e->y;
	--e->height;
	
	e->errorTerm += e->numerator;
	if (e->errorTerm >= e->denominator) {
		++e->x;
		e->errorTerm -= e->denominator;
	}
	return e->height;
}

void fpath_set_stroke_color(FContext* fctx, GColor c) {
	fctx->strokeColor = c;
#ifdef PBL_COLOR
	fctx->aarampDirty = true;
#endif
}

void fpath_set_fill_color(FContext* fctx, GColor c) {
	fctx->fillColor = c;
#ifdef PBL_COLOR
	fctx->aarampDirty = true;
#endif
}

// --------------------------------------------------------------------------
// BW - black and white drawing with 1 bit-per-pixel flag buffer.
// --------------------------------------------------------------------------

int32_t fceil(fixed_t value) {
	int32_t returnValue;
	int32_t numerator = value - 1 + FIXED_POINT_SCALE;
	if (numerator >= 0) {
		returnValue = numerator / FIXED_POINT_SCALE;
	} else {
		// deal with negative numerators correctly
		returnValue = -((-numerator) / FIXED_POINT_SCALE);
		returnValue -= ((-numerator) % FIXED_POINT_SCALE) ? 1 : 0;
	}
	return returnValue;
}

void edge_init(Edge* e, FPoint* top, FPoint* bottom) {
	
	e->y = fceil(top->y);
	int32_t yEnd = fceil(bottom->y);
	e->height = yEnd - e->y;
	if (e->height)	{
		int32_t dN = bottom->y - top->y;
		int32_t dM = bottom->x - top->x;
		int32_t initialNumerator = dM * 16 * e->y - dM * top->y +
		dN * top->x - 1 + dN * 16;
		floorDivMod(initialNumerator, dN*16, &e->x, &e->errorTerm);
		floorDivMod(dM*16, dN*16, &e->xStep, &e->numerator);
		e->denominator = dN*16;
	}
}

void fpath_init_context_bw(FContext* fctx, GContext* gctx) {
	
	GBitmap* frameBuffer = graphics_capture_frame_buffer(gctx);
	if (frameBuffer) {
		GRect bounds = gbitmap_get_bounds(frameBuffer);
		graphics_release_frame_buffer(gctx, frameBuffer);
		
		bounds.size.w += 1;
		bounds.size.h += 1;
		fctx->flagBuffer = gbitmap_create_blank(bounds.size, GBitmapFormat1Bit);

		fctx->gctx = gctx;
	}
}

void fpath_begin_fill_bw(FContext* fctx) {
	
	GRect bounds = gbitmap_get_bounds(fctx->flagBuffer);
	fctx->max.x = INT_TO_FIXED(bounds.origin.x);
	fctx->max.y = INT_TO_FIXED(bounds.origin.y);
	fctx->min.x = INT_TO_FIXED(bounds.origin.x + bounds.size.w);
	fctx->min.y = INT_TO_FIXED(bounds.origin.y + bounds.size.h);
	}

void fpath_plot_edge_bw(FContext* fctx, FPoint* a, FPoint* b) {
	
	Edge edge;
	if (a->y > b->y) {
		edge_init(&edge, b, a);
	} else {
		edge_init(&edge, a, b);
	}
	
	uint8_t* data = gbitmap_get_data(fctx->flagBuffer);
	int16_t stride = gbitmap_get_bytes_per_row(fctx->flagBuffer);
	int32_t height = edge.height;
	while (height--) {
		uint8_t* p = data + edge.y * stride + edge.x / 8;
		uint8_t mask = 1 << (edge.x % 8);
		*p ^= mask;
		edge_step(&edge);
	}

}

void fpath_draw_filled_bw(FContext* fctx, FPath* fpath) {
	
	// allocate buffer for transformed points.
	FPoint* points = (FPoint*)malloc(fpath->num_points * sizeof(FPoint));
	if (points) {
		
		// half-pixel offset
		int32_t adjust = -FIXED_POINT_SCALE / 2;

		// rotate and translate the points.
		FPoint* src = fpath->points;
		FPoint* end = src + fpath->num_points;
		FPoint* dest = points;
		int32_t c = cos_lookup(fpath->rotation);
		int32_t s = sin_lookup(fpath->rotation);
		while (src != end) {
			dest->x = (src->x * c / TRIG_MAX_RATIO) - (src->y * s / TRIG_MAX_RATIO);
			dest->y = (src->x * s / TRIG_MAX_RATIO) + (src->y * c / TRIG_MAX_RATIO);
			dest->x += fpath->offset.x + adjust;
			dest->y += fpath->offset.y + adjust;
			
			// grow a bounding box around the points visited.
			if (dest->x < fctx->min.x) fctx->min.x = dest->x;
			if (dest->y < fctx->min.y) fctx->min.y = dest->y;
			if (dest->x > fctx->max.x) fctx->max.x = dest->x;
			if (dest->y > fctx->max.y) fctx->max.y = dest->y;
			
			++src;
			++dest;
		}
		
		// rasterize the edges into the buffer
		for (uint32_t k = 0; k < fpath->num_points; ++k) {
			fpath_plot_edge_bw(fctx, points+k, points+((k+1) % fpath->num_points));
		}
		
		free(points);
	}

}

void fpath_end_fill_bw(FContext* fctx) {
	
#ifdef PBL_COLOR
	uint8_t color = fctx->fillColor.argb;
#else
	uint8_t color = gcolor_equal(fctx->fillColor, GColorWhite) ? 0xff : 0x00;
#endif

	uint16_t rowBegin = FIXED_TO_INT(fctx->min.y);
	uint16_t rowEnd   = FIXED_TO_INT(fctx->max.y) + 1;
	uint16_t colBegin = FIXED_TO_INT(fctx->min.x);
	uint16_t colEnd   = FIXED_TO_INT(fctx->max.x) + 1;
	
	GBitmap* fb = graphics_capture_frame_buffer(fctx->gctx);
	uint8_t* fbData = gbitmap_get_data(fb);
	uint16_t fbStride = gbitmap_get_bytes_per_row(fb);
	
	uint8_t* data = gbitmap_get_data(fctx->flagBuffer);
	uint16_t stride = gbitmap_get_bytes_per_row(fctx->flagBuffer);
	
	uint8_t* dest;
	uint8_t* src;
	uint8_t mask;
	uint16_t col, row;
	
	for (row = rowBegin; row < rowEnd; ++row) {

		bool inside = false;
		for (col = colBegin; col <= colEnd; ++col) {
			
#ifdef PBL_COLOR
			dest = fbData + fbStride * row + col;
#else
			dest = fbData + fbStride * row + col / 8;
#endif
			src = data + stride * row + col / 8;
			mask = 1 << (col % 8);
			if (*src & mask) {
				inside = !inside;
			}
			*src &= ~mask;
			if (inside) {
#ifdef PBL_COLOR
				*dest = color;
#else
				*dest = (color & mask) | (*dest & ~mask);
#endif
			}
		}
	}
	
	graphics_release_frame_buffer(fctx->gctx, fb);

}

void fpath_deinit_context_bw(FContext* fctx) {
	if (fctx->gctx) {
		gbitmap_destroy(fctx->flagBuffer);
		fctx->gctx = NULL;
	}
}

// --------------------------------------------------------------------------
// AA - anti-aliased drawing with 8 bit-per-pixel flag buffer.
// --------------------------------------------------------------------------

#ifdef PBL_COLOR

#define SUBPIXEL_COUNT 8
#define SUBPIXEL_SHIFT 3

#define FIXED_POINT_SHIFT_AA 1
#define FIXED_POINT_SCALE_AA 2
#define INT_TO_FIXED_AA(a) ((a) * FIXED_POINT_SCALE)
#define FIXED_TO_INT_AA(a) ((a) / FIXED_POINT_SCALE)
#define FIXED_MULTIPLY_AA(a, b) (((a) * (b)) / FIXED_POINT_SCALE_AA)

int32_t fceil_aa(fixed_t value) {
	int32_t returnValue;
	int32_t numerator = value - 1 + FIXED_POINT_SCALE_AA;
	if (numerator >= 0) {
		returnValue = numerator / FIXED_POINT_SCALE_AA;
	} else {
		// deal with negative numerators correctly
		returnValue = -((-numerator) / FIXED_POINT_SCALE_AA);
		returnValue -= ((-numerator) % FIXED_POINT_SCALE_AA) ? 1 : 0;
	}
	return returnValue;
}

void fpath_calc_ramp_aa(FContext* fctx) {
	
	GColor c0 = fctx->strokeColor;
	GColor c1 = fctx->fillColor;
	GColor c;
	
	fctx->aaramp[0] = c0;
	for (int16_t k = 1; k < 8; ++k) {
		c.r = ((int16_t)c0.r * 8 + ((int16_t)c1.r - c0.r) * k + 4) / 8;
		c.g = ((int16_t)c0.g * 8 + ((int16_t)c1.g - c0.g) * k + 4) / 8;
		c.b = ((int16_t)c0.b * 8 + ((int16_t)c1.b - c0.b) * k + 4) / 8;
		c.a = 3;
		fctx->aaramp[k] = c;
	}
	fctx->aaramp[8] = c1;
	fctx->aarampDirty = false;
}

/*
 * FPoint is at a scale factor of 16.  The anti-aliased scan conversion needs
 * to address 8x8 subpixels, so if we treat the FPoint coordinates as having
 * a scale factor of 2, then we should scan in sub-pixel coordinates, with
 * sub-sub-pixel correct endpoints!  Fukn shweet.
 */
void edge_init_aa(Edge* e, FPoint* top, FPoint* bottom) {
	static const int32_t F = 2;
	e->y = fceil_aa(top->y);
	int32_t yEnd = fceil_aa(bottom->y);
	e->height = yEnd - e->y;
	if (e->height)	{
		int32_t dN = bottom->y - top->y;
		int32_t dM = bottom->x - top->x;
		int32_t initialNumerator = dM * F * e->y - dM * top->y +
		dN * top->x - 1 + dN * F;
		floorDivMod(initialNumerator, dN*F, &e->x, &e->errorTerm);
		floorDivMod(dM*F, dN*F, &e->xStep, &e->numerator);
		e->denominator = dN*F;
	}
}

void fpath_init_context_aa(FContext* fctx, GContext* gctx) {
	
	GBitmap* frameBuffer = graphics_capture_frame_buffer(gctx);
	if (frameBuffer) {
		GRect bounds = gbitmap_get_bounds(frameBuffer);
		graphics_release_frame_buffer(gctx, frameBuffer);
		bounds.size.w += 1;
		bounds.size.h += 1;
		fctx->gctx = gctx;
		fctx->flagBuffer = gbitmap_create_blank(bounds.size, GBitmapFormat8Bit);
		fctx->strokeColor = GColorBlack;
		fctx->fillColor = GColorWhite;
		fctx->aarampDirty = true;
	}
}

void fpath_plot_edge_aa(FContext* fctx, FPoint* a, FPoint* b) {

	static const int32_t offsets[SUBPIXEL_COUNT] = {
		2, 7, 4, 1, 6, 3, 0, 5 // 1/8ths
	};
	
	Edge edge;
	if (a->y > b->y) {
		edge_init_aa(&edge, b, a);
	} else {
		edge_init_aa(&edge, a, b);
	}
	
	uint8_t* data = gbitmap_get_data(fctx->flagBuffer);
	int16_t stride = gbitmap_get_bytes_per_row(fctx->flagBuffer);
	int32_t height = edge.height;
	while (height--) {
		int32_t ySub = edge.y & (SUBPIXEL_COUNT - 1);
		uint8_t mask = 1 << ySub;
		int32_t pixelX = (edge.x + offsets[ySub]) / SUBPIXEL_COUNT;
		int32_t pixelY = edge.y / SUBPIXEL_COUNT;
		
		uint8_t* p = data + pixelY * stride + pixelX;
		*p ^= mask;

		edge_step(&edge);
	}
}

void fpath_draw_filled_aa(FContext* fctx, FPath* fpath) {
	
	// allocate buffer for transformed points.
	FPoint* points = (FPoint*)malloc(fpath->num_points * sizeof(FPoint));
	if (points) {
		
		// offset by half of a subpixel.
		int32_t adjust = -1;
		
		// rotate and translate the points.
		FPoint* src = fpath->points;
		FPoint* end = src + fpath->num_points;
		FPoint* dest = points;
		int32_t c = cos_lookup(fpath->rotation);
		int32_t s = sin_lookup(fpath->rotation);
		while (src != end) {
			dest->x = (src->x * c / TRIG_MAX_RATIO) - (src->y * s / TRIG_MAX_RATIO);
			dest->y = (src->x * s / TRIG_MAX_RATIO) + (src->y * c / TRIG_MAX_RATIO);
			dest->x += fpath->offset.x + adjust;
			dest->y += fpath->offset.y + adjust;
			
			// grow a bounding box around the points visited.
			if (dest->x < fctx->min.x) fctx->min.x = dest->x;
			if (dest->y < fctx->min.y) fctx->min.y = dest->y;
			if (dest->x > fctx->max.x) fctx->max.x = dest->x;
			if (dest->y > fctx->max.y) fctx->max.y = dest->y;
			
			++src;
			++dest;
		}
		
		// rasterize the edges into the buffer
		for (uint32_t k = 0; k < fpath->num_points; ++k) {
			fpath_plot_edge_aa(fctx, points+k, points+((k+1) % fpath->num_points));
		}
		
		free(points);
	}
}

// count the number of bits set in v
uint8_t countBits(uint8_t v) {
	unsigned int c; // c accumulates the total bits set in v
	for (c = 0; v; c++)	{
		v &= v - 1; // clear the least significant bit set
	}
	return c;
}

void fpath_end_fill_aa(FContext* fctx) {
	
	if (fctx->aarampDirty) {
		fpath_calc_ramp_aa(fctx);
	}
	
	uint16_t rowBegin = FIXED_TO_INT(fctx->min.y);
	uint16_t rowEnd   = FIXED_TO_INT(fctx->max.y) + 2;
	uint16_t colBegin = FIXED_TO_INT(fctx->min.x);
	uint16_t colEnd   = FIXED_TO_INT(fctx->max.x) + 2;
	
	GBitmap* fb = graphics_capture_frame_buffer(fctx->gctx);
	uint8_t* fbData = gbitmap_get_data(fb);
	uint16_t fbStride = gbitmap_get_bytes_per_row(fb);
	uint16_t fbStep = fbStride - (colEnd - colBegin);
	
	uint8_t* data = gbitmap_get_data(fctx->flagBuffer);
	uint16_t stride = gbitmap_get_bytes_per_row(fctx->flagBuffer);
	uint16_t step = stride - (colEnd - colBegin);
	
	uint8_t* dest = fbData + rowBegin * fbStride + colBegin;
	uint8_t* src = data + rowBegin * stride + colBegin;
	uint16_t col, row;
	
	for (row = rowBegin; row < rowEnd; ++row, dest += fbStep, src += step) {
		uint8_t mask = 0;
		for (col = colBegin; col < colEnd; ++col, ++dest, ++src) {

			mask ^= *src;
			*src = 0;
			uint8_t coverage = countBits(mask);
			
			if (coverage >  0) {
				*dest = fctx->aaramp[coverage].argb;
			}
		}
	}
	
	graphics_release_frame_buffer(fctx->gctx, fb);

}

// Initialize for Anti-Aliased rendering.
fpath_init_context_func   fpath_init_context   = &fpath_init_context_aa;
fpath_begin_fill_func     fpath_begin_fill     = &fpath_begin_fill_bw;     // note bw
fpath_draw_filled_func    fpath_draw_filled    = &fpath_draw_filled_aa;
fpath_end_fill_func       fpath_end_fill       = &fpath_end_fill_aa;
fpath_deinit_context_func fpath_deinit_context = &fpath_deinit_context_bw; // note bw

void fpath_enable_aa(bool enable) {
	if (enable) {
		fpath_init_context   = &fpath_init_context_aa;
		fpath_begin_fill     = &fpath_begin_fill_bw;     // note bw
		fpath_draw_filled    = &fpath_draw_filled_aa;
		fpath_end_fill       = &fpath_end_fill_aa;
		fpath_deinit_context = &fpath_deinit_context_bw; // note bw
	} else {
		fpath_init_context   = &fpath_init_context_bw;
		fpath_begin_fill     = &fpath_begin_fill_bw;
		fpath_draw_filled    = &fpath_draw_filled_bw;
		fpath_end_fill       = &fpath_end_fill_bw;
		fpath_deinit_context = &fpath_deinit_context_bw;
	}
}

bool fpath_is_aa_enabled() {
	return fpath_init_context == &fpath_init_context_aa;
}

#else

// Initialize for Black & White rendering.
fpath_init_context_func   fpath_init_context   = &fpath_init_context_bw;
fpath_begin_fill_func     fpath_begin_fill     = &fpath_begin_fill_bw;
fpath_draw_filled_func    fpath_draw_filled    = &fpath_draw_filled_bw;
fpath_end_fill_func       fpath_end_fill       = &fpath_end_fill_bw;
fpath_deinit_context_func fpath_deinit_context = &fpath_deinit_context_bw;

#endif
