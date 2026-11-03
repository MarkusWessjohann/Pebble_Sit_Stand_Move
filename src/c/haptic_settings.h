#pragma once

#include <pebble.h>
#include <stdbool.h>

typedef enum {
  HAPTIC_PATTERN_SHORT = 0,
  HAPTIC_PATTERN_LONG = 1,
  HAPTIC_PATTERN_DOUBLE = 2,
  HAPTIC_PATTERN_UNTIL_SELECT = 3,
  HAPTIC_PATTERN_COUNT,
} HapticPattern;

/* Stored pattern; a missing, corrupt, or unsupported value reads as Short. */
HapticPattern haptic_settings_get(void);
/* Returns false, leaving the stored choice unchanged, if the write fails. */
bool haptic_settings_set(HapticPattern pattern);
/* Plays one vibration of the pattern. Owns the mapping to the native Pebble calls. Until Select
 * plays one short pulse; the repeating is haptic_alert's job. */
void haptic_settings_vibrate(HapticPattern pattern);
/* Saves the pattern and, only if the save succeeded, plays one preview. Returns whether it saved,
 * so the caller closes the Settings window on true and leaves it open on false. */
bool haptic_settings_choose(HapticPattern pattern);
