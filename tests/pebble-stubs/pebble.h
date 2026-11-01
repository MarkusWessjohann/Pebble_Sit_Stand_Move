#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef int32_t WakeupId;
typedef struct AppTimer AppTimer;
typedef void (*WakeupHandler)(WakeupId id, int32_t cookie);
typedef void (*AppTimerCallback)(void *context);

enum { S_SUCCESS = 0, E_RANGE = -8, E_DOES_NOT_EXIST = -9, APP_LAUNCH_WAKEUP = 1 };

time_t fake_time(time_t *result);
int persist_get_size(uint32_t key);
int persist_read_data(uint32_t key, void *data, size_t size);
int persist_write_data(uint32_t key, const void *data, size_t size);
int persist_delete(uint32_t key);
WakeupId wakeup_schedule(time_t timestamp, int32_t cookie, bool notify_if_missed);
void wakeup_cancel(WakeupId id);
void wakeup_cancel_all(void);
void wakeup_service_subscribe(WakeupHandler handler);
bool wakeup_get_launch_event(WakeupId *id, int32_t *cookie);
int launch_reason(void);
AppTimer *app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void *context);
void app_timer_cancel(AppTimer *timer);
void vibes_short_pulse(void);

#define time fake_time

void fake_reset(void);
void fake_run_timers(void);
void fake_deliver_wakeup(WakeupId id, int32_t cookie);

extern time_t fake_now;
extern bool fake_fail_schedule;
extern int fake_schedule_error;
extern int32_t fake_last_scheduled_cookie;
extern bool fake_fail_write;
extern int fake_schedule_count;
extern int fake_cancel_count;
extern int fake_cancel_all_count;
extern WakeupId fake_last_scheduled_id;
extern time_t fake_last_scheduled_deadline;
extern int fake_haptic_count;
