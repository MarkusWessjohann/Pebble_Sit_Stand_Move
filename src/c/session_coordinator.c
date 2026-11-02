#include "session_coordinator.h"

#include <limits.h>
#include <string.h>
#include <time.h>

enum {
  SESSION_DURATION_SECONDS = 45 * 60,
  SESSION_QUEUE_SIZE = 8,
};

typedef enum {
  EVENT_START,
  EVENT_PAUSE,
  EVENT_RESUME,
  EVENT_STOP,
  EVENT_MENU_SELECT,
  EVENT_RECOVER,
  EVENT_WAKEUP,
  EVENT_DAY_ROLLOVER,
  EVENT_CLOCK_CHECK,
} SessionEventType;

typedef struct {
  SessionEventType type;
  WakeupId wakeup_id;
  int32_t cookie;
  uint16_t row;
} SessionEvent;

static SessionRecord s_record;
static SessionError s_error;
static SessionEvent s_queue[SESSION_QUEUE_SIZE];
static uint8_t s_queue_read;
static uint8_t s_queue_count;
static AppTimer *s_drain_timer;
static int32_t s_rollover_requested_day;
static bool s_clock_check_queued;

static SessionPhase prv_next_phase(SessionPhase phase);
static int32_t prv_duration(SessionPhase phase);
static int32_t prv_local_day(time_t now);
static void prv_rollover_tally(time_t now);
static void prv_count_completion(SessionRecord *record, SessionPhase phase, int64_t completed_at,
                                 int32_t today);
static void prv_reset_tally_day(SessionRecord *record, int32_t day);
static void prv_wakeup(const SessionEvent *event);

static void prv_reconcile_phase(SessionRecord *record, time_t now, bool overdue_recovery) {
  int32_t today = prv_local_day(now);
  struct tm *local = localtime(&now);
  int64_t today_start = 0;
  if (local) {
    struct tm midnight = *local;
    midnight.tm_hour = 0;
    midnight.tm_min = 0;
    midnight.tm_sec = 0;
    today_start = (int64_t)mktime(&midnight);
  }
  if (today_start > 0 && record->deadline_utc < today_start) {
    int64_t cycles = (today_start - record->deadline_utc) / 3600;
    record->deadline_utc += cycles * 3600;
  }
  bool rebase = overdue_recovery;
  while (now >= record->deadline_utc) {
    prv_count_completion(record, record->phase, record->deadline_utc, today);
    record->phase = prv_next_phase(record->phase);
    record->deadline_utc += prv_duration(record->phase);
    if (now >= record->deadline_utc) {
      rebase = true;
    }
  }
  if (rebase) {
    record->deadline_utc = (int64_t)now + prv_duration(record->phase);
  }
}

static void prv_request_haptic(void) {
  vibes_short_pulse();
}

static bool prv_next_generation(uint32_t current, uint32_t *next) {
  if (current >= INT32_MAX) {
    return false;
  }
  *next = current + 1;
  return true;
}

static bool prv_valid_utc(time_t now) {
  return now > 0 && (uint64_t)now <= INT32_MAX;
}

static bool prv_read_utc(time_t *now) {
  *now = time(NULL);
  return prv_valid_utc(*now);
}

static bool prv_schedule(SessionRecord *candidate, WakeupId *id) {
  if (candidate->deadline_utc <= 0 || candidate->deadline_utc > INT32_MAX) {
    return false;
  }
  *id = wakeup_schedule((time_t)candidate->deadline_utc, (int32_t)candidate->generation, false);
  return *id >= 0;
}

static void prv_commit_failure(bool storage_failure) {
  s_error = storage_failure ? SESSION_ERROR_STORAGE : SESSION_ERROR_WAKEUP;
}

static void prv_start(void) {
  time_t now;
  if (s_record.lifecycle != SESSION_INACTIVE) {
    return;
  }
  if (!prv_read_utc(&now)) {
    s_error = SESSION_ERROR_CLOCK;
    return;
  }
  if ((uint64_t)now > (uint64_t)INT32_MAX - SESSION_DURATION_SECONDS) {
    s_error = SESSION_ERROR_CLOCK;
    return;
  }

  SessionRecord candidate = s_record;
  candidate.lifecycle = SESSION_RUNNING;
  candidate.phase = SESSION_PHASE_SIT;
  if (!prv_next_generation(candidate.generation, &candidate.generation)) {
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  candidate.deadline_utc = (int64_t)now + SESSION_DURATION_SECONDS;
  candidate.wakeup_id = (WakeupId)-1;
  WakeupId candidate_id;
  if (!prv_schedule(&candidate, &candidate_id)) {
    s_error = SESSION_ERROR_WAKEUP;
    return;
  }
  candidate.wakeup_id = candidate_id;
  if (!session_store_save(&candidate)) {
    wakeup_cancel(candidate_id);
    prv_commit_failure(true);
    return;
  }
  s_record = candidate;
  s_error = SESSION_ERROR_NONE;
}

static bool prv_reconcile_before_command(time_t now) {
  if (s_record.lifecycle != SESSION_RUNNING || now < s_record.deadline_utc) {
    return true;
  }
  if (s_record.wakeup_id < 0) {
    s_error = SESSION_ERROR_WAKEUP;
    return false;
  }
  SessionEvent due = { .type = EVENT_WAKEUP, .wakeup_id = s_record.wakeup_id,
                       .cookie = (int32_t)s_record.generation };
  prv_wakeup(&due);
  return s_record.lifecycle == SESSION_RUNNING && s_record.deadline_utc > now &&
         s_error == SESSION_ERROR_NONE;
}

static void prv_pause(void) {
  time_t now;
  if (s_record.lifecycle != SESSION_RUNNING) {
    return;
  }
  if (!prv_read_utc(&now)) {
    s_error = SESSION_ERROR_CLOCK;
    return;
  }
  if (!prv_reconcile_before_command(now)) {
    return;
  }
  int64_t remaining = s_record.deadline_utc - now;
  if (remaining <= 0 || remaining > INT32_MAX) {
    s_error = SESSION_ERROR_CLOCK;
    return;
  }

  SessionRecord candidate = s_record;
  WakeupId old_id = candidate.wakeup_id;
  candidate.lifecycle = SESSION_PAUSED;
  candidate.deadline_utc = 0;
  candidate.remaining_seconds = (int32_t)remaining;
  candidate.wakeup_id = (WakeupId)-1;
  if (!prv_next_generation(candidate.generation, &candidate.generation)) {
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  if (!session_store_save(&candidate)) {
    prv_commit_failure(true);
    return;
  }
  s_record = candidate;
  if (old_id >= 0) {
    wakeup_cancel(old_id);
  }
  s_error = SESSION_ERROR_NONE;
}

static void prv_resume(void) {
  time_t now;
  if (s_record.lifecycle == SESSION_RUNNING) {
    if (!prv_read_utc(&now)) {
      s_error = SESSION_ERROR_CLOCK;
      return;
    }
    if (!prv_reconcile_before_command(now)) {
      return;
    }
  }
  if (s_record.lifecycle != SESSION_PAUSED) {
    return;
  }
  if (!prv_read_utc(&now) || (uint64_t)now > (uint64_t)INT32_MAX -
                                             (uint32_t)s_record.remaining_seconds) {
    s_error = SESSION_ERROR_CLOCK;
    return;
  }

  SessionRecord candidate = s_record;
  candidate.lifecycle = SESSION_RUNNING;
  candidate.deadline_utc = (int64_t)now + candidate.remaining_seconds;
  candidate.remaining_seconds = 0;
  if (!prv_next_generation(candidate.generation, &candidate.generation)) {
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  WakeupId candidate_id;
  if (!prv_schedule(&candidate, &candidate_id)) {
    s_error = SESSION_ERROR_WAKEUP;
    return;
  }
  candidate.wakeup_id = candidate_id;
  if (!session_store_save(&candidate)) {
    wakeup_cancel(candidate_id);
    prv_commit_failure(true);
    return;
  }
  s_record = candidate;
  s_error = SESSION_ERROR_NONE;
}

static void prv_stop(void) {
  if (s_record.lifecycle != SESSION_RUNNING && s_record.lifecycle != SESSION_PAUSED) {
    return;
  }
  SessionRecord candidate = s_record;
  WakeupId old_id = s_record.wakeup_id;
  candidate.lifecycle = SESSION_INACTIVE;
  candidate.phase = SESSION_PHASE_NONE;
  if (!prv_next_generation(candidate.generation, &candidate.generation)) {
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  candidate.deadline_utc = 0;
  candidate.remaining_seconds = 0;
  candidate.wakeup_id = (WakeupId)-1;
  if (!session_store_save(&candidate)) {
    prv_commit_failure(true);
    return;
  }
  s_record = candidate;
  if (old_id >= 0) {
    wakeup_cancel(old_id);
  }
  s_error = SESSION_ERROR_NONE;
}

static void prv_recover(void) {
  SessionRecord loaded;
  bool found;
  wakeup_cancel_all();
  if (!session_store_load(&loaded, &found)) {
    memset(&s_record, 0, sizeof(s_record));
    s_record.generation = 1;
    s_record.wakeup_id = (WakeupId)-1;
    s_error = SESSION_ERROR_RECORD;
    return;
  }
  if (!found) {
    memset(&s_record, 0, sizeof(s_record));
    s_record.generation = 1;
    s_record.wakeup_id = (WakeupId)-1;
    s_error = SESSION_ERROR_NONE;
    return;
  }
  s_record = loaded;
  s_error = SESSION_ERROR_NONE;
  time_t now;
  bool valid_time = prv_read_utc(&now);
  if (valid_time && s_record.lifecycle != SESSION_RUNNING) {
    prv_rollover_tally(now);
  }
  if (s_record.lifecycle != SESSION_RUNNING) {
    return;
  }

  if (!valid_time) {
    /* Recovery cleared all alarms, so never expose the persisted ID as scheduled. */
    s_record.wakeup_id = (WakeupId)-1;
    s_error = SESSION_ERROR_CLOCK;
    return;
  }
  /* Recovery cleared all alarms, so never expose the persisted ID as scheduled. */
  s_record.wakeup_id = (WakeupId)-1;
  if (now >= s_record.deadline_utc) {
    SessionRecord reconciled = s_record;
    prv_reconcile_phase(&reconciled, now, true);
    prv_reset_tally_day(&reconciled, prv_local_day(now));
    if (!prv_next_generation(reconciled.generation, &reconciled.generation)) {
      s_error = SESSION_ERROR_STORAGE;
      return;
    }
    reconciled.wakeup_id = (WakeupId)-1;
    WakeupId id;
    if (!prv_schedule(&reconciled, &id)) {
      s_error = SESSION_ERROR_WAKEUP;
      return;
    }
    reconciled.wakeup_id = id;
    reconciled.haptic_generation = reconciled.generation;
    if (!session_store_save(&reconciled)) {
      wakeup_cancel(id);
      s_error = SESSION_ERROR_STORAGE;
      return;
    }
    s_record = reconciled;
    prv_request_haptic();
    s_error = SESSION_ERROR_NONE;
    return;
  }
  WakeupId id;
  SessionRecord candidate = s_record;
  prv_reset_tally_day(&candidate, prv_local_day(now));
  /* cancel_all already removed the stored Wakeup; never present it as scheduled. */
  s_record.wakeup_id = (WakeupId)-1;
  if (!prv_schedule(&candidate, &id)) {
    s_error = SESSION_ERROR_WAKEUP;
    return;
  }
  candidate.wakeup_id = id;
  if (!session_store_save(&candidate)) {
    wakeup_cancel(id);
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  s_record = candidate;
}

static SessionPhase prv_next_phase(SessionPhase phase) {
  return phase == SESSION_PHASE_SIT ? SESSION_PHASE_STAND :
         phase == SESSION_PHASE_STAND ? SESSION_PHASE_MOVE : SESSION_PHASE_SIT;
}

static int32_t prv_duration(SessionPhase phase) {
  return phase == SESSION_PHASE_SIT ? 45 * 60 :
         phase == SESSION_PHASE_STAND ? 10 * 60 : 5 * 60;
}

static int32_t prv_local_day(time_t now) {
  struct tm *local = localtime(&now);
  return local ? (local->tm_year + 1900) * 1000 + local->tm_yday : 0;
}

static void prv_count_completion(SessionRecord *record, SessionPhase phase, int64_t completed_at,
                                 int32_t today) {
  time_t completion_time = (time_t)completed_at;
  if (today == 0 || prv_local_day(completion_time) != today) {
    return;
  }
  if (record->tally_day != today) {
    record->tally_day = today;
    record->sit_count = 0;
    record->stand_count = 0;
    record->move_count = 0;
  }
  uint16_t *count = phase == SESSION_PHASE_SIT ? &record->sit_count :
                    phase == SESSION_PHASE_STAND ? &record->stand_count : &record->move_count;
  if (*count < 999) {
    ++*count;
  }
}

static void prv_reset_tally_day(SessionRecord *record, int32_t day) {
  if (day == 0 || day == record->tally_day) {
    return;
  }
  record->tally_day = day;
  record->sit_count = 0;
  record->stand_count = 0;
  record->move_count = 0;
}

static void prv_rollover_tally(time_t now) {
  int32_t day = prv_local_day(now);
  if (day == 0 || day == s_record.tally_day) {
    return;
  }
  SessionRecord candidate = s_record;
  candidate.tally_day = day;
  candidate.sit_count = 0;
  candidate.stand_count = 0;
  candidate.move_count = 0;
  if (!session_store_save(&candidate)) {
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  s_record = candidate;
}

static void prv_wakeup(const SessionEvent *event) {
  if (s_record.lifecycle != SESSION_RUNNING || event->cookie != (int32_t)s_record.generation ||
      event->wakeup_id != s_record.wakeup_id) {
    return;
  }
  time_t now;
  if (!prv_read_utc(&now)) {
    s_error = SESSION_ERROR_CLOCK;
    wakeup_cancel(event->wakeup_id);
    s_record.wakeup_id = (WakeupId)-1;
    return;
  }
  if (now < s_record.deadline_utc) {
    SessionRecord candidate = s_record;
    WakeupId old_id = s_record.wakeup_id;
    WakeupId id;
    if (!prv_schedule(&candidate, &id)) {
      s_error = SESSION_ERROR_WAKEUP;
      return;
    }
    candidate.wakeup_id = id;
    if (!session_store_save(&candidate)) {
      wakeup_cancel(id);
      s_error = SESSION_ERROR_STORAGE;
      return;
    }
    s_record = candidate;
    wakeup_cancel(old_id);
    return;
  }

  SessionRecord candidate = s_record;
  prv_reconcile_phase(&candidate, now, false);
  if (!prv_next_generation(candidate.generation, &candidate.generation)) {
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  candidate.haptic_generation = candidate.generation;
  candidate.wakeup_id = (WakeupId)-1;
  candidate.remaining_seconds = 0;
  WakeupId id;
  if (!prv_schedule(&candidate, &id)) {
    s_error = SESSION_ERROR_WAKEUP;
    return;
  }
  WakeupId old_id = s_record.wakeup_id;
  candidate.wakeup_id = id;
  if (!session_store_save(&candidate)) {
    wakeup_cancel(id);
    s_error = SESSION_ERROR_STORAGE;
    return;
  }
  s_record = candidate;
  wakeup_cancel(old_id);
  prv_request_haptic();
  s_error = SESSION_ERROR_NONE;
}

static void prv_process(const SessionEvent *event) {
  time_t now;
  if (event->type == EVENT_RECOVER) {
    prv_recover();
    return;
  }
  if (event->type == EVENT_WAKEUP) {
    prv_wakeup(event);
    return;
  }
  if (event->type == EVENT_MENU_SELECT) {
    if (event->row == 0 && s_record.lifecycle == SESSION_INACTIVE) {
      prv_start();
    } else if (event->row == 0 && s_record.lifecycle == SESSION_RUNNING) {
      prv_pause();
    } else if (event->row == 0 && s_record.lifecycle == SESSION_PAUSED) {
      prv_resume();
    } else if (event->row == 2 && (s_record.lifecycle == SESSION_RUNNING ||
                                   s_record.lifecycle == SESSION_PAUSED)) {
      if (s_record.lifecycle == SESSION_RUNNING && s_error != SESSION_ERROR_WAKEUP &&
          s_error != SESSION_ERROR_STORAGE && s_record.deadline_utc > 0 &&
          prv_read_utc(&now) && now >= s_record.deadline_utc) {
        SessionEvent due = { .type = EVENT_WAKEUP, .wakeup_id = s_record.wakeup_id,
                             .cookie = (int32_t)s_record.generation };
        prv_wakeup(&due);
      }
      prv_stop();
    }
    return;
  }
  if (event->type == EVENT_DAY_ROLLOVER) {
    if (prv_read_utc(&now)) {
      prv_rollover_tally(now);
    }
    return;
  }
  if (event->type == EVENT_CLOCK_CHECK) {
    s_clock_check_queued = false;
    if (s_record.lifecycle == SESSION_RUNNING) {
      time_t clock_now;
      if (!prv_read_utc(&clock_now)) {
        bool was_clock_error = s_error == SESSION_ERROR_CLOCK;
        s_error = SESSION_ERROR_CLOCK;
        if (!was_clock_error && s_record.wakeup_id >= 0) {
          wakeup_cancel(s_record.wakeup_id);
          s_record.wakeup_id = (WakeupId)-1;
        }
      } else if (s_error == SESSION_ERROR_CLOCK) {
        prv_recover();
      } else if ((s_error == SESSION_ERROR_NONE || s_error == SESSION_ERROR_QUEUE) &&
                 clock_now >= s_record.deadline_utc &&
                 s_record.wakeup_id >= 0) {
        SessionEvent due = { .type = EVENT_WAKEUP, .wakeup_id = s_record.wakeup_id,
                             .cookie = (int32_t)s_record.generation };
        prv_wakeup(&due);
      }
    }
    return;
  }
  if (event->type == EVENT_START) {
    if (s_record.lifecycle == SESSION_RUNNING && s_error != SESSION_ERROR_WAKEUP &&
        s_error != SESSION_ERROR_STORAGE && s_record.deadline_utc > 0 &&
        prv_read_utc(&now) && now >= s_record.deadline_utc) {
      SessionEvent due = { .type = EVENT_WAKEUP, .wakeup_id = s_record.wakeup_id,
                           .cookie = (int32_t)s_record.generation };
      prv_wakeup(&due);
    }
    prv_start();
  } else if (event->type == EVENT_PAUSE) {
    prv_pause();
  } else if (event->type == EVENT_RESUME) {
    prv_resume();
  } else if (event->type == EVENT_STOP) {
    if (s_record.lifecycle == SESSION_RUNNING && s_error != SESSION_ERROR_WAKEUP &&
        s_error != SESSION_ERROR_STORAGE && s_record.deadline_utc > 0 &&
        prv_read_utc(&now) && now >= s_record.deadline_utc) {
      SessionEvent due = { .type = EVENT_WAKEUP, .wakeup_id = s_record.wakeup_id,
                           .cookie = (int32_t)s_record.generation };
      prv_wakeup(&due);
    }
    prv_stop();
  }
}

static void prv_drain(void *context) {
  (void)context;
  s_drain_timer = NULL;
  while (s_queue_count) {
    SessionEvent event = s_queue[s_queue_read];
    s_queue_read = (s_queue_read + 1) % SESSION_QUEUE_SIZE;
    --s_queue_count;
    prv_process(&event);
  }
}

static bool prv_enqueue(SessionEvent event) {
  if (s_queue_count == SESSION_QUEUE_SIZE) {
    s_error = SESSION_ERROR_QUEUE;
    return false;
  }
  uint8_t write = (s_queue_read + s_queue_count) % SESSION_QUEUE_SIZE;
  s_queue[write] = event;
  ++s_queue_count;
  if (!s_drain_timer) {
    s_drain_timer = app_timer_register(1, prv_drain, NULL);
    if (!s_drain_timer) {
      prv_drain(NULL);
    }
  }
  return true;
}

void session_coordinator_init(void) {
  memset(&s_record, 0, sizeof(s_record));
  s_record.generation = 1;
  s_record.wakeup_id = (WakeupId)-1;
  s_error = SESSION_ERROR_NONE;
  s_queue_read = 0;
  s_queue_count = 0;
  s_drain_timer = NULL;
  s_rollover_requested_day = 0;
  s_clock_check_queued = false;
  wakeup_service_subscribe(session_coordinator_wakeup);
  session_coordinator_recover();
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    WakeupId id;
    int32_t cookie;
    if (wakeup_get_launch_event(&id, &cookie)) {
      session_coordinator_wakeup(id, cookie);
    }
  }
}

void session_coordinator_deinit(void) {
  if (s_drain_timer) {
    app_timer_cancel(s_drain_timer);
    s_drain_timer = NULL;
  }
}

void session_coordinator_start(void) {
  prv_enqueue((SessionEvent){ .type = EVENT_START });
}

void session_coordinator_pause(void) {
  prv_enqueue((SessionEvent){ .type = EVENT_PAUSE });
}

void session_coordinator_resume(void) {
  prv_enqueue((SessionEvent){ .type = EVENT_RESUME });
}

bool session_coordinator_resume_if_paused(void) {
  if (s_record.lifecycle != SESSION_PAUSED) {
    return false;
  }
  session_coordinator_resume();
  return true;
}

void session_coordinator_stop(void) {
  prv_enqueue((SessionEvent){ .type = EVENT_STOP });
}

void session_coordinator_select_menu_row(uint16_t row) {
  prv_enqueue((SessionEvent){ .type = EVENT_MENU_SELECT, .row = row });
}

void session_coordinator_recover(void) {
  prv_enqueue((SessionEvent){ .type = EVENT_RECOVER });
}

void session_coordinator_wakeup(WakeupId id, int32_t cookie) {
  prv_enqueue((SessionEvent){ .type = EVENT_WAKEUP, .wakeup_id = id,
                              .cookie = cookie });
}

void session_coordinator_get_view(SessionView *view) {
  time_t now = time(NULL);
  if (s_record.lifecycle == SESSION_RUNNING && !s_clock_check_queued) {
    s_clock_check_queued = true;
    if (!prv_enqueue((SessionEvent){ .type = EVENT_CLOCK_CHECK })) {
      s_clock_check_queued = false;
    }
  }
  if (prv_valid_utc(now)) {
    int32_t day = prv_local_day(now);
    if (day != 0 && day != s_record.tally_day && day != s_rollover_requested_day) {
      s_rollover_requested_day = day;
      if (!prv_enqueue((SessionEvent){ .type = EVENT_DAY_ROLLOVER })) {
        s_rollover_requested_day = 0;
      }
    }
  }
  view->record = s_record;
  view->error = s_error;
  view->remaining_seconds = 0;
  if (s_record.lifecycle == SESSION_RUNNING) {
    if (prv_valid_utc(now) && now < s_record.deadline_utc) {
      view->remaining_seconds = (int32_t)(s_record.deadline_utc - now);
    }
  } else if (s_record.lifecycle == SESSION_PAUSED) {
    view->remaining_seconds = s_record.remaining_seconds;
  }
}
