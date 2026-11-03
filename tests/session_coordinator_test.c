#define _POSIX_C_SOURCE 200112L
#include "session_coordinator.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORE_KEY 0x5353

struct AppTimer {
  AppTimerCallback callback;
  void *context;
  bool pending;
};

static uint8_t s_persisted[64];
static int s_persisted_size;
static int s_write_attempts;
static struct AppTimer s_timer;
static WakeupHandler s_wakeup_handler;
static WakeupId s_next_wakeup_id;
static int s_launch_reason;
static bool s_launch_event_available;
static WakeupId s_launch_wakeup_id;
static int32_t s_launch_cookie;

time_t fake_now;
bool fake_fail_schedule;
int fake_schedule_error;
int32_t fake_last_scheduled_cookie;
bool fake_fail_write;
int fake_schedule_count;
int fake_cancel_count;
int fake_cancel_all_count;
WakeupId fake_last_cancelled_id;
WakeupId fake_last_scheduled_id;
time_t fake_last_scheduled_deadline;
int fake_haptic_count;
char fake_log[4096];

time_t fake_time(time_t *result) {
  if (result) {
    *result = fake_now;
  }
  return fake_now;
}

int persist_get_size(uint32_t key) {
  return key == STORE_KEY && s_persisted_size ? s_persisted_size : E_DOES_NOT_EXIST;
}

int persist_read_data(uint32_t key, void *data, size_t size) {
  if (key != STORE_KEY || s_persisted_size != (int)size) {
    return -1;
  }
  memcpy(data, s_persisted, size);
  return s_persisted_size;
}

int persist_write_data(uint32_t key, const void *data, size_t size) {
  ++s_write_attempts;
  if (key != STORE_KEY || fake_fail_write || size > sizeof(s_persisted)) {
    return -1;
  }
  memcpy(s_persisted, data, size);
  s_persisted_size = (int)size;
  return s_persisted_size;
}

int persist_delete(uint32_t key) {
  if (key != STORE_KEY || !s_persisted_size) {
    return E_DOES_NOT_EXIST;
  }
  s_persisted_size = 0;
  return S_SUCCESS;
}

WakeupId wakeup_schedule(time_t timestamp, int32_t cookie, bool notify_if_missed) {
  assert(!notify_if_missed);
  ++fake_schedule_count;
  if (fake_fail_schedule) {
    return (WakeupId)(fake_schedule_error ? fake_schedule_error : -1);
  }
  fake_last_scheduled_cookie = cookie;
  fake_last_scheduled_id = s_next_wakeup_id++;
  fake_last_scheduled_deadline = timestamp;
  return fake_last_scheduled_id;
}

void wakeup_cancel(WakeupId id) {
  fake_last_cancelled_id = id;
  ++fake_cancel_count;
}

void wakeup_cancel_all(void) {
  ++fake_cancel_all_count;
}

void fake_app_log(const char *format, ...) {
  size_t used = strlen(fake_log);
  if (used + 2 >= sizeof(fake_log)) {
    return;
  }
  va_list args;
  va_start(args, format);
  vsnprintf(fake_log + used, sizeof(fake_log) - used - 1, format, args);
  va_end(args);
  used = strlen(fake_log);
  fake_log[used] = '\n';
  fake_log[used + 1] = '\0';
}

void vibes_short_pulse(void) {
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.haptic_generation == stored.generation);
  ++fake_haptic_count;
}

void wakeup_service_subscribe(WakeupHandler handler) {
  s_wakeup_handler = handler;
}

bool wakeup_get_launch_event(WakeupId *id, int32_t *cookie) {
  if (!s_launch_event_available) {
    return false;
  }
  *id = s_launch_wakeup_id;
  *cookie = s_launch_cookie;
  return true;
}

int launch_reason(void) {
  return s_launch_reason;
}

AppTimer *app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void *context) {
  (void)timeout_ms;
  s_timer.callback = callback;
  s_timer.context = context;
  s_timer.pending = true;
  return &s_timer;
}

void app_timer_cancel(AppTimer *timer) {
  if (timer) {
    timer->pending = false;
  }
}

void fake_run_timers(void) {
  while (s_timer.pending) {
    AppTimerCallback callback = s_timer.callback;
    void *context = s_timer.context;
    s_timer.pending = false;
    callback(context);
  }
}

void fake_deliver_wakeup(WakeupId id, int32_t cookie) {
  assert(s_wakeup_handler);
  s_wakeup_handler(id, cookie);
  fake_run_timers();
}

void fake_reset(void) {
  setenv("TZ", "UTC", 1);
  tzset();
  memset(s_persisted, 0, sizeof(s_persisted));
  s_persisted_size = 0;
  s_write_attempts = 0;
  memset(&s_timer, 0, sizeof(s_timer));
  s_wakeup_handler = NULL;
  s_next_wakeup_id = 1;
  s_launch_reason = 0;
  s_launch_event_available = false;
  s_launch_wakeup_id = (WakeupId)-1;
  s_launch_cookie = 0;
  fake_now = 1800000000;
  fake_fail_schedule = false;
  fake_schedule_error = 0;
  fake_last_scheduled_cookie = 0;
  fake_fail_write = false;
  fake_schedule_count = 0;
  fake_cancel_count = 0;
  fake_cancel_all_count = 0;
  fake_last_cancelled_id = (WakeupId)-2;
  fake_last_scheduled_id = (WakeupId)-1;
  fake_last_scheduled_deadline = 0;
  fake_haptic_count = 0;
  fake_log[0] = '\0';
}

static void prv_start_session(void) {
  session_coordinator_init();
  fake_run_timers();
  session_coordinator_start();
  fake_run_timers();
}

static void prv_check(bool ok, const char *what) {
  if (!ok) {
    fprintf(stderr, "test setup failed: %s\n", what);
    abort();
  }
}

static void prv_save(const SessionRecord *record) {
  bool ok = session_store_save(record);
  prv_check(ok, "session_store_save");
}

static SessionView prv_view(void) {
  SessionView view;
  session_coordinator_get_view(&view);
  return view;
}

static void test_start_and_stop(void) {
  fake_reset();
  session_coordinator_init();
  fake_run_timers();
  session_coordinator_select_menu_row(0);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == fake_now + 45 * 60);
  assert(fake_last_scheduled_deadline == view.record.deadline_utc);
  assert(fake_schedule_count == 1);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_haptic_count == 0);
  assert(s_persisted_size == 45);

  WakeupId old_id = view.record.wakeup_id;
  int32_t old_generation = (int32_t)view.record.generation;
  session_coordinator_select_menu_row(2);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_cancel_count == 1);
  assert(fake_last_cancelled_id == old_id);

  fake_deliver_wakeup(old_id, old_generation);
  assert(prv_view().record.lifecycle == SESSION_INACTIVE);
}

static void test_stale_callback_is_ignored(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  int schedules = fake_schedule_count;
  fake_deliver_wakeup(view.record.wakeup_id + 1, (int32_t)view.record.generation);
  fake_deliver_wakeup(view.record.wakeup_id, (int32_t)view.record.generation + 1);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(fake_schedule_count == schedules);
  assert(fake_haptic_count == 0);
  assert(view.error == SESSION_ERROR_NONE);
}

static void test_clock_schedule_and_storage_failures(void) {
  fake_reset();
  session_coordinator_init();
  fake_run_timers();
  fake_now = 0;
  session_coordinator_start();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.error == SESSION_ERROR_CLOCK);

  fake_now = 1800000000;
  fake_fail_schedule = true;
  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.error == SESSION_ERROR_WAKEUP);

  fake_fail_schedule = false;
  fake_fail_write = true;
  int cancelled_before = fake_cancel_count;
  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_cancel_count == cancelled_before + 1);
  assert(fake_last_cancelled_id == fake_last_scheduled_id);

  fake_fail_write = false;
  fake_now = (time_t)INT32_MAX - 45 * 60;
  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(view.record.wakeup_id, (int32_t)view.record.generation);
  view = prv_view();
  assert(view.error == SESSION_ERROR_WAKEUP);
}

static void test_stop_storage_failure_keeps_committed_session(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId committed_id = view.record.wakeup_id;
  fake_fail_write = true;
  session_coordinator_stop();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.wakeup_id == committed_id);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_cancel_count == 0);
}

static void test_deadline_callback_and_command_reconciliation(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId first_id = view.record.wakeup_id;
  int32_t first_generation = (int32_t)view.record.generation;
  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(first_id, first_generation);
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(view.record.sit_count == 1);
  assert(view.record.haptic_generation == view.record.generation);
  assert(fake_haptic_count == 1);
  assert(fake_last_cancelled_id == first_id);
  WakeupId second_id = view.record.wakeup_id;

  fake_deliver_wakeup(first_id, first_generation);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(fake_haptic_count == 1);

  fake_now = (time_t)view.record.deadline_utc;
  session_coordinator_stop();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(fake_schedule_count == 3);
  assert(fake_cancel_count == 3);
  /* The due Stand Wakeup is replaced by Move's, and Stop then cancels Move's. */
  assert(fake_last_scheduled_id != second_id);
  assert(fake_last_cancelled_id == fake_last_scheduled_id);
}

static void test_forward_clock_change_reconciles_due_deadline(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  time_t deadline = (time_t)view.record.deadline_utc;
  int schedules = fake_schedule_count;
  fake_now = deadline + 1;

  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(view.record.deadline_utc == deadline + 10 * 60);
  assert(fake_schedule_count == schedules + 1);
  assert(fake_haptic_count == 1);
}

static void test_backward_clock_change_defers_deadline(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  time_t deadline = (time_t)view.record.deadline_utc;
  int schedules = fake_schedule_count;
  fake_now -= 60 * 60;

  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == deadline);
  assert(fake_schedule_count == schedules);
  assert(fake_haptic_count == 0);
}

static void test_full_queue_does_not_suppress_later_clock_check(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId id = view.record.wakeup_id;
  int32_t cookie = (int32_t)view.record.generation;
  time_t deadline = (time_t)view.record.deadline_utc;

  for (int i = 0; i < 8; ++i) {
    session_coordinator_wakeup(id + 100, cookie + 1);
  }
  fake_now = deadline;
  (void)prv_view();
  fake_run_timers();
  (void)prv_view();
  fake_run_timers();

  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(fake_haptic_count == 1);
}

static void test_full_queue_does_not_suppress_tally_rollover(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_INACTIVE,
    .phase = SESSION_PHASE_NONE,
    .generation = 3,
    .wakeup_id = (WakeupId)-1,
    .tally_day = 1,
    .sit_count = 2,
    .stand_count = 3,
    .move_count = 4,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  fake_now += 24 * 3600;
  for (int i = 0; i < 8; ++i) {
    session_coordinator_stop();
  }
  (void)prv_view();
  fake_run_timers();
  (void)prv_view();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.tally_day != 1);
  assert(view.record.sit_count == 0 && view.record.stand_count == 0 && view.record.move_count == 0);
}

static void test_natural_cycle_tally_and_haptics_persist(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId id = view.record.wakeup_id;
  int32_t cookie = (int32_t)view.record.generation;

  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(id, cookie);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1 && view.record.stand_count == 0 && view.record.move_count == 0);
  assert(fake_haptic_count == 1);

  id = view.record.wakeup_id;
  cookie = (int32_t)view.record.generation;
  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(id, cookie);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_MOVE);
  assert(view.record.sit_count == 1 && view.record.stand_count == 1 && view.record.move_count == 0);
  assert(fake_haptic_count == 2);

  id = view.record.wakeup_id;
  cookie = (int32_t)view.record.generation;
  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(id, cookie);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 1 && view.record.stand_count == 1 && view.record.move_count == 1);
  assert(fake_haptic_count == 3);

  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.sit_count == 1 && stored.stand_count == 1 && stored.move_count == 1);
  session_coordinator_deinit();
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.record.sit_count == 1 && view.record.stand_count == 1 && view.record.move_count == 1);
  assert(fake_haptic_count == 3);
}

static void test_tally_persists_across_stop_and_start(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(view.record.wakeup_id, (int32_t)view.record.generation);
  view = prv_view();
  assert(view.record.sit_count == 1);

  session_coordinator_stop();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.record.sit_count == 1);

  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 1);
}

static void test_invalid_then_valid_utc_recovers_before_scheduling(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  SessionRecord committed = view.record;
  int schedules = fake_schedule_count;
  int cancels = fake_cancel_count;
  fake_now = 0;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == committed.phase);
  assert(view.record.deadline_utc == committed.deadline_utc);
  assert(fake_schedule_count == schedules);
  assert(fake_cancel_count == cancels + 1);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_haptic_count == 0);

  fake_now = (time_t)committed.deadline_utc - 1;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.phase == committed.phase);
  assert(view.record.deadline_utc == committed.deadline_utc);
  assert(fake_schedule_count == schedules + 1);
  assert(fake_haptic_count == 0);

  fake_now = (time_t)committed.deadline_utc;
  fake_deliver_wakeup(view.record.wakeup_id, (int32_t)view.record.generation);
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(fake_schedule_count == schedules + 2);
  assert(fake_haptic_count == 1);
}

static void test_failed_transition_has_no_tally_or_haptic(void) {
  fake_reset();
  prv_start_session();
  SessionView committed = prv_view();
  WakeupId id = committed.record.wakeup_id;
  int32_t cookie = (int32_t)committed.record.generation;
  fake_now = (time_t)committed.record.deadline_utc;

  fake_fail_schedule = true;
  fake_deliver_wakeup(id, cookie);
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 0);
  assert(fake_haptic_count == 0);

  fake_fail_schedule = false;
  fake_fail_write = true;
  (void)prv_view();
  fake_run_timers();
  fake_deliver_wakeup(id, cookie);
  view = prv_view();
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 0);
  assert(fake_haptic_count == 0);

  /* Start, the failed Wakeup transition, and the clock-check repair whose save failed. */
  assert(fake_schedule_count == 3);
  fake_fail_write = false;
  session_coordinator_recover();
  fake_run_timers();
  view = prv_view();
  /* The pending clock check repairs the lost Wakeup and commits Stand (4); the explicit recovery
   * then only reschedules the same committed deadline (5). */
  assert(fake_schedule_count == 5);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(fake_haptic_count == 1);
}

static void test_launch_repair_and_invalid_record(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 7,
    .deadline_utc = fake_now + 45 * 60,
    .wakeup_id = 88,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(fake_cancel_all_count == 1);
  assert(fake_schedule_count == 1);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.generation == 7);
  assert(view.record.wakeup_id == fake_last_scheduled_id);

  session_coordinator_deinit();
  fake_reset();
  record.deadline_utc = fake_now - 30 * 60;
  record.wakeup_id = 89;
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == fake_now + 45 * 60);
  assert(view.record.generation == 8);
  session_coordinator_deinit();

  fake_reset();
  uint8_t invalid_record[] = { 0, 1, 2 };
  int written = persist_write_data(STORE_KEY, invalid_record, sizeof(invalid_record));
  assert(written == (int)sizeof(invalid_record));
  (void)written;
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.error == SESSION_ERROR_RECORD);
  assert(fake_cancel_all_count == 1);
}

static void test_schedule_error_codes_and_cookie(void) {
  fake_reset();
  session_coordinator_init();
  fake_run_timers();
  fake_fail_schedule = true;
  fake_schedule_error = E_RANGE;
  session_coordinator_start();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(s_persisted_size == 0);

  fake_fail_schedule = false;
  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(fake_last_scheduled_cookie == (int32_t)view.record.generation);
}

static void test_rollover_is_requested_once_per_day(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_INACTIVE,
    .phase = SESSION_PHASE_NONE,
    .generation = 3,
    .wakeup_id = (WakeupId)-1,
    .tally_day = 1,
    .sit_count = 2,
    .stand_count = 3,
    .move_count = 4,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.tally_day != 1);
  assert(view.record.sit_count == 0 && view.record.stand_count == 0 && view.record.move_count == 0);

  fake_reset();
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  fake_fail_write = true;
  fake_now += 24 * 3600;
  for (int i = 0; i < 20; ++i) {
    (void)prv_view();
  }
  int attempts_before = s_write_attempts;
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(view.record.tally_day != 1);
  assert(s_write_attempts == attempts_before + 1);
  fake_run_timers();
  assert(s_write_attempts == attempts_before + 1);
}

static void test_recovery_failure_is_not_presented_as_scheduled(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 7,
    .deadline_utc = fake_now + 45 * 60,
    .wakeup_id = 88,
  };
  prv_save(&record);
  fake_fail_schedule = true;
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(view.record.wakeup_id == (WakeupId)-1);
}

static void test_store_clear_reports_status(void) {
  fake_reset();
  bool cleared_missing = session_store_clear();
  assert(!cleared_missing);
  (void)cleared_missing;
  SessionRecord record = { .generation = 1, .wakeup_id = (WakeupId)-1 };
  prv_save(&record);
  bool cleared = session_store_clear();
  assert(cleared);
  (void)cleared;
}

static int32_t prv_expected_duration(SessionPhase phase) {
  return phase == SESSION_PHASE_SIT ? 45 * 60 : phase == SESSION_PHASE_STAND ? 10 * 60 : 5 * 60;
}

/* Independent model: the phase that follows the end of a Sit, `x` seconds after that end. */
static SessionPhase prv_phase_after_sit_end(int64_t x) {
  int64_t pos = x % 3600;
  return pos < 600 ? SESSION_PHASE_STAND : pos < 900 ? SESSION_PHASE_MOVE : SESSION_PHASE_SIT;
}

static void test_wakeup_catch_up_table(void) {
  static const int64_t offsets[] = { 0, 1, 599, 600, 899, 900, 3599, 3600, 3600 + 650,
                                     2 * 3600 + 100, 5 * 3600 + 1000, 40 * 3600 + 3000 };
  for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
    fake_reset();
    prv_start_session();
    SessionView view = prv_view();
    int64_t sit_end = view.record.deadline_utc;
    WakeupId id = view.record.wakeup_id;
    uint32_t generation = view.record.generation;
    fake_now = (time_t)(sit_end + offsets[i]);
    fake_deliver_wakeup(id, (int32_t)generation);
    view = prv_view();
    SessionPhase expected = prv_phase_after_sit_end(offsets[i]);
    assert(view.error == SESSION_ERROR_NONE);
    assert(view.record.phase == expected);
    assert(view.record.generation == generation + 1);
    assert(view.record.wakeup_id == fake_last_scheduled_id);
    assert(fake_last_scheduled_cookie == (int32_t)view.record.generation);
    assert(view.record.deadline_utc > fake_now);
    if (offsets[i] < 600) {
      assert(view.record.deadline_utc == sit_end + 600);
    } else {
      assert(view.record.deadline_utc == fake_now + prv_expected_duration(expected));
    }
    assert(fake_haptic_count == 1);
    assert(fake_last_scheduled_deadline == view.record.deadline_utc);
  }
}

static void test_overdue_recovery_table(void) {
  static const int64_t offsets[] = { 0, 599, 600, 899, 900, 3599, 3600, 3600 + 650, 7 * 3600 + 1000 };
  for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
    fake_reset();
    SessionRecord record = {
      .lifecycle = SESSION_RUNNING,
      .phase = SESSION_PHASE_SIT,
      .generation = 4,
      .deadline_utc = fake_now - offsets[i],
      .wakeup_id = 55,
    };
    prv_save(&record);
    session_coordinator_init();
    fake_run_timers();
    SessionView view = prv_view();
    SessionPhase expected = prv_phase_after_sit_end(offsets[i]);
    assert(view.error == SESSION_ERROR_NONE);
    assert(view.record.lifecycle == SESSION_RUNNING);
    assert(view.record.phase == expected);
    assert(view.record.generation == 5);
    assert(view.record.deadline_utc == fake_now + prv_expected_duration(expected));
    assert(view.record.wakeup_id == fake_last_scheduled_id);
    assert(fake_last_scheduled_cookie == 5);
    session_coordinator_deinit();
  }
}

static void test_recovery_counts_each_elapsed_phase_once(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = fake_now - 3600,
    .wakeup_id = 55,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 2);
  assert(view.record.stand_count == 1);
  assert(view.record.move_count == 1);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(fake_haptic_count == 1);
}

static void test_wakeup_launch_reconciles_and_renders_committed_state(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = fake_now - 10,
    .wakeup_id = 55,
  };
  prv_save(&record);
  s_launch_reason = APP_LAUNCH_WAKEUP;
  s_launch_event_available = true;
  s_launch_wakeup_id = record.wakeup_id;
  s_launch_cookie = (int32_t)record.generation;
  session_coordinator_init();
  fake_run_timers();

  SessionView view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.remaining_seconds == 10 * 60);
  assert(fake_haptic_count == 1);

  session_coordinator_deinit();
  s_launch_event_available = false;
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(fake_haptic_count == 1);
}

static void test_recovery_counts_completions_on_local_day_only(void) {
  fake_reset();
  struct tm local = *localtime(&fake_now);
  local.tm_hour = 0;
  local.tm_min = 0;
  local.tm_sec = 0;
  time_t midnight = mktime(&local);
  fake_now = midnight + 3000;
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = midnight - 100,
    .wakeup_id = 55,
    .tally_day = 1,
    .sit_count = 3,
    .stand_count = 2,
    .move_count = 1,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  struct tm *now_local = localtime(&fake_now);
  int32_t today = (now_local->tm_year + 1900) * 1000 + now_local->tm_yday;
  assert(view.record.tally_day == today);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 0);
  assert(view.record.stand_count == 1);
  assert(view.record.move_count == 1);
}

static void test_launch_repair_rolls_over_running_tally(void) {
  fake_reset();
  struct tm local = *localtime(&fake_now);
  local.tm_hour = 0;
  local.tm_min = 0;
  local.tm_sec = 0;
  time_t midnight = mktime(&local);
  fake_now = midnight + 300;
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = fake_now + 45 * 60,
    .wakeup_id = 55,
    .tally_day = 1,
    .sit_count = 3,
    .stand_count = 2,
    .move_count = 1,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();

  SessionView view = prv_view();
  struct tm *now_local = localtime(&fake_now);
  int32_t today = (now_local->tm_year + 1900) * 1000 + now_local->tm_yday;
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.tally_day == today);
  assert(view.record.sit_count == 0 && view.record.stand_count == 0 && view.record.move_count == 0);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
}

static void test_long_recovery_tallies_only_current_local_day(void) {
  fake_reset();
  struct tm local = *localtime(&fake_now);
  local.tm_hour = 0;
  local.tm_min = 0;
  local.tm_sec = 0;
  time_t midnight = mktime(&local);
  fake_now = midnight + 12 * 3600;
  int32_t today = (local.tm_year + 1900) * 1000 + local.tm_yday;
  uint16_t expected[3] = { 0, 0, 0 }; /* Sit, Stand, Move */
  int64_t deadline = midnight - 40 * 3600;
  SessionPhase phase = SESSION_PHASE_SIT;
  while (deadline <= fake_now) {
    time_t completed_at = (time_t)deadline;
    struct tm *completion_day = localtime(&completed_at);
    if (completion_day && (completion_day->tm_year + 1900) * 1000 + completion_day->tm_yday == today) {
      int slot = phase == SESSION_PHASE_SIT ? 0 : phase == SESSION_PHASE_STAND ? 1 : 2;
      ++expected[slot];
    }
    phase = phase == SESSION_PHASE_SIT ? SESSION_PHASE_STAND :
            phase == SESSION_PHASE_STAND ? SESSION_PHASE_MOVE : SESSION_PHASE_SIT;
    deadline += phase == SESSION_PHASE_SIT ? 45 * 60 :
                phase == SESSION_PHASE_STAND ? 10 * 60 : 5 * 60;
  }

  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = midnight - 40 * 3600,
    .wakeup_id = 55,
    .tally_day = 1,
    .sit_count = 3,
    .stand_count = 2,
    .move_count = 1,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.tally_day == today);
  assert(view.record.sit_count == expected[0]);
  assert(view.record.stand_count == expected[1]);
  assert(view.record.move_count == expected[2]);
}

static void test_overdue_recovery_on_dst_change_day_counts_whole_local_day(void) {
  fake_reset();
  setenv("TZ", "Europe/Berlin", 1); /* 2026-10-25: clocks fall back, a 25-hour day */
  tzset();
  struct tm local = { .tm_year = 126, .tm_mon = 9, .tm_mday = 25, .tm_hour = 12, .tm_isdst = -1 };
  fake_now = mktime(&local);
  local.tm_hour = 0;
  local.tm_isdst = -1;
  time_t midnight = mktime(&local);
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = midnight - 5 * 60,
    .wakeup_id = 55,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  /* From 00:00 to 12:00 local on a 25-hour day there are 13 hours: 13 full cycles of 60 minutes. */
  assert(view.record.sit_count == 13);
  assert(view.record.stand_count == 13);
  assert(view.record.move_count == 13);
  session_coordinator_deinit();
  setenv("TZ", "UTC", 1);
  tzset();
}

static void test_early_callback_reschedules_same_deadline(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId old_id = view.record.wakeup_id;
  int64_t deadline = view.record.deadline_utc;
  uint32_t generation = view.record.generation;
  fake_now = (time_t)(deadline - 100);
  int schedules = fake_schedule_count;
  fake_deliver_wakeup(old_id, (int32_t)generation);
  view = prv_view();
  assert(fake_schedule_count == schedules + 1);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == deadline);
  assert(view.record.generation == generation);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(view.record.wakeup_id != old_id);
  assert(fake_haptic_count == 0);
  SessionRecord stored;
  bool found;
  bool loaded = session_store_load(&stored, &found);
  assert(loaded && found);
  assert(stored.wakeup_id == view.record.wakeup_id);

  fake_deliver_wakeup(old_id, (int32_t)generation);
  assert(fake_schedule_count == schedules + 1);
}

static void test_stop_failure_keeps_persisted_session(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  fake_fail_write = true;
  session_coordinator_stop();
  fake_run_timers();
  SessionRecord stored;
  bool found;
  bool loaded = session_store_load(&stored, &found);
  assert(loaded && found);
  assert(stored.lifecycle == SESSION_RUNNING);
  assert(stored.wakeup_id == view.record.wakeup_id);
  assert(stored.generation == view.record.generation);

  fake_fail_write = false;
  session_coordinator_stop();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.lifecycle == SESSION_INACTIVE);
  loaded = session_store_load(&stored, &found);
  assert(loaded && found);
  assert(stored.lifecycle == SESSION_INACTIVE);
}

static void test_menu_row_contract(void) {
  fake_reset();
  session_coordinator_init();
  fake_run_timers();
  int cancels = fake_cancel_count;
  session_coordinator_select_menu_row(2);
  session_coordinator_select_menu_row(1);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(fake_schedule_count == 0);
  assert(fake_cancel_count == cancels);

  session_coordinator_select_menu_row(0);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);

  /* Skip (row 1) advances Sit to Stand while running, with one haptic. */
  int writes = s_write_attempts;
  WakeupId sit_id = view.record.wakeup_id;
  session_coordinator_select_menu_row(1);
  fake_run_timers();
  SessionView skipped = prv_view();
  assert(skipped.record.lifecycle == SESSION_RUNNING);
  assert(skipped.record.phase == SESSION_PHASE_STAND);
  assert(skipped.record.deadline_utc == fake_now + 10 * 60);
  assert(skipped.record.generation == view.record.generation + 1);
  assert(skipped.record.wakeup_id == fake_last_scheduled_id);
  assert(skipped.record.sit_count == 0);
  assert(s_write_attempts == writes + 1);
  assert(fake_schedule_count == 2);
  assert(fake_cancel_count == cancels + 1);
  assert(fake_last_cancelled_id == sit_id);
  assert(fake_haptic_count == 1);

  session_coordinator_select_menu_row(0);
  fake_run_timers();
  SessionView after = prv_view();
  assert(after.record.lifecycle == SESSION_PAUSED);
  assert(after.record.phase == SESSION_PHASE_STAND);
  assert(after.record.remaining_seconds == 10 * 60);
  assert(after.record.deadline_utc == 0);
  assert(after.record.wakeup_id == (WakeupId)-1);
  assert(after.record.generation == skipped.record.generation + 1);
  assert(fake_schedule_count == 2);
  assert(fake_cancel_count == cancels + 2);
  assert(s_write_attempts == writes + 2);
  assert(fake_haptic_count == 1);

  /* ... and from paused it leaves the pause, running Move at its full default duration. */
  session_coordinator_select_menu_row(1);
  fake_run_timers();
  after = prv_view();
  assert(after.record.lifecycle == SESSION_RUNNING);
  assert(after.record.phase == SESSION_PHASE_MOVE);
  assert(after.record.deadline_utc == fake_now + 5 * 60);
  assert(after.record.remaining_seconds == 0);
  assert(after.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_schedule_count == 3);
  assert(fake_haptic_count == 2);
}

static void test_skip_cycles_phases_without_counting_and_ignores_stale_callback(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId sit_id = view.record.wakeup_id;
  int32_t sit_cookie = (int32_t)view.record.generation;
  fake_now += 100;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.record.generation == (uint32_t)sit_cookie + 1);
  assert(view.record.haptic_generation == view.record.generation);
  assert(view.record.sit_count == 0 && view.record.stand_count == 0 && view.record.move_count == 0);
  assert(fake_haptic_count == 1);
  assert(fake_last_cancelled_id == sit_id);

  /* A callback for the skipped phase is ignored. */
  fake_deliver_wakeup(sit_id, sit_cookie);
  assert(prv_view().record.phase == SESSION_PHASE_STAND);
  assert(fake_haptic_count == 1);

  fake_now += 7;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_MOVE);
  assert(view.record.deadline_utc == fake_now + 5 * 60);
  assert(fake_haptic_count == 2);

  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == fake_now + 45 * 60);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_last_scheduled_deadline == view.record.deadline_utc);
  assert(view.record.sit_count == 0 && view.record.stand_count == 0 && view.record.move_count == 0);
  assert(fake_haptic_count == 3);
  assert(view.error == SESSION_ERROR_NONE);
}

static void test_skip_reconciles_due_deadline_first(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  fake_now = (time_t)view.record.deadline_utc;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  /* Due Sit completes (counted) into Stand, then Skip moves on from Stand to Move. */
  assert(view.record.phase == SESSION_PHASE_MOVE);
  assert(view.record.deadline_utc == fake_now + 5 * 60);
  assert(view.record.sit_count == 1);
  assert(view.record.stand_count == 0);
  assert(view.record.move_count == 0);

  /* A due timed-pause end is reconciled first, then Skip applies to the resumed phase. */
  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  view = prv_view();
  fake_now = (time_t)view.record.pause_deadline_utc;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.record.sit_count == 0);

  /* If the due reconcile fails, Skip is dropped. */
  fake_reset();
  prv_start_session();
  view = prv_view();
  fake_now = (time_t)view.record.deadline_utc;
  fake_fail_schedule = true;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 0);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);
}

static void test_skip_from_paused_and_timed_paused_starts_running_phase(void) {
  fake_reset();
  prv_start_session();
  fake_now += 60;
  session_coordinator_pause();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_PAUSED);
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.record.remaining_seconds == 0);
  assert(view.record.pause_deadline_utc == 0 && view.record.pause_duration_seconds == 0);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(view.record.sit_count == 0);
  assert(fake_haptic_count == 1);

  fake_reset();
  prv_start_session();
  fake_now += 60;
  session_coordinator_timed_pause(10 * 60);
  fake_run_timers();
  view = prv_view();
  WakeupId pause_id = view.record.wakeup_id;
  int32_t pause_cookie = (int32_t)view.record.generation;
  int cancels = fake_cancel_count;
  fake_now += 30;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.record.remaining_seconds == 0);
  assert(view.record.pause_deadline_utc == 0 && view.record.pause_duration_seconds == 0);
  assert(fake_cancel_count == cancels + 1);
  assert(fake_last_cancelled_id == pause_id);
  assert(fake_haptic_count == 1);

  /* A later pause-end callback does nothing. */
  fake_deliver_wakeup(pause_id, pause_cookie);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(fake_haptic_count == 1);
}

static void test_skip_is_noop_while_inactive(void) {
  fake_reset();
  session_coordinator_init();
  fake_run_timers();
  session_coordinator_skip();
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_INACTIVE);
  assert(s_write_attempts == 0);
  assert(fake_schedule_count == 0);
  assert(fake_haptic_count == 0);
}

static void test_skip_failures_retain_committed_state(void) {
  for (int lifecycle = 0; lifecycle < 2; ++lifecycle) {
    for (int failure = 0; failure < 2; ++failure) {
      fake_reset();
      prv_start_session();
      if (lifecycle) {
        session_coordinator_timed_pause(5 * 60);
        fake_run_timers();
      }
      (void)prv_view();
      fake_run_timers();
      SessionView committed = prv_view();
      int cancels = fake_cancel_count;
      fake_fail_schedule = failure == 0;
      fake_fail_write = failure == 1;
      session_coordinator_skip();
      fake_run_timers();
      SessionView view = prv_view();
      assert(memcmp(&view.record, &committed.record, sizeof(view.record)) == 0);
      assert(view.error == (failure == 0 ? SESSION_ERROR_WAKEUP : SESSION_ERROR_STORAGE));
      assert(fake_haptic_count == 0);
      /* Only a storage failure cancels (the candidate); the committed Wakeup is kept. */
      assert(fake_cancel_count == cancels + (failure == 1 ? 1 : 0));
      assert(fake_last_cancelled_id != committed.record.wakeup_id);
    }
  }

  /* Paused: committed pause is retained on failure. */
  fake_reset();
  prv_start_session();
  session_coordinator_pause();
  fake_run_timers();
  (void)prv_view();
  fake_run_timers();
  SessionView committed = prv_view();
  fake_fail_write = true;
  session_coordinator_skip();
  fake_run_timers();
  SessionView view = prv_view();
  assert(memcmp(&view.record, &committed.record, sizeof(view.record)) == 0);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_haptic_count == 0);
}

static void test_skip_invalid_utc_preserves_committed_state(void) {
  fake_reset();
  prv_start_session();
  (void)prv_view();
  fake_run_timers();
  SessionView committed = prv_view();
  fake_now = 0;
  session_coordinator_skip();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == committed.record.phase);
  assert(view.record.deadline_utc == committed.record.deadline_utc);
  assert(fake_schedule_count == 1);
  assert(fake_haptic_count == 0);

  fake_reset();
  prv_start_session();
  session_coordinator_pause();
  fake_run_timers();
  (void)prv_view();
  fake_run_timers();
  committed = prv_view();
  fake_now = 0;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_PAUSED);
  assert(view.record.phase == committed.record.phase);
  assert(view.record.remaining_seconds == committed.record.remaining_seconds);
  assert(fake_schedule_count == 1);
  assert(fake_haptic_count == 0);

  /* A valid but overflowing clock is rejected by prv_skip's own guard. */
  fake_reset();
  prv_start_session();
  session_coordinator_pause();
  fake_run_timers();
  (void)prv_view();
  fake_run_timers();
  committed = prv_view();
  int writes = s_write_attempts;
  fake_now = (time_t)INT32_MAX - 10;
  session_coordinator_skip();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_PAUSED);
  assert(view.record.phase == committed.record.phase);
  assert(view.record.remaining_seconds == committed.record.remaining_seconds);
  assert(view.record.generation == committed.record.generation);
  assert(s_write_attempts == writes);
  assert(fake_schedule_count == 1);
  assert(fake_haptic_count == 0);
}

static void test_skip_restart_does_not_replay_haptic(void) {
  fake_reset();
  prv_start_session();
  session_coordinator_skip();
  fake_run_timers();
  assert(fake_haptic_count == 1);
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.haptic_generation == stored.generation);
  session_coordinator_deinit();
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.error == SESSION_ERROR_NONE);
  assert(fake_haptic_count == 1);
}

static void test_menu_row_zero_resumes_paused_session(void) {
  fake_reset();
  prv_start_session();
  fake_now += 137;
  session_coordinator_pause();
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_PAUSED);
  fake_now += 60;
  session_coordinator_select_menu_row(0);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == paused.record.phase);
  assert(view.record.deadline_utc == fake_now + paused.record.remaining_seconds);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_haptic_count == 0);
}

static void test_pause_resume_freezes_persists_and_rejects_stale_callback(void) {
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  WakeupId old_id = running.record.wakeup_id;
  int32_t old_cookie = (int32_t)running.record.generation;
  fake_now += 137;
  session_coordinator_pause();
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_PAUSED);
  assert(paused.record.phase == SESSION_PHASE_SIT);
  assert(paused.record.remaining_seconds == 45 * 60 - 137);
  assert(paused.remaining_seconds == 45 * 60 - 137);
  assert(paused.record.deadline_utc == 0);
  assert(paused.record.wakeup_id == (WakeupId)-1);
  assert(paused.record.generation == running.record.generation + 1);
  assert(fake_cancel_count == 1);
  assert(fake_haptic_count == 0);
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_PAUSED);
  assert(stored.remaining_seconds == 45 * 60 - 137);
  assert(stored.deadline_utc == 0 && stored.wakeup_id == (WakeupId)-1);

  int schedules = fake_schedule_count;
  fake_deliver_wakeup(old_id, old_cookie);
  paused = prv_view();
  assert(paused.record.lifecycle == SESSION_PAUSED);
  assert(paused.record.phase == SESSION_PHASE_SIT);
  assert(fake_schedule_count == schedules);
  assert(fake_haptic_count == 0);

  fake_now += 3600;
  session_coordinator_resume();
  fake_run_timers();
  SessionView resumed = prv_view();
  assert(resumed.record.lifecycle == SESSION_RUNNING);
  assert(resumed.record.phase == SESSION_PHASE_SIT);
  assert(resumed.record.deadline_utc == fake_now + 45 * 60 - 137);
  assert(resumed.record.wakeup_id == fake_last_scheduled_id);
  assert(resumed.record.generation == paused.record.generation + 1);
  assert(fake_haptic_count == 0);
  fake_deliver_wakeup(old_id, old_cookie);
  assert(prv_view().record.phase == SESSION_PHASE_SIT);
  assert(fake_haptic_count == 0);
}

static void test_pause_due_boundary_reconciles_first(void) {
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  WakeupId old_id = running.record.wakeup_id;
  int32_t old_cookie = (int32_t)running.record.generation;
  fake_now = (time_t)running.record.deadline_utc;
  session_coordinator_pause();
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_PAUSED);
  assert(paused.record.phase == SESSION_PHASE_STAND);
  assert(paused.record.remaining_seconds == 10 * 60);
  assert(paused.record.deadline_utc == 0);
  assert(paused.record.wakeup_id == (WakeupId)-1);
  assert(paused.record.sit_count == 1);
  assert(fake_haptic_count == 1);
  fake_deliver_wakeup(old_id, old_cookie);
  assert(prv_view().record.phase == SESSION_PHASE_STAND);
  assert(fake_haptic_count == 1);
}

static void test_pause_resume_failures_retain_committed_state(void) {
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  fake_fail_write = true;
  session_coordinator_pause();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.wakeup_id == running.record.wakeup_id);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_cancel_count == 0);
  assert(fake_haptic_count == 0);
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_RUNNING);
  assert(stored.wakeup_id == running.record.wakeup_id);

  fake_fail_write = false;
  session_coordinator_pause();
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_PAUSED);

  fake_fail_schedule = true;
  session_coordinator_resume();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_PAUSED);
  assert(view.record.remaining_seconds == paused.record.remaining_seconds);
  assert(view.error == SESSION_ERROR_WAKEUP);

  fake_fail_schedule = false;
  fake_fail_write = true;
  int cancels = fake_cancel_count;
  session_coordinator_resume();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_PAUSED);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_cancel_count == cancels + 1);
  assert(fake_haptic_count == 0);
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_PAUSED);
  assert(stored.remaining_seconds == paused.record.remaining_seconds);
}

static void test_pause_resume_reject_invalid_or_overflowing_clock(void) {
  /* Pause: the running session keeps its committed record and shows a clock error. */
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  fake_run_timers();
  int writes = s_write_attempts;
  int schedules = fake_schedule_count;
  fake_now = 0;
  session_coordinator_pause();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == running.record.phase);
  assert(view.record.deadline_utc == running.record.deadline_utc);
  assert(s_write_attempts == writes);
  assert(fake_schedule_count == schedules);

  /* Resume of a PAUSED record is not covered by the generic pre-command clock check, so
   * prv_resume's own guards are what keep a bogus deadline from being committed. */
  const time_t bad_clocks[] = { 0, (time_t)INT32_MAX - 10 };
  for (size_t i = 0; i < sizeof(bad_clocks) / sizeof(bad_clocks[0]); ++i) {
    fake_reset();
    prv_start_session();
    session_coordinator_pause();
    fake_run_timers();
    SessionView paused = prv_view();
    assert(paused.record.lifecycle == SESSION_PAUSED);
    writes = s_write_attempts;
    schedules = fake_schedule_count;
    fake_now = bad_clocks[i];
    session_coordinator_resume();
    fake_run_timers();
    view = prv_view();
    assert(view.error == SESSION_ERROR_CLOCK);
    assert(view.record.lifecycle == SESSION_PAUSED);
    assert(view.record.remaining_seconds == paused.record.remaining_seconds);
    assert(view.record.deadline_utc == 0);
    assert(view.record.generation == paused.record.generation);
    assert(s_write_attempts == writes);
    assert(fake_schedule_count == schedules);
  }
}

static void test_pause_does_not_commit_when_due_reconciliation_fails(void) {
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  fake_now = (time_t)running.record.deadline_utc;
  fake_fail_schedule = true;
  session_coordinator_pause();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == running.record.phase);
  assert(view.record.deadline_utc == running.record.deadline_utc);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);

  fake_fail_schedule = false;
  fake_fail_write = true;
  session_coordinator_pause();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == running.record.phase);
  assert(view.record.deadline_utc == running.record.deadline_utc);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_haptic_count == 0);
}

static void test_paused_record_recovers_without_wakeup(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_PAUSED,
    .phase = SESSION_PHASE_MOVE,
    .generation = 12,
    .remaining_seconds = 73,
    .wakeup_id = (WakeupId)-1,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.lifecycle == SESSION_PAUSED);
  assert(view.record.phase == SESSION_PHASE_MOVE);
  assert(view.record.remaining_seconds == 73);
  assert(view.remaining_seconds == 73);
  assert(view.record.deadline_utc == 0);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_schedule_count == 0);
  assert(fake_cancel_all_count == 1);
  assert(fake_haptic_count == 0);
}

static void test_middle_control_resumes_only_while_paused(void) {
  fake_reset();
  prv_start_session();
  assert(session_coordinator_middle_control());
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_TIMED_PAUSED);
  assert(session_coordinator_middle_control());
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_RUNNING);
  session_coordinator_pause();
  fake_run_timers();
  assert(session_coordinator_middle_control());
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_RUNNING);
}

static void test_control_enqueue_failure_is_reported(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  for (int i = 0; i < 8; ++i) {
    session_coordinator_wakeup(view.record.wakeup_id + 100 + i,
                               (int32_t)view.record.generation + 1);
  }
  assert(!session_coordinator_middle_control());
  assert(!session_coordinator_top_control());
  assert(!session_coordinator_bottom_control());
  fake_run_timers();
  assert(prv_view().error == SESSION_ERROR_QUEUE);
  assert(prv_view().record.deadline_utc == view.record.deadline_utc);
}

static void test_extend_adds_and_stacks_silently(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  int64_t original = view.record.deadline_utc;
  WakeupId old_id = view.record.wakeup_id;
  uint32_t generation = view.record.generation;
  fake_now += 100;
  assert(session_coordinator_top_control());
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == original + 300);
  assert(view.record.generation == generation + 1);
  assert(view.record.haptic_generation == 0);
  assert(view.record.remaining_seconds == 0);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_last_scheduled_deadline == original + 300);
  assert(fake_last_scheduled_cookie == (int32_t)view.record.generation);
  assert(fake_last_cancelled_id == old_id);
  assert(view.remaining_seconds == (int32_t)(original + 300 - fake_now));
  assert(view.error == SESSION_ERROR_NONE);
  assert(fake_haptic_count == 0);

  assert(session_coordinator_bottom_control());
  fake_run_timers();
  assert(session_coordinator_bottom_control());
  fake_run_timers();
  assert(session_coordinator_top_control());
  fake_run_timers();
  view = prv_view();
  assert(view.record.deadline_utc == original + 300 + 600 + 600 + 300);
  assert(view.record.generation == generation + 4);
  assert(fake_last_scheduled_deadline == view.record.deadline_utc);
  assert(fake_haptic_count == 0);

  /* The next phase still gets its default duration. */
  fake_now = (time_t)view.record.deadline_utc;
  fake_deliver_wakeup(view.record.wakeup_id, (int32_t)view.record.generation);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60);
  assert(view.record.sit_count == 1);
  assert(fake_haptic_count == 1);
}

static void test_extend_ignores_stale_callback_and_persists_across_restart(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId old_id = view.record.wakeup_id;
  int32_t old_cookie = (int32_t)view.record.generation;
  session_coordinator_bottom_control();
  fake_run_timers();
  view = prv_view();
  int64_t extended = view.record.deadline_utc;
  fake_now = extended - 1;
  int schedules = fake_schedule_count;
  fake_deliver_wakeup(old_id, old_cookie);
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.deadline_utc == extended);
  assert(fake_schedule_count == schedules);
  assert(fake_haptic_count == 0);

  fake_now = (time_t)extended - 2000;
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.deadline_utc == extended);
  session_coordinator_deinit();
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.deadline_utc == extended);
  assert(fake_last_scheduled_deadline == extended);
  assert(view.error == SESSION_ERROR_NONE);
  assert(fake_haptic_count == 0);
}

static void test_extend_reconciles_due_deadline_first(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  fake_now = (time_t)view.record.deadline_utc;
  assert(session_coordinator_top_control());
  fake_run_timers();
  view = prv_view();
  /* Due Sit completes (counted, haptic) into Stand, then Extend applies to Stand. */
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.deadline_utc == fake_now + 10 * 60 + 300);
  assert(view.record.sit_count == 1);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_haptic_count == 1);

  /* If the due reconcile fails, Extend is dropped. */
  fake_reset();
  prv_start_session();
  view = prv_view();
  fake_now = (time_t)view.record.deadline_utc;
  fake_fail_schedule = true;
  assert(session_coordinator_bottom_control());
  fake_run_timers();
  view = prv_view();
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.sit_count == 0);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);
}

static void test_extend_is_not_available_outside_running(void) {
  fake_reset();
  session_coordinator_init();
  fake_run_timers();
  assert(!session_coordinator_top_control());
  assert(!session_coordinator_bottom_control());
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_INACTIVE);
  assert(s_write_attempts == 0);

  fake_reset();
  prv_start_session();
  session_coordinator_pause();
  fake_run_timers();
  SessionView paused = prv_view();
  int writes = s_write_attempts;
  assert(!session_coordinator_top_control());
  assert(!session_coordinator_bottom_control());
  fake_run_timers();
  SessionView view = prv_view();
  assert(memcmp(&view.record, &paused.record, sizeof(view.record)) == 0);
  assert(s_write_attempts == writes);
  assert(fake_haptic_count == 0);

  /* Timed pause keeps selecting the break length. */
  fake_reset();
  prv_start_session();
  session_coordinator_middle_control();
  fake_run_timers();
  assert(session_coordinator_top_control());
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.record.pause_duration_seconds == 5 * 60);
}

static void test_extend_press_during_timed_pause_does_not_extend_resumed_phase(void) {
  for (int bottom = 0; bottom < 2; ++bottom) {
    fake_reset();
    prv_start_session();
    session_coordinator_timed_pause(5 * 60);
    fake_run_timers();
    SessionView paused = prv_view();
    fake_now = (time_t)paused.record.pause_deadline_utc;
    assert(bottom ? session_coordinator_bottom_control() : session_coordinator_top_control());
    fake_run_timers();
    SessionView view = prv_view();
    assert(view.record.lifecycle == SESSION_RUNNING);
    assert(view.record.phase == SESSION_PHASE_SIT);
    assert(view.record.deadline_utc == fake_now + paused.record.remaining_seconds);
    assert(fake_haptic_count == 1);
  }
}

static void test_extend_failures_retain_committed_state(void) {
  for (int failure = 0; failure < 2; ++failure) {
    fake_reset();
    prv_start_session();
    (void)prv_view();
    fake_run_timers();
    SessionView committed = prv_view();
    int cancels = fake_cancel_count;
    fake_fail_schedule = failure == 0;
    fake_fail_write = failure == 1;
    assert(session_coordinator_top_control());
    fake_run_timers();
    SessionView view = prv_view();
    assert(memcmp(&view.record, &committed.record, sizeof(view.record)) == 0);
    assert(view.error == (failure == 0 ? SESSION_ERROR_WAKEUP : SESSION_ERROR_STORAGE));
    assert(fake_haptic_count == 0);
    /* Only a storage failure cancels (the candidate); the committed Wakeup is kept. */
    assert(fake_cancel_count == cancels + (failure == 1 ? 1 : 0));
    assert(fake_last_cancelled_id != committed.record.wakeup_id);
    SessionRecord stored;
    bool found;
    assert(session_store_load(&stored, &found) && found);
    assert(stored.deadline_utc == committed.record.deadline_utc);
  }
}

static void test_extend_invalid_or_overflowing_utc_preserves_committed_state(void) {
  fake_reset();
  prv_start_session();
  (void)prv_view();
  fake_run_timers();
  SessionView committed = prv_view();
  fake_now = 0;
  assert(session_coordinator_top_control());
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.deadline_utc == committed.record.deadline_utc);
  assert(fake_schedule_count == 1);
  assert(fake_haptic_count == 0);

  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 3,
    .deadline_utc = (int64_t)INT32_MAX - 200,
    .wakeup_id = 5,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  committed = prv_view();
  assert(committed.record.deadline_utc == (int64_t)INT32_MAX - 200);
  int writes = s_write_attempts;
  int schedules = fake_schedule_count;
  assert(session_coordinator_top_control());
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.deadline_utc == committed.record.deadline_utc);
  assert(view.record.generation == committed.record.generation);
  assert(view.record.wakeup_id == committed.record.wakeup_id);
  assert(s_write_attempts == writes);
  assert(fake_schedule_count == schedules);
  assert(fake_haptic_count == 0);
}

static void test_clock_check_retries_failed_due_transition(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  SessionRecord committed = view.record;
  fake_now = (time_t)view.record.deadline_utc;
  fake_fail_schedule = true;
  int schedules = fake_schedule_count;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_schedule_count == schedules + 1);
  assert(fake_haptic_count == 0);

  /* Still failing: each clock check retries and keeps the loss visible. */
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_schedule_count == schedules + 2);

  fake_fail_schedule = false;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(view.record.generation > committed.generation);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_haptic_count == 1);
}

static void test_menu_stop_and_start_reconcile_due_deadline_first(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  fake_run_timers(); /* drain the clock check so the command itself must reconcile */
  fake_now = (time_t)view.record.deadline_utc;
  int schedules = fake_schedule_count;
  session_coordinator_select_menu_row(2);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.record.sit_count == 1);
  assert(fake_haptic_count == 1);
  assert(fake_schedule_count == schedules + 1);
  assert(fake_last_cancelled_id == fake_last_scheduled_id);

  fake_reset();
  prv_start_session();
  view = prv_view();
  fake_run_timers();
  fake_now = (time_t)view.record.deadline_utc;
  schedules = fake_schedule_count;
  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == SESSION_PHASE_STAND);
  assert(view.record.sit_count == 1);
  assert(fake_schedule_count == schedules + 1);
  assert(fake_haptic_count == 1);
}

static void test_wakeup_launch_event_is_queued_behind_recovery_and_stale(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 4,
    .deadline_utc = fake_now + 45 * 60,
    .wakeup_id = 55,
  };
  prv_save(&record);
  s_launch_reason = APP_LAUNCH_WAKEUP;
  s_launch_event_available = true;
  s_launch_wakeup_id = record.wakeup_id;
  s_launch_cookie = (int32_t)record.generation;
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  /* Recovery cleared the stored Wakeup and issued a new id, so the launch event is stale. */
  assert(fake_cancel_all_count == 1);
  assert(fake_schedule_count == 1);
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.phase == SESSION_PHASE_SIT);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(view.record.wakeup_id != record.wakeup_id);
  assert(fake_haptic_count == 0);
  session_coordinator_deinit();
}

static void test_recovery_failure_exits_report_error_and_retry(void) {
  SessionRecord record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 7,
    .wakeup_id = 88,
  };

  /* Invalid clock at launch. */
  fake_reset();
  record.deadline_utc = fake_now + 45 * 60;
  prv_save(&record);
  fake_now = 0;
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_CLOCK);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_schedule_count == 0);
  fake_now = 1800000000;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  session_coordinator_deinit();

  /* Overdue recovery: Wakeup scheduling fails, then succeeds on retry. */
  fake_reset();
  record.deadline_utc = fake_now - 30 * 60;
  prv_save(&record);
  fake_fail_schedule = true;
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.wakeup_id == (WakeupId)-1);
  fake_fail_schedule = false;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(view.record.deadline_utc > fake_now);
  session_coordinator_deinit();

  /* Overdue recovery: persistence fails, then succeeds on retry. */
  fake_reset();
  record.deadline_utc = fake_now - 30 * 60;
  prv_save(&record);
  fake_fail_write = true;
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_last_cancelled_id == fake_last_scheduled_id);
  fake_fail_write = false;
  (void)prv_view();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  session_coordinator_deinit();
}

static void test_stop_works_with_invalid_clock(void) {
  for (int timed = 0; timed < 2; ++timed) {
    fake_reset();
    prv_start_session();
    if (timed) {
      session_coordinator_timed_pause(5 * 60);
      fake_run_timers();
      assert(prv_view().record.lifecycle == SESSION_TIMED_PAUSED);
    }
    WakeupId committed_id = prv_view().record.wakeup_id;
    int32_t committed_generation = (int32_t)prv_view().record.generation;
    int cancels = fake_cancel_count;
    fake_now = 0;
    session_coordinator_stop();
    fake_run_timers();
    SessionView view = prv_view();
    assert(view.record.lifecycle == SESSION_INACTIVE);
    assert(view.record.phase == SESSION_PHASE_NONE);
    assert(view.record.deadline_utc == 0 && view.record.remaining_seconds == 0);
    assert(view.record.pause_deadline_utc == 0 && view.record.pause_duration_seconds == 0);
    assert(view.record.wakeup_id == (WakeupId)-1);
    assert(fake_cancel_count == cancels + 1);
    assert(fake_last_cancelled_id == committed_id);
    SessionRecord stored;
    bool found;
    assert(session_store_load(&stored, &found) && found);
    assert(stored.lifecycle == SESSION_INACTIVE);
    fake_deliver_wakeup(committed_id, committed_generation);
    assert(prv_view().record.lifecycle == SESSION_INACTIVE);
  }

  /* Menu row 2 takes the same path. */
  fake_reset();
  prv_start_session();
  fake_now = 0;
  session_coordinator_select_menu_row(2);
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_INACTIVE);
}

static void test_stop_from_timed_pause_cancels_pause_wakeup(void) {
  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(10 * 60);
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_TIMED_PAUSED);
  int cancels = fake_cancel_count;
  session_coordinator_select_menu_row(2);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.record.remaining_seconds == 0);
  assert(view.record.pause_deadline_utc == 0 && view.record.pause_duration_seconds == 0);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_cancel_count == cancels + 1);
  assert(fake_last_cancelled_id == paused.record.wakeup_id);
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_INACTIVE);
  assert(stored.pause_deadline_utc == 0 && stored.remaining_seconds == 0);

  int schedules = fake_schedule_count;
  fake_now = (time_t)paused.record.pause_deadline_utc;
  fake_deliver_wakeup(paused.record.wakeup_id, (int32_t)paused.record.generation);
  assert(prv_view().record.lifecycle == SESSION_INACTIVE);
  assert(fake_schedule_count == schedules);
  assert(fake_haptic_count == 0);

  /* EVENT_STOP takes the same lifecycle path. */
  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  session_coordinator_stop();
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_INACTIVE);
}

static void test_menu_resume_row_ends_timed_pause_early(void) {
  fake_reset();
  prv_start_session();
  fake_now += 100;
  session_coordinator_timed_pause(30 * 60);
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_TIMED_PAUSED);
  fake_now += 60;
  int cancels = fake_cancel_count;
  session_coordinator_select_menu_row(0);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.phase == paused.record.phase);
  assert(view.record.deadline_utc == fake_now + paused.record.remaining_seconds);
  assert(view.record.remaining_seconds == 0);
  assert(view.record.pause_deadline_utc == 0 && view.record.pause_duration_seconds == 0);
  assert(view.record.wakeup_id == fake_last_scheduled_id);
  assert(view.record.generation == paused.record.generation + 1);
  assert(fake_cancel_count == cancels + 1);
  assert(fake_last_cancelled_id == paused.record.wakeup_id);
  assert(fake_haptic_count == 0);
}

typedef enum {
  DAY_TIMED_ENTRY, DAY_EARLY_RESUME, DAY_PAUSE_END_CALLBACK, DAY_PAUSE_END_RECOVERY,
  DAY_MANUAL_PAUSE, DAY_MANUAL_RESUME, DAY_SKIP_RUNNING, DAY_SKIP_PAUSED,
  DAY_EXTEND_TOP, DAY_EXTEND_BOTTOM, DAY_CASES
} DayCase;

/* Starts 100 s before local midnight with a non-zero tally, runs the case's setup, crosses
 * midnight without a render tick (a tick would enqueue the async rollover and hide a missing
 * reset), applies the transition and expects the tally to belong to the new local day. */
static void test_pause_transitions_reset_tally_day_across_midnight(void) {
  for (int c = 0; c < DAY_CASES; ++c) {
    fake_reset();
    struct tm local = *localtime(&fake_now);
    local.tm_hour = 0;
    local.tm_min = 0;
    local.tm_sec = 0;
    time_t midnight = mktime(&local) + 24 * 3600;
    fake_now = midnight - 100;
    session_coordinator_init();
    fake_run_timers();
    session_coordinator_start();
    fake_run_timers();
    SessionView view = prv_view();
    fake_run_timers(); /* day rollover for the first day */
    SessionRecord seeded = prv_view().record;
    int32_t first_day = seeded.tally_day;
    assert(first_day != 0);
    seeded.sit_count = 3;
    seeded.stand_count = 2;
    seeded.move_count = 1;
    prv_save(&seeded);
    session_coordinator_recover();
    fake_run_timers();
    (void)view;

    if (c == DAY_EARLY_RESUME || c == DAY_PAUSE_END_CALLBACK || c == DAY_PAUSE_END_RECOVERY) {
      session_coordinator_timed_pause(c == DAY_EARLY_RESUME ? 30 * 60 : 5 * 60);
      fake_run_timers();
    } else if (c == DAY_MANUAL_RESUME || c == DAY_SKIP_PAUSED) {
      session_coordinator_pause();
      fake_run_timers();
    }
    SessionRecord before = prv_view().record;
    fake_run_timers();
    assert(before.tally_day == first_day && before.sit_count == 3);

    fake_now = midnight + 100; /* crosses local midnight; no render tick follows */
    switch (c) {
      case DAY_TIMED_ENTRY:
        session_coordinator_timed_pause(5 * 60);
        fake_run_timers();
        break;
      case DAY_EARLY_RESUME:
        session_coordinator_middle_control();
        fake_run_timers();
        break;
      case DAY_PAUSE_END_CALLBACK:
        fake_now = (time_t)before.pause_deadline_utc;
        if (fake_now < midnight + 100) {
          fake_now = midnight + 100;
        }
        fake_deliver_wakeup(before.wakeup_id, (int32_t)before.generation);
        break;
      case DAY_PAUSE_END_RECOVERY:
        fake_now = (time_t)before.pause_deadline_utc;
        if (fake_now < midnight + 100) {
          fake_now = midnight + 100;
        }
        session_coordinator_deinit();
        session_coordinator_init();
        fake_run_timers();
        break;
      case DAY_MANUAL_PAUSE:
        session_coordinator_pause();
        fake_run_timers();
        break;
      case DAY_SKIP_RUNNING:
      case DAY_SKIP_PAUSED:
        session_coordinator_skip();
        fake_run_timers();
        break;
      case DAY_EXTEND_TOP:
        assert(session_coordinator_top_control());
        fake_run_timers();
        break;
      case DAY_EXTEND_BOTTOM:
        assert(session_coordinator_bottom_control());
        fake_run_timers();
        break;
      default:
        session_coordinator_resume();
        fake_run_timers();
        break;
    }
    SessionRecord stored;
    bool found;
    assert(session_store_load(&stored, &found) && found);
    assert(stored.tally_day != first_day);
    assert(stored.sit_count == 0 && stored.stand_count == 0 && stored.move_count == 0);
    if (c == DAY_EXTEND_TOP || c == DAY_EXTEND_BOTTOM) {
      assert(stored.deadline_utc ==
             before.deadline_utc + (c == DAY_EXTEND_TOP ? 300 : 600));
    }
    session_coordinator_deinit();
  }
}

static void test_overdue_pause_end_recovery_does_not_cancel_invalid_id(void) {
  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  SessionRecord paused = prv_view().record;
  session_coordinator_deinit();
  fake_now = (time_t)paused.pause_deadline_utc + 60;
  fake_last_cancelled_id = (WakeupId)-2;
  int cancels = fake_cancel_count;
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(fake_cancel_all_count == 2); /* start-up of the test session, then this recovery */
  /* cancel_all already removed the stored Wakeup, so no per-id cancel (and never id -1) follows. */
  assert(fake_cancel_count == cancels);
  assert(fake_last_cancelled_id == (WakeupId)-2);
  session_coordinator_deinit();
}

static void test_stop_paused_session_clears_saved_remainder(void) {
  fake_reset();
  prv_start_session();
  fake_now += 10;
  session_coordinator_pause();
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_PAUSED);
  int cancels = fake_cancel_count;
  session_coordinator_select_menu_row(2);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.record.remaining_seconds == 0);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_cancel_count == cancels);
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_INACTIVE);
  assert(stored.remaining_seconds == 0);
}

static void test_timed_pause_entry_change_resume_and_stale_callback(void) {
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  WakeupId phase_id = running.record.wakeup_id;
  int32_t phase_cookie = (int32_t)running.record.generation;
  fake_now += 137;
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(paused.record.phase == SESSION_PHASE_SIT);
  assert(paused.record.remaining_seconds == 45 * 60 - 137);
  assert(paused.record.pause_duration_seconds == 5 * 60);
  assert(paused.record.pause_deadline_utc == fake_now + 5 * 60);
  assert(paused.remaining_seconds == 5 * 60);
  assert(paused.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_haptic_count == 0);
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_TIMED_PAUSED);
  assert(stored.remaining_seconds == 45 * 60 - 137);
  assert(stored.pause_deadline_utc == fake_now + 5 * 60);

  WakeupId old_pause_id = paused.record.wakeup_id;
  int32_t old_pause_cookie = (int32_t)paused.record.generation;
  session_coordinator_change_timed_pause(true);
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(paused.record.pause_duration_seconds == 30 * 60);
  assert(paused.record.pause_deadline_utc == fake_now + 30 * 60);
  assert(paused.record.remaining_seconds == 45 * 60 - 137);
  assert(paused.record.wakeup_id != old_pause_id);
  fake_deliver_wakeup(old_pause_id, old_pause_cookie);
  assert(prv_view().record.lifecycle == SESSION_TIMED_PAUSED);
  assert(fake_haptic_count == 0);

  WakeupId current_pause_id = paused.record.wakeup_id;
  int32_t current_pause_cookie = (int32_t)paused.record.generation;
  fake_now += 60;
  assert(session_coordinator_middle_control());
  fake_run_timers();
  SessionView resumed = prv_view();
  assert(resumed.record.lifecycle == SESSION_RUNNING);
  assert(resumed.record.phase == SESSION_PHASE_SIT);
  assert(resumed.record.deadline_utc == fake_now + 45 * 60 - 137);
  assert(resumed.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_haptic_count == 0);
  assert(fake_cancel_count == 3);
  fake_deliver_wakeup(phase_id, phase_cookie);
  fake_deliver_wakeup(current_pause_id, current_pause_cookie);
  assert(prv_view().record.phase == SESSION_PHASE_SIT);
  assert(fake_haptic_count == 0);
}

static void test_timed_pause_due_boundary_and_overdue_recovery(void) {
  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(10 * 60);
  fake_run_timers();
  SessionView paused = prv_view();
  WakeupId pause_id = paused.record.wakeup_id;
  int32_t pause_cookie = (int32_t)paused.record.generation;
  int32_t saved = paused.record.remaining_seconds;
  session_coordinator_change_timed_pause(false);
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.pause_duration_seconds == 5 * 60);
  session_coordinator_change_timed_pause(true);
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.pause_duration_seconds == 30 * 60);
  saved = paused.record.remaining_seconds;
  fake_now = (time_t)paused.record.pause_deadline_utc;
  assert(session_coordinator_middle_control());
  fake_run_timers();
  SessionView resumed = prv_view();
  assert(resumed.record.lifecycle == SESSION_RUNNING);
  assert(resumed.record.phase == SESSION_PHASE_SIT);
  assert(resumed.record.deadline_utc == fake_now + saved);
  assert(fake_haptic_count == 1);
  fake_deliver_wakeup(pause_id, pause_cookie);
  assert(fake_haptic_count == 1);

  fake_reset();
  prv_start_session();
  session_coordinator_middle_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(paused.record.pause_duration_seconds == 30 * 60);
  session_coordinator_top_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.pause_duration_seconds == 5 * 60);
  session_coordinator_bottom_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.pause_duration_seconds == 30 * 60);
  session_coordinator_top_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.pause_duration_seconds == 5 * 60);
  session_coordinator_bottom_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.pause_duration_seconds == 30 * 60);
  assert(paused.record.remaining_seconds == 45 * 60);
  assert(fake_haptic_count == 0);

  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(10 * 60);
  fake_run_timers();
  paused = prv_view();
  pause_id = paused.record.wakeup_id;
  pause_cookie = (int32_t)paused.record.generation;
  int32_t exact_remainder = paused.record.remaining_seconds;
  fake_now = (time_t)paused.record.pause_deadline_utc;
  fake_deliver_wakeup(pause_id, pause_cookie);
  resumed = prv_view();
  assert(resumed.record.lifecycle == SESSION_RUNNING);
  assert(resumed.record.deadline_utc == fake_now + exact_remainder);
  assert(resumed.record.phase == SESSION_PHASE_SIT);
  assert(fake_haptic_count == 1);

  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_TIMED_PAUSED,
    .phase = SESSION_PHASE_MOVE,
    .generation = 8,
    .remaining_seconds = 73,
    .pause_deadline_utc = fake_now - 1,
    .pause_duration_seconds = 5 * 60,
    .wakeup_id = 55,
  };
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  resumed = prv_view();
  assert(resumed.record.lifecycle == SESSION_RUNNING);
  assert(resumed.record.phase == SESSION_PHASE_MOVE);
  assert(resumed.record.deadline_utc == fake_now + 73);
  assert(resumed.record.generation == 9);
  assert(resumed.record.move_count == 0 && resumed.record.sit_count == 0 &&
         resumed.record.stand_count == 0);
  assert(fake_haptic_count == 1);
}

static void test_pause_end_recovery_failures_and_restart_haptic_marker(void) {
  fake_reset();
  SessionRecord record = {
    .lifecycle = SESSION_TIMED_PAUSED,
    .phase = SESSION_PHASE_STAND,
    .generation = 9,
    .remaining_seconds = 87,
    .pause_deadline_utc = fake_now - 1,
    .pause_duration_seconds = 10 * 60,
    .wakeup_id = 42,
  };
  prv_save(&record);
  fake_fail_schedule = true;
  session_coordinator_init();
  fake_run_timers();
  SessionRecord stored;
  bool found;
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_TIMED_PAUSED);
  assert(stored.remaining_seconds == 87);
  assert(stored.pause_deadline_utc == record.pause_deadline_utc);
  assert(stored.wakeup_id == record.wakeup_id);
  assert(prv_view().record.lifecycle == SESSION_TIMED_PAUSED);
  assert(fake_haptic_count == 0);

  fake_reset();
  prv_save(&record);
  fake_fail_write = true;
  session_coordinator_init();
  fake_run_timers();
  assert(session_store_load(&stored, &found) && found);
  assert(stored.lifecycle == SESSION_TIMED_PAUSED);
  assert(stored.remaining_seconds == 87);
  assert(stored.pause_deadline_utc == record.pause_deadline_utc);
  assert(stored.wakeup_id == record.wakeup_id);
  assert(prv_view().record.lifecycle == SESSION_TIMED_PAUSED);
  assert(fake_haptic_count == 0);

  fake_reset();
  record.pause_deadline_utc = fake_now - 1;
  prv_save(&record);
  session_coordinator_init();
  fake_run_timers();
  SessionView resumed = prv_view();
  assert(resumed.record.lifecycle == SESSION_RUNNING);
  assert(resumed.record.phase == SESSION_PHASE_STAND);
  assert(resumed.record.haptic_generation == resumed.record.generation);
  assert(fake_haptic_count == 1);
  session_coordinator_deinit();
  session_coordinator_init();
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_RUNNING);
  assert(fake_haptic_count == 1);
}

static void test_timed_pause_failure_retains_committed_state(void) {
  fake_reset();
  prv_start_session();
  SessionView running = prv_view();
  fake_fail_schedule = true;
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.wakeup_id == running.record.wakeup_id);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);

  fake_fail_schedule = false;
  fake_fail_write = true;
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_RUNNING);
  assert(view.record.wakeup_id == running.record.wakeup_id);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_cancel_count == 1);

  fake_fail_write = false;
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  SessionView paused = prv_view();
  assert(paused.record.lifecycle == SESSION_TIMED_PAUSED);
  fake_fail_schedule = true;
  session_coordinator_change_timed_pause(true);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.record.pause_duration_seconds == 5 * 60);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);

  fake_fail_schedule = false;
  fake_fail_write = true;
  session_coordinator_change_timed_pause(false);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.record.pause_duration_seconds == 5 * 60);
  assert(view.error == SESSION_ERROR_STORAGE);
  fake_fail_write = false;
  session_coordinator_change_timed_pause(false);
  fake_run_timers();
  paused = prv_view();
  fake_fail_schedule = true;
  session_coordinator_middle_control();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.record.pause_deadline_utc == paused.record.pause_deadline_utc);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);

  fake_fail_schedule = false;
  session_coordinator_middle_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.lifecycle == SESSION_RUNNING);
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  paused = prv_view();
  WakeupId early_resume_id = paused.record.wakeup_id;
  fake_fail_write = true;
  session_coordinator_middle_control();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.record.wakeup_id == early_resume_id);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_cancel_count >= 2);
  fake_fail_write = false;
  session_coordinator_middle_control();
  fake_run_timers();
  paused = prv_view();
  assert(paused.record.lifecycle == SESSION_RUNNING);
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  paused = prv_view();
  fake_fail_schedule = true;
  fake_now = (time_t)paused.record.pause_deadline_utc;
  fake_deliver_wakeup(paused.record.wakeup_id, (int32_t)paused.record.generation);
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.error == SESSION_ERROR_WAKEUP);
  assert(fake_haptic_count == 0);

  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  paused = prv_view();
  WakeupId due_id = paused.record.wakeup_id;
  int32_t due_cookie = (int32_t)paused.record.generation;
  int32_t remainder = paused.record.remaining_seconds;
  fake_now = (time_t)paused.record.pause_deadline_utc;
  fake_fail_write = true;
  fake_deliver_wakeup(due_id, due_cookie);
  view = prv_view();
  assert(view.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(view.record.remaining_seconds == remainder);
  assert(view.error == SESSION_ERROR_STORAGE);
  assert(fake_haptic_count == 0);
}

static void test_timed_pause_invalid_utc_preserves_committed_state(void) {
  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  SessionView committed = prv_view();
  fake_now = 0;
  (void)prv_view();
  fake_run_timers();
  SessionView invalid = prv_view();
  assert(invalid.error == SESSION_ERROR_CLOCK);
  assert(invalid.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(invalid.record.phase == committed.record.phase);
  assert(invalid.record.remaining_seconds == committed.record.remaining_seconds);
  assert(invalid.record.pause_deadline_utc == committed.record.pause_deadline_utc);
  assert(invalid.record.wakeup_id == (WakeupId)-1);
  assert(fake_haptic_count == 0);

  fake_now = (time_t)committed.record.pause_deadline_utc;
  (void)prv_view();
  fake_run_timers();
  SessionView recovered = prv_view();
  assert(recovered.error == SESSION_ERROR_NONE);
  assert(recovered.record.lifecycle == SESSION_RUNNING);
  assert(recovered.record.phase == committed.record.phase);
  assert(recovered.record.deadline_utc == fake_now + committed.record.remaining_seconds);
  assert(fake_haptic_count == 1);

  fake_reset();
  prv_start_session();
  session_coordinator_timed_pause(5 * 60);
  fake_run_timers();
  committed = prv_view();
  fake_now = 0;
  session_coordinator_middle_control();
  fake_run_timers();
  invalid = prv_view();
  assert(invalid.error == SESSION_ERROR_CLOCK);
  assert(invalid.record.lifecycle == SESSION_TIMED_PAUSED);
  assert(invalid.record.pause_deadline_utc == committed.record.pause_deadline_utc);
  assert(invalid.record.wakeup_id == (WakeupId)-1);
}

static void prv_expect_invalid(SessionRecord record);
static void prv_expect_load_rejected(const uint8_t *bytes, size_t size);

static void test_timed_pause_store_validation(void) {
  fake_reset();
  SessionRecord valid = {
    .lifecycle = SESSION_TIMED_PAUSED,
    .phase = SESSION_PHASE_STAND,
    .generation = 2,
    .remaining_seconds = 12,
    .pause_deadline_utc = fake_now + 5 * 60,
    .pause_duration_seconds = 5 * 60,
    .wakeup_id = 3,
  };
  assert(session_store_validate(&valid));
  SessionRecord invalid = valid;
  invalid.pause_duration_seconds = 6 * 60;
  prv_expect_invalid(invalid);
  invalid = valid; invalid.pause_deadline_utc = 0; prv_expect_invalid(invalid);
  invalid = valid; invalid.pause_deadline_utc = (int64_t)INT32_MAX + 1; prv_expect_invalid(invalid);
  invalid = valid; invalid.remaining_seconds = 0; prv_expect_invalid(invalid);
  invalid = valid; invalid.deadline_utc = 1; prv_expect_invalid(invalid);
  invalid = valid; invalid.wakeup_id = (WakeupId)-1; prv_expect_invalid(invalid);
  invalid = valid; invalid.pause_duration_seconds = 0; prv_expect_invalid(invalid);

  prv_save(&valid);
  uint8_t bytes[45];
  memcpy(bytes, s_persisted, sizeof(bytes));
  uint8_t valid_bytes[45];
  memcpy(valid_bytes, bytes, sizeof(valid_bytes));
  bytes[41] = 0x20;
  bytes[42] = 0x03;
  bytes[43] = 0;
  bytes[44] = 0;
  prv_expect_load_rejected(bytes, sizeof(bytes));
  memcpy(bytes, valid_bytes, sizeof(bytes));
  bytes[40] = 1;
  prv_expect_load_rejected(bytes, sizeof(bytes));
}

static SessionRecord prv_full_record(void) {
  return (SessionRecord){
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_STAND,
    .generation = 5,
    .haptic_generation = 3,
    .deadline_utc = 0x7ffffff0,
    .remaining_seconds = 0,
    .wakeup_id = 0x12345,
    .tally_day = 2026 * 1000 + 305,
    .sit_count = 111,
    .stand_count = 222,
    .move_count = 333,
  };
}

static void test_store_round_trip(void) {
  fake_reset();
  SessionRecord record = prv_full_record();
  prv_save(&record);
  assert(s_persisted_size == 45);
  SessionRecord loaded;
  bool found;
  bool ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == record.lifecycle && loaded.phase == record.phase);
  assert(loaded.generation == 5 && loaded.haptic_generation == 3);
  assert(loaded.deadline_utc == 0x7ffffff0 && loaded.wakeup_id == 0x12345);
  assert(loaded.remaining_seconds == 0);
  assert(loaded.tally_day == 2026305);
  assert(loaded.sit_count == 111 && loaded.stand_count == 222 && loaded.move_count == 333);

  SessionRecord inactive = { .generation = 9, .haptic_generation = 9, .wakeup_id = (WakeupId)-1,
                             .tally_day = 1, .sit_count = 999, .stand_count = 998, .move_count = 997 };
  prv_save(&inactive);
  ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == SESSION_INACTIVE && loaded.phase == SESSION_PHASE_NONE);
  assert(loaded.wakeup_id == (WakeupId)-1 && loaded.deadline_utc == 0);
  assert(loaded.sit_count == 999 && loaded.stand_count == 998 && loaded.move_count == 997);

  SessionRecord paused = {
    .lifecycle = SESSION_PAUSED,
    .phase = SESSION_PHASE_STAND,
    .generation = 10,
    .remaining_seconds = 321,
    .wakeup_id = (WakeupId)-1,
  };
  prv_save(&paused);
  assert(s_persisted_size == 45);
  ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == SESSION_PAUSED);
  assert(loaded.phase == SESSION_PHASE_STAND);
  assert(loaded.remaining_seconds == 321);
  assert(loaded.deadline_utc == 0 && loaded.wakeup_id == (WakeupId)-1);

  SessionRecord timed = {
    .lifecycle = SESSION_TIMED_PAUSED,
    .phase = SESSION_PHASE_MOVE,
    .generation = 11,
    .remaining_seconds = 321,
    .pause_deadline_utc = 1800000300,
    .pause_duration_seconds = 5 * 60,
    .wakeup_id = 22,
  };
  prv_save(&timed);
  assert(s_persisted_size == 45);
  ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == SESSION_TIMED_PAUSED);
  assert(loaded.phase == SESSION_PHASE_MOVE && loaded.remaining_seconds == 321);
  assert(loaded.pause_deadline_utc == timed.pause_deadline_utc);
  assert(loaded.pause_duration_seconds == 5 * 60);
  assert(loaded.wakeup_id == timed.wakeup_id);

  SessionRecord legacy = prv_full_record();
  prv_save(&legacy);
  s_persisted[0] = 1;
  s_persisted_size = 33;
  ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == SESSION_RUNNING);
  assert(loaded.deadline_utc == legacy.deadline_utc);
  assert(loaded.pause_deadline_utc == 0 && loaded.pause_duration_seconds == 0);

  SessionRecord legacy_paused = {
    .lifecycle = SESSION_PAUSED,
    .phase = SESSION_PHASE_STAND,
    .generation = 12,
    .remaining_seconds = 321,
    .wakeup_id = (WakeupId)-1,
  };
  prv_save(&legacy_paused);
  s_persisted[0] = 1;
  s_persisted_size = 33;
  ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == SESSION_PAUSED);
  assert(loaded.phase == SESSION_PHASE_STAND);
  assert(loaded.remaining_seconds == 321);
  assert(loaded.pause_deadline_utc == 0 && loaded.pause_duration_seconds == 0);
}

static void prv_expect_invalid(SessionRecord record) {
  assert(!session_store_validate(&record));
  fake_reset();
  assert(!session_store_save(&record));
  assert(s_persisted_size == 0);
}

static void prv_expect_load_rejected(const uint8_t *bytes, size_t size) {
  fake_reset();
  int written = persist_write_data(STORE_KEY, bytes, size);
  prv_check(written == (int)size, "persist_write_data");
  SessionRecord loaded;
  bool found = true;
  assert(!session_store_load(&loaded, &found));
  assert(!found);
}

static void test_store_rejects_invalid_records(void) {
  fake_reset();
  SessionRecord valid = prv_full_record();
  assert(session_store_validate(&valid));

  SessionRecord r = valid; r.generation = 0; prv_expect_invalid(r);
  r = valid; r.generation = (uint32_t)INT32_MAX + 1; prv_expect_invalid(r);
  r = valid; r.haptic_generation = valid.generation + 1; prv_expect_invalid(r);
  r = valid; r.tally_day = -1; prv_expect_invalid(r);
  r = valid; r.sit_count = 1000; prv_expect_invalid(r);
  r = valid; r.stand_count = 1000; prv_expect_invalid(r);
  r = valid; r.move_count = 1000; prv_expect_invalid(r);
  r = valid; r.lifecycle = (SessionLifecycle)3; prv_expect_invalid(r);
  r = valid; r.phase = SESSION_PHASE_NONE; prv_expect_invalid(r);
  r = valid; r.phase = (SessionPhase)4; prv_expect_invalid(r);
  r = valid; r.deadline_utc = 0; prv_expect_invalid(r);
  r = valid; r.deadline_utc = (int64_t)INT32_MAX + 1; prv_expect_invalid(r);
  r = valid; r.wakeup_id = (WakeupId)-1; prv_expect_invalid(r);
  r = valid; r.wakeup_id = -5; prv_expect_invalid(r);
  r = valid; r.lifecycle = SESSION_INACTIVE; r.phase = SESSION_PHASE_NONE; r.deadline_utc = 0;
  r.wakeup_id = 7; prv_expect_invalid(r);
  r = valid; r.lifecycle = SESSION_INACTIVE; r.phase = SESSION_PHASE_SIT; r.deadline_utc = 0;
  r.wakeup_id = (WakeupId)-1; prv_expect_invalid(r);
  r = valid; r.lifecycle = SESSION_INACTIVE; r.phase = SESSION_PHASE_NONE; r.deadline_utc = 5;
  r.wakeup_id = (WakeupId)-1; prv_expect_invalid(r);
  r = valid; r.lifecycle = SESSION_PAUSED; r.deadline_utc = 0; r.remaining_seconds = 0;
  r.wakeup_id = (WakeupId)-1; prv_expect_invalid(r);
  r = valid; r.lifecycle = SESSION_PAUSED; r.deadline_utc = 1; r.remaining_seconds = 5;
  r.wakeup_id = (WakeupId)-1; prv_expect_invalid(r);
  r = valid; r.lifecycle = SESSION_PAUSED; r.deadline_utc = 0; r.remaining_seconds = 5;
  r.wakeup_id = 7; prv_expect_invalid(r);

  fake_reset();
  prv_save(&valid);
  uint8_t bytes[45];
  memcpy(bytes, s_persisted, sizeof(bytes));
  uint8_t mutated[45];
  memcpy(mutated, bytes, sizeof(bytes)); mutated[0] = 0; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[0] = 3; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[1] = 2; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[2] = 9; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[15] = 1; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[18] = 0x80; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[1] = SESSION_PAUSED;
  mutated[11] = 0; mutated[12] = 0; mutated[13] = 0; mutated[14] = 0;
  mutated[15] = 0; mutated[16] = 0; mutated[17] = 0; mutated[18] = 0;
  mutated[19] = 0xff; mutated[20] = 0xff; mutated[21] = 0xff; mutated[22] = 0xff;
  prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[1] = SESSION_PAUSED;
  mutated[11] = 5; mutated[12] = 0; mutated[13] = 0; mutated[14] = 0;
  mutated[15] = 1;
  mutated[19] = 0xff; mutated[20] = 0xff; mutated[21] = 0xff; mutated[22] = 0xff;
  prv_expect_load_rejected(mutated, sizeof(mutated));
  prv_expect_load_rejected(bytes, sizeof(bytes) - 1);

  /* Plain PAUSED record: the encoding is otherwise valid, so only the remainder decides. */
  fake_reset();
  SessionRecord paused = {
    .lifecycle = SESSION_PAUSED,
    .phase = SESSION_PHASE_SIT,
    .generation = 2,
    .remaining_seconds = 5,
    .wakeup_id = (WakeupId)-1,
  };
  prv_save(&paused);
  uint8_t paused_bytes[45];
  size_t paused_size = (size_t)s_persisted_size;
  assert(paused_size <= sizeof(paused_bytes));
  memcpy(paused_bytes, s_persisted, paused_size);
  SessionRecord loaded;
  bool found;
  assert(session_store_load(&loaded, &found) && found && loaded.remaining_seconds == 5);
  paused_bytes[11] = 0xff; paused_bytes[12] = 0xff; paused_bytes[13] = 0xff;
  paused_bytes[14] = 0x7f; /* INT32_MAX: still accepted */
  memcpy(s_persisted, paused_bytes, paused_size);
  assert(session_store_load(&loaded, &found) && found && loaded.remaining_seconds == INT32_MAX);
  paused_bytes[14] = 0x80; /* INT32_MAX + 1: rejected */
  prv_expect_load_rejected(paused_bytes, paused_size);

  /* 33-byte legacy size: only the version byte decides, so unknown versions must be rejected. */
  fake_reset();
  SessionRecord legacy_record = {
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_SIT,
    .generation = 2,
    .deadline_utc = fake_now + 45 * 60,
    .wakeup_id = 7,
  };
  prv_save(&legacy_record);
  uint8_t legacy_bytes[33];
  memcpy(legacy_bytes, s_persisted, sizeof(legacy_bytes));
  legacy_bytes[0] = 1;
  fake_reset();
  assert(persist_write_data(STORE_KEY, legacy_bytes, sizeof(legacy_bytes)) == (int)sizeof(legacy_bytes));
  assert(session_store_load(&loaded, &found) && found && loaded.wakeup_id == 7);
  const uint8_t unknown_versions[] = { 0, 2, 9 };
  for (size_t i = 0; i < sizeof(unknown_versions); ++i) {
    legacy_bytes[0] = unknown_versions[i];
    prv_expect_load_rejected(legacy_bytes, sizeof(legacy_bytes));
  }
}

static void test_invalid_record_is_persisted_as_error_until_replaced(void) {
  fake_reset();
  uint8_t bad[33] = { 9 };
  int written = persist_write_data(STORE_KEY, bad, sizeof(bad));
  prv_check(written == (int)sizeof(bad), "persist_write_data");
  session_coordinator_init();
  fake_run_timers();
  SessionView view = prv_view();
  assert(view.error == SESSION_ERROR_RECORD);
  assert(view.record.lifecycle == SESSION_INACTIVE);
  session_coordinator_start();
  fake_run_timers();
  view = prv_view();
  assert(view.error == SESSION_ERROR_NONE);
  assert(view.record.lifecycle == SESSION_RUNNING);
  SessionRecord stored;
  bool found;
  bool loaded = session_store_load(&stored, &found);
  assert(loaded && found);
}

static void test_log_lines_report_callbacks_and_haptics(void) {
  fake_reset();
  prv_start_session();
  SessionView view = prv_view();
  WakeupId id = view.record.wakeup_id;
  int32_t gen = (int32_t)view.record.generation;
  char want[160];

  /* Stale: wrong generation. Delivery is logged, then the stale rejection with both ids. */
  fake_log[0] = '\0';
  fake_deliver_wakeup(id, gen + 1);
  snprintf(want, sizeof(want), "WAKEUP_DELIVERED id=%ld gen=%ld\n", (long)id, (long)gen + 1);
  assert(strstr(fake_log, want));
  snprintf(want, sizeof(want), "WAKEUP result=stale lc=1 gen=%ld egen=%ld id=%ld eid=%ld\n",
           (long)gen, (long)gen + 1, (long)id, (long)id);
  assert(strstr(fake_log, want));
  assert(!strstr(fake_log, "HAPTIC_REQUEST"));

  /* Early: before the deadline. The reschedule gives a new id, which the accepted line shows. */
  int64_t deadline = view.record.deadline_utc;
  fake_now = (time_t)(deadline - 100);
  fake_log[0] = '\0';
  fake_deliver_wakeup(id, gen);
  snprintf(want, sizeof(want), "WAKEUP result=early lc=1 gen=%ld id=%ld eid=%ld now=%ld dl=%ld\n",
           (long)gen, (long)id, (long)id, (long)fake_now, (long)deadline);
  assert(strstr(fake_log, want));
  assert(!strstr(fake_log, "result=accepted"));
  assert(!strstr(fake_log, "HAPTIC_REQUEST"));

  /* Accepted at the deadline, with the haptic request logged after it. */
  view = prv_view();
  id = view.record.wakeup_id;
  fake_now = (time_t)deadline;
  fake_log[0] = '\0';
  fake_deliver_wakeup(id, gen);
  snprintf(want, sizeof(want), "WAKEUP result=accepted lc=1 gen=%ld id=%ld eid=%ld now=%ld dl=%ld\n",
           (long)gen, (long)id, (long)id, (long)fake_now, (long)deadline);
  const char *accepted = strstr(fake_log, want);
  assert(accepted);
  assert(!strstr(fake_log, "result=early"));
  const char *haptic = strstr(fake_log, "HAPTIC_REQUEST lc=1 phase=2 gen=");
  assert(haptic && haptic > accepted);
  assert(fake_haptic_count == 1);

  /* Skip requests a haptic with no WAKEUP line. */
  fake_log[0] = '\0';
  session_coordinator_skip();
  fake_run_timers();
  assert(strstr(fake_log, "HAPTIC_REQUEST lc=1 phase=3 gen="));
  assert(!strstr(fake_log, "WAKEUP"));
  assert(fake_haptic_count == 2);
}

int main(void) {
  test_start_and_stop();
  test_stale_callback_is_ignored();
  test_log_lines_report_callbacks_and_haptics();
  test_clock_schedule_and_storage_failures();
  test_stop_storage_failure_keeps_committed_session();
  test_deadline_callback_and_command_reconciliation();
  test_forward_clock_change_reconciles_due_deadline();
  test_backward_clock_change_defers_deadline();
  test_full_queue_does_not_suppress_later_clock_check();
  test_full_queue_does_not_suppress_tally_rollover();
  test_natural_cycle_tally_and_haptics_persist();
  test_tally_persists_across_stop_and_start();
  test_invalid_then_valid_utc_recovers_before_scheduling();
  test_failed_transition_has_no_tally_or_haptic();
  test_launch_repair_and_invalid_record();
  test_schedule_error_codes_and_cookie();
  test_rollover_is_requested_once_per_day();
  test_recovery_failure_is_not_presented_as_scheduled();
  test_store_clear_reports_status();
  test_wakeup_catch_up_table();
  test_overdue_recovery_table();
  test_recovery_counts_each_elapsed_phase_once();
  test_wakeup_launch_reconciles_and_renders_committed_state();
  test_recovery_counts_completions_on_local_day_only();
  test_launch_repair_rolls_over_running_tally();
  test_long_recovery_tallies_only_current_local_day();
  test_early_callback_reschedules_same_deadline();
  test_stop_failure_keeps_persisted_session();
  test_menu_row_contract();
  test_menu_row_zero_resumes_paused_session();
  test_skip_cycles_phases_without_counting_and_ignores_stale_callback();
  test_skip_reconciles_due_deadline_first();
  test_skip_from_paused_and_timed_paused_starts_running_phase();
  test_skip_is_noop_while_inactive();
  test_skip_failures_retain_committed_state();
  test_skip_invalid_utc_preserves_committed_state();
  test_skip_restart_does_not_replay_haptic();
  test_pause_resume_freezes_persists_and_rejects_stale_callback();
  test_pause_due_boundary_reconciles_first();
  test_pause_resume_failures_retain_committed_state();
  test_pause_resume_reject_invalid_or_overflowing_clock();
  test_pause_does_not_commit_when_due_reconciliation_fails();
  test_paused_record_recovers_without_wakeup();
  test_middle_control_resumes_only_while_paused();
  test_control_enqueue_failure_is_reported();
  test_extend_adds_and_stacks_silently();
  test_extend_ignores_stale_callback_and_persists_across_restart();
  test_extend_reconciles_due_deadline_first();
  test_extend_is_not_available_outside_running();
  test_extend_press_during_timed_pause_does_not_extend_resumed_phase();
  test_extend_failures_retain_committed_state();
  test_extend_invalid_or_overflowing_utc_preserves_committed_state();
  test_clock_check_retries_failed_due_transition();
  test_overdue_recovery_on_dst_change_day_counts_whole_local_day();
  test_menu_stop_and_start_reconcile_due_deadline_first();
  test_wakeup_launch_event_is_queued_behind_recovery_and_stale();
  test_recovery_failure_exits_report_error_and_retry();
  test_stop_paused_session_clears_saved_remainder();
  test_overdue_pause_end_recovery_does_not_cancel_invalid_id();
  test_stop_works_with_invalid_clock();
  test_stop_from_timed_pause_cancels_pause_wakeup();
  test_menu_resume_row_ends_timed_pause_early();
  test_pause_transitions_reset_tally_day_across_midnight();
  test_timed_pause_entry_change_resume_and_stale_callback();
  test_timed_pause_due_boundary_and_overdue_recovery();
  test_pause_end_recovery_failures_and_restart_haptic_marker();
  test_timed_pause_failure_retains_committed_state();
  test_timed_pause_invalid_utc_preserves_committed_state();
  test_timed_pause_store_validation();
  test_store_round_trip();
  test_store_rejects_invalid_records();
  test_invalid_record_is_persisted_as_error_until_replaced();
  puts("session coordinator tests passed");
  return 0;
}
