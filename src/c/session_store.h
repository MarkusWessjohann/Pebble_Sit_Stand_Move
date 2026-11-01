#pragma once

#include <pebble.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SESSION_INACTIVE = 0,
  SESSION_RUNNING = 1,
} SessionLifecycle;

typedef enum {
  SESSION_PHASE_NONE = 0,
  SESSION_PHASE_SIT = 1,
  SESSION_PHASE_STAND = 2,
  SESSION_PHASE_MOVE = 3,
} SessionPhase;

typedef struct {
  SessionLifecycle lifecycle;
  SessionPhase phase;
  uint32_t generation;
  uint32_t haptic_generation;
  int64_t deadline_utc;
  WakeupId wakeup_id;
  int32_t tally_day;
  uint16_t sit_count;
  uint16_t stand_count;
  uint16_t move_count;
} SessionRecord;

bool session_store_load(SessionRecord *record, bool *found);
bool session_store_save(const SessionRecord *record);
bool session_store_clear(void);
bool session_store_validate(const SessionRecord *record);
