#include "haptic_settings.h"

enum {
  HAPTIC_SETTINGS_KEY = 0x5354,
};

HapticPattern haptic_settings_get(void) {
  uint8_t value;
  if (persist_get_size(HAPTIC_SETTINGS_KEY) != (int)sizeof(value) ||
      persist_read_data(HAPTIC_SETTINGS_KEY, &value, sizeof(value)) != (int)sizeof(value) ||
      value >= HAPTIC_PATTERN_COUNT) {
    return HAPTIC_PATTERN_SHORT;
  }
  return (HapticPattern)value;
}

bool haptic_settings_set(HapticPattern pattern) {
  uint8_t value = (uint8_t)pattern;
  if (pattern < HAPTIC_PATTERN_SHORT || pattern >= HAPTIC_PATTERN_COUNT) {
    return false;
  }
  return persist_write_data(HAPTIC_SETTINGS_KEY, &value, sizeof(value)) == (int)sizeof(value);
}

void haptic_settings_vibrate(HapticPattern pattern) {
  switch (pattern) {
    case HAPTIC_PATTERN_LONG: vibes_long_pulse(); break;
    case HAPTIC_PATTERN_DOUBLE: vibes_double_pulse(); break;
    case HAPTIC_PATTERN_UNTIL_SELECT:
    case HAPTIC_PATTERN_SHORT:
    default: vibes_short_pulse(); break;
  }
}

bool haptic_settings_choose(HapticPattern pattern) {
  if (!haptic_settings_set(pattern)) {
    return false;
  }
  haptic_settings_vibrate(pattern);
  return true;
}
