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

void wakeup_service_subscribe(WakeupHandler handler) {
  s_wakeup_handler = handler;
}

bool wakeup_get_launch_event(WakeupId *id, int32_t *cookie) {
  (void)id;
  (void)cookie;
  return false;
}

int launch_reason(void) {
  return 0;
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

  fake_now = (time_t)view.record.deadline_utc;
  session_coordinator_stop();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(fake_schedule_count == 3);
  assert(fake_cancel_count == 3);
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
  int schedules = fake_schedule_count;
  int writes = s_write_attempts;
  for (uint16_t row = 0; row < 2; ++row) {
    session_coordinator_select_menu_row(row);
    fake_run_timers();
    SessionView after = prv_view();
    assert(after.record.lifecycle == SESSION_RUNNING);
    assert(after.record.generation == view.record.generation);
    assert(after.record.wakeup_id == view.record.wakeup_id);
    assert(after.record.deadline_utc == view.record.deadline_utc);
    assert(after.error == SESSION_ERROR_NONE);
    assert(fake_schedule_count == schedules);
    assert(s_write_attempts == writes);
  }
}

static SessionRecord prv_full_record(void) {
  return (SessionRecord){
    .lifecycle = SESSION_RUNNING,
    .phase = SESSION_PHASE_STAND,
    .generation = 5,
    .haptic_generation = 3,
    .deadline_utc = 0x7ffffff0,
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
  r = valid; r.lifecycle = (SessionLifecycle)2; prv_expect_invalid(r);
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
  test_launch_repair_and_invalid_record();
  test_schedule_error_codes_and_cookie();
  test_rollover_is_requested_once_per_day();
  test_recovery_failure_is_not_presented_as_scheduled();
  test_store_clear_reports_status();
  test_wakeup_catch_up_table();
  test_overdue_recovery_table();
  test_early_callback_reschedules_same_deadline();
  test_stop_failure_keeps_persisted_session();
  test_menu_row_contract();
  test_store_round_trip();
  test_store_rejects_invalid_records();
  test_invalid_record_is_persisted_as_error_until_replaced();
  puts("session coordinator tests passed");
  return 0;
}
