
#include "pebble.h"
#include "basalt-compatibility.h"

#ifdef PBL_PLATFORM_APLITE
GBitmap* gbitmap_create_1bit(GSize size) {
    return gbitmap_create_blank(size);
}
GBitmapFormat gbitmap_get_format(const GBitmap *bitmap) {
	return GBitmapFormat1Bit;
}
#endif


#ifdef PBL_PLATFORM_BASALT
GBitmap* gbitmap_create_1bit(GSize size) {
    return gbitmap_create_blank(size, GBitmapFormat1Bit);
}
#endif

