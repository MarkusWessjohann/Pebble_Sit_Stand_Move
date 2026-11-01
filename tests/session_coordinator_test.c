#include "session_coordinator.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define STORE_KEY 0x5353

struct AppTimer {
  AppTimerCallback callback;
  void *context;
  bool pending;
};

static uint8_t s_persisted[64];
static int s_persisted_size;
static struct AppTimer s_timer;
static WakeupHandler s_wakeup_handler;
static WakeupId s_next_wakeup_id;

time_t fake_now;
bool fake_fail_schedule;
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
  if (key != STORE_KEY || fake_fail_write || size > sizeof(s_persisted)) {
    return -1;
  }
  memcpy(s_persisted, data, size);
  s_persisted_size = (int)size;
  return s_persisted_size;
}

bool persist_delete(uint32_t key) {
  if (key != STORE_KEY) {
    return false;
  }
  s_persisted_size = 0;
  return true;
}

WakeupId wakeup_schedule(time_t timestamp, int32_t cookie, bool notify_if_missed) {
  (void)cookie;
  assert(!notify_if_missed);
  ++fake_schedule_count;
  if (fake_fail_schedule) {
    return (WakeupId)-1;
  }
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
  memset(&s_timer, 0, sizeof(s_timer));
  s_wakeup_handler = NULL;
  s_next_wakeup_id = 1;
  fake_now = 1800000000;
  fake_fail_schedule = false;
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
  assert(session_store_save(&record));
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
  assert(session_store_save(&record));
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
  assert(persist_write_data(STORE_KEY, invalid_record, sizeof(invalid_record)) == sizeof(invalid_record));
  session_coordinator_init();
  fake_run_timers();
  view = prv_view();
  assert(view.record.lifecycle == SESSION_INACTIVE);
  assert(view.error == SESSION_ERROR_RECORD);
  assert(fake_cancel_all_count == 1);
}

int main(void) {
  test_start_and_stop();
  test_stale_callback_is_ignored();
  test_clock_schedule_and_storage_failures();
  test_stop_storage_failure_keeps_committed_session();
  test_deadline_callback_and_command_reconciliation();
  test_launch_repair_and_invalid_record();
  puts("session coordinator tests passed");
  return 0;
}
