#pragma once

#include "session_store.h"

typedef enum {
  SESSION_ERROR_NONE = 0,
  SESSION_ERROR_CLOCK,
  SESSION_ERROR_STORAGE,
  SESSION_ERROR_WAKEUP,
  SESSION_ERROR_RECORD,
  SESSION_ERROR_QUEUE,
} SessionError;

typedef struct {
  SessionRecord record;
  SessionError error;
  int32_t remaining_seconds;
} SessionView;

void session_coordinator_init(void);
void session_coordinator_deinit(void);
void session_coordinator_start(void);
void session_coordinator_pause(void);
void session_coordinator_resume(void);
void session_coordinator_timed_pause(int32_t duration_seconds);
bool session_coordinator_middle_control(void);
bool session_coordinator_top_control(void);
bool session_coordinator_bottom_control(void);
void session_coordinator_skip(void);
void session_coordinator_stop(void);
void session_coordinator_select_menu_row(uint16_t row);
void session_coordinator_recover(void);
void session_coordinator_wakeup(WakeupId id, int32_t cookie);
void session_coordinator_get_view(SessionView *view);
