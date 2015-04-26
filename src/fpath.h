
#pragma once
#include <pebble.h>

typedef int32_t fixed_t;

// Defines the fixed point conversions
#define FIXED_POINT_SHIFT 4
#define FIXED_POINT_SCALE 16
#define INT_TO_FIXED(a) ((a) * FIXED_POINT_SCALE)
#define FIXED_TO_INT(a) ((a) / FIXED_POINT_SCALE)
#define FIXED_MULTIPLY(a, b) (((a) * (b)) / FIXED_POINT_SCALE)

typedef struct FPoint {
    fixed_t x;
    fixed_t y;
} FPoint;
#define FPoint(x, y) ((FPoint){(x), (y)})
#define FPointI(x, y) ((FPoint){INT_TO_FIXED(x), INT_TO_FIXED(y)})
#define FPointZero FPoint(0, 0)
bool fpoint_equal(const FPoint * const point_a, const FPoint * const point_b);

typedef struct FSize {
    fixed_t w;
    fixed_t h;
} FSize;

typedef struct FRect {
    FPoint origin;
    FSize size;
} FRect;

typedef struct FPathInfo {
    uint32_t num_points;
    FPoint* points;
} FPathInfo;

typedef struct FPath {
	uint32_t num_points;
	FPoint* points;
	int32_t rotation;
	FPoint offset;
} FPath;

typedef struct FContext {
	GContext* gctx;
	GBitmap* flagBuffer;
	FPoint min;
	FPoint max;
	GColor strokeColor;
    GColor fillColor;
#ifdef PBL_COLOR
	bool aarampDirty;
	GColor8 aaramp[9];
#endif
} FContext;

void fpath_destroy(FPath* path);
void fpath_rotate_to(FPath* path, int32_t angle);
void fpath_move_to(FPath* path, FPoint point);

void fpath_set_fill_color(FContext* fctx, GColor c);
void fpath_set_stroke_color(FContext* fctx, GColor c);
#ifdef PBL_COLOR
void fpath_enable_aa(bool enable);
#endif

typedef void (*fpath_init_context_func)(FContext* fctx, GContext* gctx);
typedef void (*fpath_begin_fill_func)(FContext* fctx);
typedef void (*fpath_draw_filled_func)(FContext* fctx, FPath* fpath);
typedef void (*fpath_end_fill_func)(FContext* fctx);
typedef void (*fpath_deinit_context_func)(FContext* fctx);

extern fpath_init_context_func fpath_init_context;
extern fpath_begin_fill_func fpath_begin_fill;
extern fpath_draw_filled_func fpath_draw_filled;
extern fpath_end_fill_func fpath_end_fill;
extern fpath_deinit_context_func fpath_deinit_context;
