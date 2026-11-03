#include "phase_settings.h"

enum {
  PHASE_SETTINGS_KEY = 0x5355,
  PHASE_COUNT = 3,
  PHASE_MINUTES_MIN = 5,
  PHASE_MINUTES_MAX = 60,
  PHASE_MINUTES_STEP = 5,
};

static const uint8_t s_default_minutes[PHASE_COUNT] = { 45, 10, 5 };

static bool prv_valid_minutes(uint8_t minutes) {
  return minutes >= PHASE_MINUTES_MIN && minutes <= PHASE_MINUTES_MAX &&
         minutes % PHASE_MINUTES_STEP == 0;
}

static int prv_index(SessionPhase phase) {
  return phase == SESSION_PHASE_SIT ? 0 : phase == SESSION_PHASE_STAND ? 1 : 2;
}

/* One record of three bytes (Sit, Stand, Move), validated per byte. */
static void prv_load(uint8_t minutes[PHASE_COUNT]) {
  uint8_t stored[PHASE_COUNT];
  bool have = persist_get_size(PHASE_SETTINGS_KEY) == (int)sizeof(stored) &&
              persist_read_data(PHASE_SETTINGS_KEY, stored, sizeof(stored)) == (int)sizeof(stored);
  for (int i = 0; i < PHASE_COUNT; ++i) {
    minutes[i] = have && prv_valid_minutes(stored[i]) ? stored[i] : s_default_minutes[i];
  }
}

int phase_settings_get_minutes(SessionPhase phase) {
  uint8_t minutes[PHASE_COUNT];
  prv_load(minutes);
  return minutes[prv_index(phase)];
}

bool phase_settings_step(SessionPhase phase) {
  if (phase != SESSION_PHASE_SIT && phase != SESSION_PHASE_STAND && phase != SESSION_PHASE_MOVE) {
    return false;
  }
  uint8_t minutes[PHASE_COUNT];
  prv_load(minutes);
  uint8_t *value = &minutes[prv_index(phase)];
  *value = *value >= PHASE_MINUTES_MAX ? PHASE_MINUTES_MIN : (uint8_t)(*value + PHASE_MINUTES_STEP);
  return persist_write_data(PHASE_SETTINGS_KEY, minutes, sizeof(minutes)) == (int)sizeof(minutes);
}

bool phase_settings_reset(void) {
  return persist_write_data(PHASE_SETTINGS_KEY, s_default_minutes, sizeof(s_default_minutes)) ==
         (int)sizeof(s_default_minutes);
}
