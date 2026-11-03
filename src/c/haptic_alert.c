#include "haptic_alert.h"

#include "haptic_settings.h"

enum {
  HAPTIC_ALERT_INTERVAL_MS = 2000,
  /* Cap counted in ticks, without reading the clock. The first pulse plays before the timer, ticks
   * 1..59 each repeat it, and tick 60 (at 120 s) only stops, so the last pulse lands at 118 s. */
  HAPTIC_ALERT_MAX_TICKS = 60,
};

static AppTimer *s_timer;
static bool s_active;
static int s_ticks;

static void prv_tick(void *context);

static void prv_arm(void) {
  s_timer = app_timer_register(HAPTIC_ALERT_INTERVAL_MS, prv_tick, NULL);
  if (!s_timer) {
    s_active = false; /* no timer, no repeat: never show a cue that nothing will end */
  }
}

static void prv_tick(void *context) {
  (void)context;
  s_timer = NULL;
  if (!s_active) {
    return;
  }
  if (++s_ticks >= HAPTIC_ALERT_MAX_TICKS) {
    haptic_alert_stop();
    return;
  }
  haptic_settings_vibrate(HAPTIC_PATTERN_UNTIL_SELECT);
  prv_arm();
}

void haptic_alert_start(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  s_active = true;
  s_ticks = 0;
  prv_arm();
}

void haptic_alert_stop(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  if (s_active) {
    s_active = false;
    s_ticks = 0;
    vibes_cancel();
  }
}

bool haptic_alert_active(void) {
  return s_active;
}

bool haptic_alert_dismiss(void) {
  if (!s_active) {
    return false;
  }
  haptic_alert_stop();
  return true;
}
