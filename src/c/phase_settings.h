#pragma once

#include "session_store.h"

#include <pebble.h>
#include <stdbool.h>
#include <stdint.h>

/* Configured length of a phase in minutes (5..60, steps of 5). A missing, wrong-sized, or invalid
 * stored value reads as that phase's default (Sit 45, Stand 10, Move 5), per phase. */
int phase_settings_get_minutes(SessionPhase phase);
/* Adds 5 minutes to the phase's length, wrapping 60 to 5, and saves. Returns false, leaving the
 * stored lengths unchanged, if the phase is invalid or the write fails. */
bool phase_settings_step(SessionPhase phase);
