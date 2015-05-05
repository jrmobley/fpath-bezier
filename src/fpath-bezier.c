#include <pebble.h>
#include "fpath_builder.h"

#define MAX_POINTS 256
#define DRAW_LINE false
#define BENCHMARK false
#define MAX_DEMO_PATHS 4

static const int rot_step = TRIG_MAX_ANGLE / 360;
static Window *window;
static Layer *layer;
static GPath *s_gpath;
static FPath *s_fpath;
static FContext s_fctx;
static uint8_t path_switcher = 0;
static enum {DRAW_GPATH, DRAW_FPATH_BW, DRAW_FPATH_AA} draw_line_switcher = DRAW_GPATH;
#ifdef PBL_COLOR
static GColor8 foreground_color;
static GColor8 background_color;
#else
static GColor foreground_color;
static GColor background_color;
#endif

static void prv_create_path(void);

static void app_timer_callback(void *data) {
  if (s_gpath) {
    s_gpath->rotation = (s_gpath->rotation + rot_step) % TRIG_MAX_ANGLE;
    s_fpath->rotation = s_gpath->rotation;
    layer_mark_dirty(layer);
  }
  
  app_timer_register(35, app_timer_callback, NULL);
}

static void update_layer(struct Layer *layer, GContext *ctx) {

  if (DRAW_GPATH == draw_line_switcher) {

    graphics_context_set_fill_color(ctx, foreground_color);
    gpath_draw_filled(ctx, s_gpath);

  } else {

#ifdef PBL_COLOR
    bool is_aa = fpath_is_aa_enabled();
    if (is_aa && DRAW_FPATH_BW == draw_line_switcher) {
      fpath_deinit_context(&s_fctx);
      fpath_enable_aa(false);
    } else if (!is_aa && DRAW_FPATH_AA == draw_line_switcher) {
      fpath_deinit_context(&s_fctx);
      fpath_enable_aa(true);
    }
#endif

    if (NULL == s_fctx.gctx) {
      fpath_init_context(&s_fctx, ctx);
    }

    fpath_set_stroke_color(&s_fctx, background_color);
    fpath_set_fill_color(&s_fctx, foreground_color);
    fpath_begin_fill(&s_fctx);
    fpath_draw_filled(&s_fctx, s_fpath);
    fpath_end_fill(&s_fctx);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Up");
#ifdef PBL_COLOR
  draw_line_switcher = (draw_line_switcher + 1) % 3;
#else
  draw_line_switcher = (draw_line_switcher + 1) % 2;
#endif
  layer_mark_dirty(layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Select");
  path_switcher = (path_switcher + 1) % MAX_DEMO_PATHS;
  prv_create_path();
  layer_mark_dirty(layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(text_layer, "Down");
  if (gcolor_equal(background_color, GColorBlack)) {
    background_color = GColorWhite;
    foreground_color = GColorBlack;
  } else {
    background_color = GColorBlack;
    foreground_color = GColorWhite;
  }
  
  window_set_background_color(window, background_color);
  layer_mark_dirty(layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void prv_create_path() {
  if (s_gpath) {
    gpath_destroy(s_gpath);
  }
  if (s_fpath) {
    fpath_destroy(s_fpath);
  }
  
#if BENCHMARK
  time_t start = time(NULL);
  uint16_t start_ms = time_ms(NULL, NULL);
#endif
  
  FPathBuilder *builder = fpath_builder_create(MAX_POINTS);
  
  switch (path_switcher) {
  case 0:
      fpath_builder_move_to_point (builder, FPointI(-15, -15));
      fpath_builder_curve_to_point(builder, FPointI( 15, -15), FPointI(-15, -60), FPointI( 15, -60));
      fpath_builder_curve_to_point(builder, FPointI( 15,  15), FPointI( 60, -15), FPointI( 60,  15));
      fpath_builder_curve_to_point(builder, FPointI(-15,  15), FPointI( 15,  60), FPointI(-15,  60));
      fpath_builder_curve_to_point(builder, FPointI(-15, -15), FPointI(-60,  15), FPointI(-60, -15));
      break;
  case 1:
      fpath_builder_move_to_point (builder, FPointI(-20, -50));
      fpath_builder_curve_to_point(builder, FPointI( 20, -50), FPointI(-25, -60), FPointI( 25, -60));
      fpath_builder_curve_to_point(builder, FPointI( 20,  50), FPointI(  0,   0), FPointI(  0,   0));
      fpath_builder_curve_to_point(builder, FPointI(-20,  50), FPointI( 25,  60), FPointI(-25,  60));
      fpath_builder_curve_to_point(builder, FPointI(-20, -50), FPointI(  0,   0), FPointI(  0,   0));
      break;
  case 2:
      fpath_builder_move_to_point (builder, FPointI(  0, -60));
      fpath_builder_curve_to_point(builder, FPointI( 60,   0), FPointI( 35, -60), FPointI( 60, -35));
      fpath_builder_curve_to_point(builder, FPointI(  0,  60), FPointI( 60,  35), FPointI( 35,  60));
      fpath_builder_curve_to_point(builder, FPointI(  0,   0), FPointI(-50,  60), FPointI(-50,   0));
      fpath_builder_curve_to_point(builder, FPointI(  0, -60), FPointI( 50,   0), FPointI( 50, -60));
      break;
  case 3:
      fpath_builder_move_to_point (builder, FPointI(  0, -60));
      fpath_builder_curve_to_point(builder, FPointI( 60,   0), FPointI( 35, -60), FPointI( 60, -35));
      fpath_builder_line_to_point (builder, FPointI(-60,   0));
      fpath_builder_curve_to_point(builder, FPointI(  0,  60), FPointI(-60,  35), FPointI(-35,  60));
      fpath_builder_line_to_point (builder, FPointI(  0, -60));
      break;
  default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid demo path id: %d", path_switcher);
  }
  
  
  s_fpath = fpath_builder_create_path(builder);
  s_gpath = fpath_builder_create_gpath(builder);
  fpath_builder_destroy(builder);
  
#if BENCHMARK
  time_t end = time(NULL);
  uint16_t end_ms = time_ms(NULL, NULL);
#endif
  
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  gpath_move_to(s_gpath, GPoint((int16_t)(bounds.size.w/2), (int16_t)(bounds.size.h/2)));
  fpath_move_to(s_fpath, FPointI((int16_t)(bounds.size.w/2), (int16_t)(bounds.size.h/2)));

#if BENCHMARK
  int total = (end - start) * 1000 + end_ms - start_ms;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "building took %d ms (%d points)", total, (int)s_fpath->num_points);
#endif
}

static void window_load(Window *window) {
  foreground_color = GColorWhite;
  background_color = GColorBlack;
  
  Layer *window_layer = window_get_root_layer(window);
  window_set_background_color(window, background_color);
  GRect bounds = layer_get_bounds(window_layer);

  layer = layer_create(bounds);
  layer_set_update_proc(layer, update_layer);
  layer_add_child(window_layer, layer);
  
  prv_create_path();
  
  app_timer_callback(NULL);
}

static void window_unload(Window *window) {
  gpath_destroy(s_gpath);
  fpath_destroy(s_fpath);
  layer_destroy(layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  fpath_deinit_context(&s_fctx);
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
