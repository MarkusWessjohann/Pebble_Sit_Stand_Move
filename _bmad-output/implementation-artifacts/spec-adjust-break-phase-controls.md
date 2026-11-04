---
title: 'Adjust break duration with hardware buttons'
type: 'feature'
created: '2026-11-04'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
context: []
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** During a timed break, the hardware buttons currently switch among fixed 5-, 10-, and 30-minute choices instead of adjusting the active countdown like the normal phase.

**Approach:** Keep starting a break at 30 minutes. During a break, the up button adds 5 minutes and the down button subtracts 5 minutes from the break deadline, using the normal phase's one-minute minimum. Show `+5` and `-5` labels during the break.

## Implementation Notes

- Reuse the coordinator-owned deadline adjustment, scheduling, persistence, and cancellation behavior for both running phases and timed breaks. Preserve the press-time lifecycle meaning so a queued break adjustment cannot affect a phase resumed by break completion.
- Timed-break duration storage must continue to validate persisted active breaks without requiring the old fixed 5/10/30-minute set. The 30-minute middle-button start and timed-break resume behavior remain unchanged.
- Update the host coordinator tests for plus/minus break adjustments, one-minute clamp/no-op, failure handling, and unchanged initial 30-minute break; update the running and break labels to the shared `+5`/`-5` treatment.
- Verification: `scripts/test.sh` and `git diff --check`. The host harness does not compile the Pebble UI source.

Implemented in `src/c/session_coordinator.c`, `src/c/session_coordinator.h`, `src/c/session_store.c`, `src/c/sit_stand_move.c`, and `tests/session_coordinator_test.c`. `scripts/test.sh` passes coordinator tests and Python wrapper tests; the wrapper reports Pebble SDK 4.33.1 is not active in this environment.

</frozen-after-approval>
