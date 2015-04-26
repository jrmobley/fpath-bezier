
#pragma once

GBitmap* gbitmap_create_1bit(GSize size);

#ifdef PBL_PLATFORM_APLITE

typedef enum GBitmapFormat {
	GBitmapFormat1Bit = 0, //<! 1-bit black and white. 0 = black, 1 = white.
} GBitmapFormat;

GBitmapFormat gbitmap_get_format(const GBitmap *bitmap);


#endif
