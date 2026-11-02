#include "session_store.h"

#include <string.h>

enum {
  SESSION_STORE_KEY = 0x5353,
  SESSION_STORE_VERSION = 1,
  SESSION_STORE_SIZE = 33,
};

static void prv_write_u16(uint8_t *bytes, uint16_t value) {
  bytes[0] = (uint8_t)value;
  bytes[1] = (uint8_t)(value >> 8);
}

static void prv_write_u32(uint8_t *bytes, uint32_t value) {
  bytes[0] = (uint8_t)value;
  bytes[1] = (uint8_t)(value >> 8);
  bytes[2] = (uint8_t)(value >> 16);
  bytes[3] = (uint8_t)(value >> 24);
}

static uint16_t prv_read_u16(const uint8_t *bytes) {
  return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t prv_read_u32(const uint8_t *bytes) {
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

bool session_store_validate(const SessionRecord *record) {
  if (!record || record->generation == 0 || record->generation > INT32_MAX ||
      record->haptic_generation > record->generation || record->tally_day < 0 ||
      record->sit_count > 999 || record->stand_count > 999 || record->move_count > 999) {
    return false;
  }

  if (record->lifecycle == SESSION_INACTIVE) {
    return record->phase == SESSION_PHASE_NONE && record->deadline_utc == 0 &&
           record->remaining_seconds == 0 &&
           record->wakeup_id == (WakeupId)-1;
  }

  if (record->phase < SESSION_PHASE_SIT || record->phase > SESSION_PHASE_MOVE) {
    return false;
  }
  if (record->lifecycle == SESSION_RUNNING) {
    return record->deadline_utc > 0 && record->deadline_utc <= INT32_MAX &&
           record->remaining_seconds == 0 && record->wakeup_id >= 0 &&
           record->wakeup_id != (WakeupId)-1;
  }
  return record->lifecycle == SESSION_PAUSED && record->deadline_utc == 0 &&
         record->remaining_seconds > 0 && record->wakeup_id == (WakeupId)-1;
}

bool session_store_load(SessionRecord *record, bool *found) {
  uint8_t bytes[SESSION_STORE_SIZE];
  *found = false;
  int size = persist_get_size(SESSION_STORE_KEY);
  if (size == E_DOES_NOT_EXIST) {
    return true;
  }
  if (size != SESSION_STORE_SIZE || persist_read_data(SESSION_STORE_KEY, bytes, sizeof(bytes)) != size ||
      bytes[0] != SESSION_STORE_VERSION) {
    return false;
  }

  SessionRecord loaded = {
    .lifecycle = (SessionLifecycle)bytes[1],
    .phase = (SessionPhase)bytes[2],
    .generation = prv_read_u32(&bytes[3]),
    .haptic_generation = prv_read_u32(&bytes[7]),
    .deadline_utc = 0,
    .remaining_seconds = 0,
    .wakeup_id = (WakeupId)prv_read_u32(&bytes[19]),
    .tally_day = (int32_t)prv_read_u32(&bytes[23]),
    .sit_count = prv_read_u16(&bytes[27]),
    .stand_count = prv_read_u16(&bytes[29]),
    .move_count = prv_read_u16(&bytes[31]),
  };
  uint32_t time_low = prv_read_u32(&bytes[11]);
  uint32_t time_high = prv_read_u32(&bytes[15]);
  if (loaded.lifecycle == SESSION_PAUSED) {
    if (time_high != 0 || time_low > INT32_MAX) {
      return false;
    }
    loaded.remaining_seconds = (int32_t)time_low;
  } else {
    loaded.deadline_utc = (int64_t)((uint64_t)time_low | ((uint64_t)time_high << 32));
  }
  if (!session_store_validate(&loaded)) {
    return false;
  }
  *record = loaded;
  *found = true;
  return true;
}

bool session_store_save(const SessionRecord *record) {
  if (!session_store_validate(record)) {
    return false;
  }
  uint8_t bytes[SESSION_STORE_SIZE] = { SESSION_STORE_VERSION, record->lifecycle, record->phase };
  prv_write_u32(&bytes[3], record->generation);
  prv_write_u32(&bytes[7], record->haptic_generation);
  uint64_t stored_time = record->lifecycle == SESSION_PAUSED
                             ? (uint32_t)record->remaining_seconds
                             : (uint64_t)record->deadline_utc;
  prv_write_u32(&bytes[11], (uint32_t)stored_time);
  prv_write_u32(&bytes[15], (uint32_t)(stored_time >> 32));
  prv_write_u32(&bytes[19], (uint32_t)record->wakeup_id);
  prv_write_u32(&bytes[23], (uint32_t)record->tally_day);
  prv_write_u16(&bytes[27], record->sit_count);
  prv_write_u16(&bytes[29], record->stand_count);
  prv_write_u16(&bytes[31], record->move_count);
  return persist_write_data(SESSION_STORE_KEY, bytes, sizeof(bytes)) == sizeof(bytes);
}

bool session_store_clear(void) {
  return persist_delete(SESSION_STORE_KEY) == S_SUCCESS;
}
