#pragma once

#include <pebble.h>
#include <stdbool.h>

/* The repeating Until Select alert. Volatile: never persisted, never resumed after relaunch. */

/* Starts repeating short pulses; restarts the cap if already running. The caller has already
 * played the first vibration. */
void haptic_alert_start(void);
/* Ends the alert and cancels any vibration in progress. No effect when not repeating. */
void haptic_alert_stop(void);
bool haptic_alert_active(void);
/* Select press: stops the alert and returns true if it was repeating, else returns false. */
bool haptic_alert_dismiss(void);
