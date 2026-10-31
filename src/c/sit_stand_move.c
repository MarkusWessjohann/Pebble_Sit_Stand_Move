#include <pebble.h>
#include <stdio.h>
#include <time.h>

static Window *s_window;
static TextLayer *s_text_layer;
static char s_status[128] = "UP: phase 120s\nDOWN: pause 180s\nSELECT: cancel";
static WakeupId s_test_wakeup_id = (WakeupId)-1;

enum {
  COOKIE_PHASE_TEST = 101,
  COOKIE_PAUSE_TEST = 102,
};

static void prv_show_status(const char *status) {
  snprintf(s_status, sizeof(s_status), "%s", status);
  if (s_text_layer) {
    text_layer_set_text(s_text_layer, s_status);
  }
}

static void prv_schedule_test(int32_t cookie, int32_t delay_seconds, const char *name) {
  time_t now = time(NULL);
  if (now <= 0) {
    prv_show_status("Clock unavailable\nSet watch time and retry");
    APP_LOG(APP_LOG_LEVEL_ERROR, "Wakeup test not scheduled: time(NULL)=%ld", (long)now);
    return;
  }

  if (s_test_wakeup_id != (WakeupId)-1) {
    APP_LOG(APP_LOG_LEVEL_INFO, "TEST_PREVIOUS_WAKEUP_CANCEL_REQUESTED id=%d", (int)s_test_wakeup_id);
    wakeup_cancel(s_test_wakeup_id);
    s_test_wakeup_id = (WakeupId)-1;
  }

  time_t deadline = now + delay_seconds;
  WakeupId id = wakeup_schedule(deadline, cookie, false);
  if (id == (WakeupId)-1) {
    snprintf(s_status, sizeof(s_status), "%s test failed\nWakeup schedule error", name);
    text_layer_set_text(s_text_layer, s_status);
    APP_LOG(APP_LOG_LEVEL_ERROR, "TEST_SCHEDULE_FAILED name=%s cookie=%ld deadline=%ld",
            name, (long)cookie, (long)deadline);
    return;
  }
  s_test_wakeup_id = id;

  snprintf(s_status, sizeof(s_status), "%s test scheduled\nID %d\nWait %ld sec, then check log",
           name, (int)id, (long)delay_seconds);
  text_layer_set_text(s_text_layer, s_status);
  APP_LOG(APP_LOG_LEVEL_INFO, "TEST_SCHEDULE name=%s id=%d cookie=%ld now=%ld deadline=%ld result=%d",
          name, (int)id, (long)cookie, (long)now, (long)deadline, (int)id);
}

static void prv_report_wakeup(WakeupId id, int32_t cookie) {
  const char *name = cookie == COOKIE_PHASE_TEST ? "Phase" :
                     cookie == COOKIE_PAUSE_TEST ? "Timed pause" : "Unknown";
  time_t now = time(NULL);
  if (id == s_test_wakeup_id) {
    s_test_wakeup_id = (WakeupId)-1;
  }
  if (cookie != COOKIE_PHASE_TEST && cookie != COOKIE_PAUSE_TEST) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "TEST_WAKEUP_IGNORED id=%d cookie=%ld",
            (int)id, (long)cookie);
    return;
  }
  snprintf(s_status, sizeof(s_status), "%s Wakeup received\nID %d\nHaptic requested",
           name, (int)id);
  if (s_text_layer) {
    text_layer_set_text(s_text_layer, s_status);
  }
  vibes_short_pulse();
  APP_LOG(APP_LOG_LEVEL_INFO, "TEST_WAKEUP name=%s id=%d cookie=%ld callback_time=%ld haptic_requested=1",
          name, (int)id, (long)cookie, (long)now);
}

static void prv_wakeup_handler(WakeupId id, int32_t cookie) {
  prv_report_wakeup(id, cookie);
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_test_wakeup_id != (WakeupId)-1) {
    APP_LOG(APP_LOG_LEVEL_INFO, "TEST_WAKEUP_CANCEL_REQUESTED id=%d", (int)s_test_wakeup_id);
    wakeup_cancel(s_test_wakeup_id);
    s_test_wakeup_id = (WakeupId)-1;
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "TEST_WAKEUP_CANCEL_NO_PENDING_ID");
  }
  prv_show_status("Cancel requested\nWait past deadline\nUP: phase / DOWN: pause");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  prv_schedule_test(COOKIE_PHASE_TEST, 120, "Phase");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  prv_schedule_test(COOKIE_PAUSE_TEST, 180, "Timed pause");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 24, bounds.size.w, bounds.size.h - 48));
  text_layer_set_text(s_text_layer, s_status);
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void prv_init(void) {
  wakeup_service_subscribe(prv_wakeup_handler);

  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    APP_LOG(APP_LOG_LEVEL_INFO, "TEST_WAKEUP_LAUNCH reason=%d", (int)launch_reason());
    snprintf(s_status, sizeof(s_status), "Wakeup launched\nCheck callback log");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "TEST_APP_LAUNCH reason=%d", (int)launch_reason());
  }

  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
