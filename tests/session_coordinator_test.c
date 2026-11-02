#include "session_coordinator.h"

#include <assert.h>
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
WakeupId fake_last_scheduled_id;
time_t fake_last_scheduled_deadline;
int fake_haptic_count;

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
  (void)id;
  ++fake_cancel_count;
}

void wakeup_cancel_all(void) {
  ++fake_cancel_all_count;
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
  fake_last_scheduled_id = (WakeupId)-1;
  fake_last_scheduled_deadline = 0;
  fake_haptic_count = 0;
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
  assert(s_persisted_size == 33);

  WakeupId old_id = view.record.wakeup_id;
  int32_t old_generation = (int32_t)view.record.generation;
  session_coordinator_select_menu_row(2);
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.record.wakeup_id == (WakeupId)-1);
  assert(fake_cancel_count == 1);

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
  (void)prv_view();
  fake_run_timers();
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

  fake_fail_write = false;
  session_coordinator_recover();
  fake_run_timers();
  view = prv_view();
  assert(fake_schedule_count >= 4);
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
  uint16_t expected[3] = { 0, 0, 0 };
  int64_t deadline = midnight - 40 * 3600;
  SessionPhase phase = SESSION_PHASE_SIT;
  while (deadline <= fake_now) {
    struct tm *completion_day = localtime((time_t *)&deadline);
    if (completion_day && (completion_day->tm_year + 1900) * 1000 + completion_day->tm_yday == today) {
      ++expected[phase - 1];
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
  int writes = s_write_attempts;
  session_coordinator_select_menu_row(0);
  fake_run_timers();
  SessionView after = prv_view();
  assert(after.record.lifecycle == SESSION_PAUSED);
  assert(after.record.phase == view.record.phase);
  assert(after.record.remaining_seconds == 45 * 60);
  assert(after.record.deadline_utc == 0);
  assert(after.record.wakeup_id == (WakeupId)-1);
  assert(after.record.generation == view.record.generation + 1);
  assert(fake_schedule_count == 1);
  assert(fake_cancel_count == cancels + 1);
  assert(s_write_attempts == writes + 1);
  assert(fake_haptic_count == 0);

  session_coordinator_select_menu_row(0);
  fake_run_timers();
  after = prv_view();
  assert(after.record.lifecycle == SESSION_RUNNING);
  assert(after.record.phase == view.record.phase);
  assert(after.record.deadline_utc == fake_now + 45 * 60);
  assert(after.record.wakeup_id == fake_last_scheduled_id);
  assert(fake_schedule_count == 2);
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
  assert(!session_coordinator_resume_if_paused());
  session_coordinator_pause();
  fake_run_timers();
  assert(session_coordinator_resume_if_paused());
  fake_run_timers();
  assert(prv_view().record.lifecycle == SESSION_RUNNING);
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
  assert(s_persisted_size == 33);
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
  assert(s_persisted_size == 33);
  ok = session_store_load(&loaded, &found);
  assert(ok && found);
  assert(loaded.lifecycle == SESSION_PAUSED);
  assert(loaded.phase == SESSION_PHASE_STAND);
  assert(loaded.remaining_seconds == 321);
  assert(loaded.deadline_utc == 0 && loaded.wakeup_id == (WakeupId)-1);
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
  uint8_t bytes[33];
  memcpy(bytes, s_persisted, sizeof(bytes));
  uint8_t mutated[33];
  memcpy(mutated, bytes, sizeof(bytes)); mutated[0] = 0; prv_expect_load_rejected(mutated, sizeof(mutated));
  memcpy(mutated, bytes, sizeof(bytes)); mutated[0] = 2; prv_expect_load_rejected(mutated, sizeof(mutated));
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

int main(void) {
  test_start_and_stop();
  test_stale_callback_is_ignored();
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
  test_pause_resume_freezes_persists_and_rejects_stale_callback();
  test_pause_due_boundary_reconciles_first();
  test_pause_resume_failures_retain_committed_state();
  test_pause_does_not_commit_when_due_reconciliation_fails();
  test_paused_record_recovers_without_wakeup();
  test_middle_control_resumes_only_while_paused();
  test_stop_paused_session_clears_saved_remainder();
  test_store_round_trip();
  test_store_rejects_invalid_records();
  test_invalid_record_is_persisted_as_error_until_replaced();
  puts("session coordinator tests passed");
  return 0;
}
