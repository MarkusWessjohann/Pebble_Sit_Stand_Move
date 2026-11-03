#include <pebble.h>
#include <stdio.h>
#include <time.h>

#include "haptic_alert.h"
#include "haptic_settings.h"
#include "session_coordinator.h"

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_countdown_layer;
static Layer *s_icon_layer;
static TextLayer *s_phase_layer;
static TextLayer *s_label_layers[3];
static TextLayer *s_tally_layer;
static TextLayer *s_error_layer;
static Window *s_menu_window;
static MenuLayer *s_menu_layer;
static Window *s_settings_window;
static MenuLayer *s_settings_layer;
static AppTimer *s_refresh_timer;
static char s_time_text[16];
static char s_countdown_text[16];
static char s_phase_text[16];
static char s_label_text[3][8];
static char s_tally_text[48];
static char s_error_text[32];

#define LABEL_COLUMN_WIDTH 78
#define LABEL_RIGHT_MARGIN 5
#define LABEL_HEIGHT 22
#define TALLY_HEIGHT 28
#define ERROR_HEIGHT 18

static const char *prv_error_text(SessionError error) {
  switch (error) {
    case SESSION_ERROR_CLOCK: return "Clock time unavailable";
    case SESSION_ERROR_STORAGE: return "Session save failed";
    case SESSION_ERROR_WAKEUP: return "Reminder scheduling failed";
    case SESSION_ERROR_RECORD: return "Session data invalid";
    case SESSION_ERROR_QUEUE: return "Please retry";
    case SESSION_ERROR_NONE:
    default: return "";
  }
}

static const char *prv_phase_name(SessionPhase phase) {
  switch (phase) {
    case SESSION_PHASE_SIT: return "Sit";
    case SESSION_PHASE_STAND: return "Stand";
    case SESSION_PHASE_MOVE: return "Move";
    case SESSION_PHASE_NONE:
    default: return "Ready";
  }
}

static void prv_draw_icon(Layer *layer, GContext *ctx) {
  SessionView view;
  session_coordinator_get_view(&view);
  GRect bounds = layer_get_bounds(layer);
  int16_t center = bounds.size.w / 2;
  int16_t top = 2;
  graphics_context_set_stroke_width(ctx, 2);
  if (view.record.phase == SESSION_PHASE_SIT) {
    graphics_draw_line(ctx, GPoint(center - 7, top), GPoint(center - 7, top + 16));
    graphics_draw_line(ctx, GPoint(center - 7, top + 9), GPoint(center + 7, top + 9));
    graphics_draw_line(ctx, GPoint(center + 7, top + 9), GPoint(center + 7, top + 16));
    graphics_draw_line(ctx, GPoint(center - 7, top + 16), GPoint(center + 7, top + 16));
    graphics_draw_line(ctx, GPoint(center - 4, top + 16), GPoint(center - 4, top + 23));
    graphics_draw_line(ctx, GPoint(center + 4, top + 16), GPoint(center + 4, top + 23));
  } else if (view.record.phase == SESSION_PHASE_STAND) {
    graphics_draw_circle(ctx, GPoint(center, top + 3), 3);
    graphics_draw_line(ctx, GPoint(center, top + 7), GPoint(center, top + 17));
    graphics_draw_line(ctx, GPoint(center, top + 10), GPoint(center - 7, top + 15));
    graphics_draw_line(ctx, GPoint(center, top + 10), GPoint(center + 7, top + 15));
    graphics_draw_line(ctx, GPoint(center, top + 17), GPoint(center - 6, top + 24));
    graphics_draw_line(ctx, GPoint(center, top + 17), GPoint(center + 6, top + 24));
  } else if (view.record.phase == SESSION_PHASE_MOVE) {
    graphics_draw_circle(ctx, GPoint(center + 3, top + 3), 3);
    graphics_draw_line(ctx, GPoint(center + 1, top + 7), GPoint(center - 3, top + 15));
    graphics_draw_line(ctx, GPoint(center - 1, top + 10), GPoint(center - 9, top + 8));
    graphics_draw_line(ctx, GPoint(center - 1, top + 10), GPoint(center + 7, top + 12));
    graphics_draw_line(ctx, GPoint(center - 3, top + 15), GPoint(center - 9, top + 23));
    graphics_draw_line(ctx, GPoint(center - 3, top + 15), GPoint(center + 6, top + 22));
  }
}

static void prv_render(void) {
  SessionView view;
  session_coordinator_get_view(&view);
  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  if (local && now > 0) {
    strftime(s_time_text, sizeof(s_time_text), "%H:%M", local);
  } else {
    snprintf(s_time_text, sizeof(s_time_text), "--:--");
  }
  if (haptic_alert_active()) {
    snprintf(s_time_text, sizeof(s_time_text), "Select to stop");
  }
  text_layer_set_text(s_time_layer, s_time_text);

  if (view.record.lifecycle == SESSION_TIMED_PAUSED) {
    snprintf(s_countdown_text, sizeof(s_countdown_text), "%02d:%02d",
             (int)(view.remaining_seconds / 60), (int)(view.remaining_seconds % 60));
    snprintf(s_phase_text, sizeof(s_phase_text), "BREAK");
    text_layer_set_font(s_phase_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    int shorter = view.record.pause_duration_seconds == 5 * 60 ? 10 : 5;
    int longer = view.record.pause_duration_seconds == 30 * 60 ? 10 : 30;
    snprintf(s_label_text[0], sizeof(s_label_text[0]), "%dm", shorter);
    snprintf(s_label_text[1], sizeof(s_label_text[1]), "BREAK");
    snprintf(s_label_text[2], sizeof(s_label_text[2]), "%dm", longer);
  } else if (view.record.lifecycle == SESSION_RUNNING || view.record.lifecycle == SESSION_PAUSED) {
    snprintf(s_countdown_text, sizeof(s_countdown_text), "%02d:%02d",
             (int)(view.remaining_seconds / 60), (int)(view.remaining_seconds % 60));
    for (int i = 0; i < 3; i++) {
      s_label_text[i][0] = '\0';
    }
    if (view.record.lifecycle == SESSION_RUNNING) {
      snprintf(s_label_text[0], sizeof(s_label_text[0]), "+5");
      snprintf(s_label_text[1], sizeof(s_label_text[1]), "II");
      snprintf(s_label_text[2], sizeof(s_label_text[2]), "-5");
    }
    if (view.record.lifecycle == SESSION_PAUSED) {
      snprintf(s_label_text[1], sizeof(s_label_text[1]), "PAUSED");
    }
    snprintf(s_phase_text, sizeof(s_phase_text), "%s", prv_phase_name(view.record.phase));
    text_layer_set_font(s_phase_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  } else {
    snprintf(s_countdown_text, sizeof(s_countdown_text), "--:--");
    snprintf(s_phase_text, sizeof(s_phase_text), "Ready");
    for (int i = 0; i < 3; i++) {
      s_label_text[i][0] = '\0';
    }
    text_layer_set_font(s_phase_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  }
  text_layer_set_text(s_countdown_layer, s_countdown_text);
  text_layer_set_text(s_phase_layer, s_phase_text);
  for (int i = 0; i < 3; i++) {
    text_layer_set_text(s_label_layers[i], s_label_text[i]);
  }
  if (s_icon_layer) {
    layer_mark_dirty(s_icon_layer);
  }
  snprintf(s_tally_text, sizeof(s_tally_text), "S%u T%u M%u",
           (unsigned)view.record.sit_count, (unsigned)view.record.stand_count,
           (unsigned)view.record.move_count);
  text_layer_set_text(s_tally_layer, s_tally_text);
  snprintf(s_error_text, sizeof(s_error_text), "%s", prv_error_text(view.error));
  text_layer_set_text(s_error_layer, s_error_text);
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

static void prv_refresh(void *context) {
  s_refresh_timer = NULL;
  prv_render();
  s_refresh_timer = app_timer_register(1000, prv_refresh, NULL);
}

static uint16_t prv_menu_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  SessionView view;
  session_coordinator_get_view(&view);
  return view.record.lifecycle == SESSION_INACTIVE ? 2 : 4;
}

/* Settings is the last row in every lifecycle state and is handled here, never by the
 * coordinator, whose row numbering stays as it was. */
static bool prv_is_settings_row(uint16_t row) {
  SessionView view;
  session_coordinator_get_view(&view);
  return row == (view.record.lifecycle == SESSION_INACTIVE ? 1 : 3);
}

static void prv_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                              void *context) {
  SessionView view;
  session_coordinator_get_view(&view);
  const char *title;
  if (prv_is_settings_row(cell_index->row)) {
    title = "Settings";
  } else if (view.record.lifecycle == SESSION_INACTIVE) {
    title = "Start";
  } else if (cell_index->row == 0) {
    title = view.record.lifecycle == SESSION_RUNNING ? "Pause" : "Resume";
  } else if (cell_index->row == 1) {
    title = "Skip";
  } else {
    title = "Stop";
  }
  menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);
}

static void prv_render_cb(void *context) {
  (void)context;
  if (s_time_layer) {
    prv_render();
  }
}

static void prv_menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  (void)menu_layer;
  (void)context;
  if (prv_is_settings_row(cell_index->row)) {
    window_stack_push(s_settings_window, true);
    return;
  }
  session_coordinator_select_menu_row(cell_index->row);
  window_stack_pop(true);
  app_timer_register(5, prv_render_cb, NULL);
}

static void prv_menu_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_menu_layer = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = prv_menu_num_rows,
    .draw_row = prv_menu_draw_row,
    .select_click = prv_menu_select,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(root, menu_layer_get_layer(s_menu_layer));
}

static void prv_menu_window_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
}

static const char *const s_pattern_titles[] = { "Short", "Long", "Double", "Until Select" };
static const char *const s_pattern_marked_titles[] = {
  "* Short", "* Long", "* Double", "* Until Select"
};
#define PATTERN_COUNT HAPTIC_PATTERN_COUNT

static uint16_t prv_settings_num_rows(MenuLayer *menu_layer, uint16_t section_index,
                                      void *context) {
  return PATTERN_COUNT;
}

static void prv_settings_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                                  void *context) {
  bool current = (int)haptic_settings_get() == cell_index->row;
  menu_cell_basic_draw(ctx, cell_layer,
                       current ? s_pattern_marked_titles[cell_index->row] :
                                 s_pattern_titles[cell_index->row],
                       NULL, NULL);
}

static void prv_settings_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  (void)menu_layer;
  (void)context;
  if (haptic_settings_choose((HapticPattern)cell_index->row)) {
    window_stack_pop(true);
  }
  /* On a failed save the window stays open with no preview: the mark not moving is the signal. */
}

static void prv_settings_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_settings_layer = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_settings_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = prv_settings_num_rows,
    .draw_row = prv_settings_draw_row,
    .select_click = prv_settings_select,
  });
  menu_layer_set_selected_index(s_settings_layer,
                                (MenuIndex){ .section = 0, .row = (uint16_t)haptic_settings_get() },
                                MenuRowAlignCenter, false);
  menu_layer_set_click_config_onto_window(s_settings_layer, window);
  layer_add_child(root, menu_layer_get_layer(s_settings_layer));
}

static void prv_settings_window_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_settings_layer);
  s_settings_layer = NULL;
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (session_coordinator_middle_control()) {
    app_timer_register(5, prv_render_cb, NULL);
    return;
  }
  window_stack_push(s_menu_window, true);
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  if (session_coordinator_top_control()) {
    app_timer_register(5, prv_render_cb, NULL);
  } else {
    window_stack_push(s_menu_window, true);
  }
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  if (session_coordinator_bottom_control()) {
    app_timer_register(5, prv_render_cb, NULL);
  } else {
    window_stack_push(s_menu_window, true);
  }
}

static void prv_menu_click_handler(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  window_stack_push(s_menu_window, true);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, prv_menu_click_handler, NULL);
}

static TextLayer *prv_create_label_layer(Layer *root, GRect bounds, int center_y) {
  TextLayer *layer = text_layer_create(GRect(bounds.size.w - LABEL_COLUMN_WIDTH -
                                             LABEL_RIGHT_MARGIN,
                                             center_y - LABEL_HEIGHT / 2,
                                             LABEL_COLUMN_WIDTH, LABEL_HEIGHT));
  /* The countdown is centered on the full screen width, so its layer overlaps this one. */
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(layer));
  return layer;
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 26));
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  /* Countdown, icon, and phase name form one block centered on the screen, both axes. */
  int16_t block_y = (bounds.size.h - (40 + 27 + 28)) / 2;

  s_countdown_layer = text_layer_create(GRect(0, block_y, bounds.size.w, 40));
  text_layer_set_font(s_countdown_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_countdown_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_countdown_layer, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_countdown_layer));

  s_icon_layer = layer_create(GRect(0, block_y + 40, bounds.size.w, 27));
  layer_set_update_proc(s_icon_layer, prv_draw_icon);
  layer_add_child(root, s_icon_layer);

  s_phase_layer = text_layer_create(GRect(0, block_y + 40 + 27, bounds.size.w, 28));
  text_layer_set_font(s_phase_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_phase_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_phase_layer));

  /* Labels sit level with the top, middle, and lower right-side buttons. The quarter positions
   * are an estimate and must be checked against Pebble Time 2 hardware. */
  s_label_layers[0] = prv_create_label_layer(root, bounds, bounds.size.h / 4);
  s_label_layers[1] = prv_create_label_layer(root, bounds, bounds.size.h / 2);
  s_label_layers[2] = prv_create_label_layer(root, bounds, bounds.size.h * 3 / 4);

  int16_t tally_y = bounds.size.h - ERROR_HEIGHT - TALLY_HEIGHT;
  s_tally_layer = text_layer_create(GRect(0, tally_y, bounds.size.w, TALLY_HEIGHT));
  text_layer_set_font(s_tally_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_tally_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_tally_layer));

  s_error_layer = text_layer_create(GRect(0, bounds.size.h - ERROR_HEIGHT, bounds.size.w,
                                          ERROR_HEIGHT));
  text_layer_set_font(s_error_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_error_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_error_layer));
  prv_render();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_countdown_layer);
  layer_destroy(s_icon_layer);
  text_layer_destroy(s_phase_layer);
  for (int i = 0; i < 3; i++) {
    text_layer_destroy(s_label_layers[i]);
    s_label_layers[i] = NULL;
  }
  text_layer_destroy(s_tally_layer);
  text_layer_destroy(s_error_layer);
  s_time_layer = NULL;
  s_countdown_layer = NULL;
  s_icon_layer = NULL;
  s_phase_layer = NULL;
  s_tally_layer = NULL;
  s_error_layer = NULL;
}

static void prv_init(void) {
  session_coordinator_init();
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers){
    .load = prv_menu_window_load,
    .unload = prv_menu_window_unload,
  });
  s_settings_window = window_create();
  window_set_window_handlers(s_settings_window, (WindowHandlers){
    .load = prv_settings_window_load,
    .unload = prv_settings_window_unload,
  });
  window_stack_push(s_window, true);
  s_refresh_timer = app_timer_register(1000, prv_refresh, NULL);
}

static void prv_deinit(void) {
  if (s_refresh_timer) {
    app_timer_cancel(s_refresh_timer);
  }
  session_coordinator_deinit();
  window_destroy(s_settings_window);
  window_destroy(s_menu_window);
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
