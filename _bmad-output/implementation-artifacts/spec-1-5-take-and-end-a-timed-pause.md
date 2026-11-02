---
title: 'Take and end a timed pause'
type: 'feature'
created: '2026-11-02'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '480422cf49750c82ae97e89c53c8565ec45f0219'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** A running phase cannot yet be suspended for a fixed break and resumed automatically without losing its place. The watch owner needs a 5-, 10-, or 30-minute timed pause with optional early resume.

**Approach:** Add coordinator-owned timed-pause transitions that persist the interrupted phase's remaining time and a separate pause-end UTC deadline, schedule and validate its Wakeup, and resume the same phase at break end. Render `BREAK` and the remaining break time, and map the middle control to 30-minute entry/early resume and top/bottom controls to the shorter/longer alternative durations.

## Boundaries & Constraints

**Always:** Use only 5, 10, and 30 minutes. Reconcile `now >=` the relevant deadline before a competing command. Preserve phase and positive phase remainder while timed-paused; do not advance or count a phase during break. Increment generation for mutations and validate callback lifecycle, generation/cookie, Wakeup ID, and deadline. Schedule/commit using existing coordinator ordering; keep the committed state authoritative on failure and show a visible error. Early resume cancels its pause Wakeup and is silent; PauseEnd resumes with `now + saved remainder`, requests at most one haptic, and requests app open/foreground. Overdue PauseEnd recovery follows the same-phase resume rule. Keep all state/timing mutation in the coordinator.

**Never:** Add other pause durations, phase advancement during a break, haptics for entry/duration change/early resume, UI-owned timing, new product surfaces, or claims of physical Pebble behavior verified; that remains Story 1.8.

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Start break | Running; choose 5/10/30 | Save phase remainder, commit `timed_paused`, schedule selected pause-end Wakeup; no haptic | Retain committed state; display error if scheduling or persistence fails |
| Change duration | Timed pause; choose top/bottom alternative | Reschedule pause deadline for selected duration; phase remainder is unchanged | Retain prior committed break and display error on failure |
| Early resume | Timed pause; middle control before deadline | Resume same phase with saved remainder, cancel pause Wakeup; no haptic | Retain committed timed pause if persistence fails |
| PauseEnd | Matching callback or due recovery | Resume same phase at current UTC plus saved remainder; no cycle advance; one haptic attempt and app-open request | Keep committed state and report failures; never claim uncommitted resume |
| Stale callback | Callback from superseded/early-resumed break | Ignore without haptic or app reopen | No error required |

</frozen-after-approval>

## Code Map

- `src/c/session_store.h`, `src/c/session_store.c` -- add timed-paused lifecycle validation and persist both positive phase remainder and pause-end deadline in a compact versioned record. Use a new schema version for the added field and retain loading of existing v1 records; running and manual-paused semantics must remain intact.
- `src/c/session_coordinator.c`, `src/c/session_coordinator.h` -- extend FIFO event processing, deadline reconciliation, recovery, Wakeup validation, view remaining-time derivation, menu dispatch, and public control APIs. Reuse `prv_pause`, `prv_resume`, `prv_schedule`, `prv_next_generation`, and existing commit/failure patterns. PauseEnd resumes the same phase; do not call phase-cycle reconciliation for it.
- `src/c/sit_stand_move.c` -- extend `prv_render`, middle-click handling, and click subscriptions for top/middle/bottom controls. During timed pause display `BREAK`, break time, and shorter/longer duration labels; during running middle starts 30 minutes; during timed pause middle resumes early.
- `tests/session_coordinator_test.c` -- extend fake clock, persisted-record, Wakeup, scheduling/storage failure, and haptic fixtures/tests. Cover duration entry/change, PauseEnd, overdue recovery, early resume, and stale callbacks. Host tests do not compile Pebble UI.
- `_bmad-output/planning-artifacts/epics.md` Story 1.5 and `architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` AD-2, AD-7, AD-9–AD-13, AD-16–AD-17 are the behavior and persistence/ordering authority.
- `scripts/test.sh` runs coordinator/store C tests and Python wrapper tests; use it with `git diff --check` for verification.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_store.h` and `src/c/session_store.c` -- support and strictly validate `timed_paused`, preserving both pause deadline and phase remainder and loading existing v1 records.
- [x] `src/c/session_coordinator.c` and `src/c/session_coordinator.h` -- add timed-pause entry, alternative-duration change, early resume and PauseEnd callback/recovery through the FIFO coordinator and existing scheduling/commit rules.
- [x] `src/c/sit_stand_move.c` -- wire hardware controls and render BREAK, break countdown, and fixed alternative duration labels.
- [x] `tests/session_coordinator_test.c` -- verify normal and failure paths, exact due boundary, recovery, no phase advancement, haptic at-most-once, and stale callback rejection.

**Acceptance Criteria:**
- Given a running phase with valid UTC, when 5, 10, or 30 minutes is selected, then `timed_paused` commits the same phase and its positive remaining duration with a scheduled pause-end deadline and no haptic.
- Given a running session, when the middle hardware control is pressed, then a 30-minute break begins and the view shows `BREAK` with remaining break time.
- Given a timed pause is active, when displayed, then it shows `BREAK`, break time, and top/bottom alternatives (5→10/30, 10→5/30, 30→5/10) without changing saved phase remainder.
- Given a timed pause is active, when a top/bottom alternative is selected, then the deadline and Wakeup are updated for that duration and callbacks for the superseded Wakeup are ignored.
- Given a matching PauseEnd is on time or overdue, when processed, then the same phase resumes at current UTC plus the saved remainder, no cycle/tally advancement occurs, and at most one haptic is requested with app open/foreground requested.
- Given a timed pause is resumed early from the middle control, when its deadline is still future, then its Wakeup is cancelled, the same phase resumes with saved remainder without haptic, and a later stale callback does not vibrate or reopen the app.
- Given Resume arrives at or after pause-end deadline, when processed, then PauseEnd is reconciled first and Resume causes no second transition or haptic.
- Given UTC is invalid during timed pause, when time-based processing occurs, then committed state is preserved and scheduling/haptics are suppressed until UTC is valid, after which PauseEnd is reconciled before scheduling resumes.
- Given timed-pause entry, duration change, resume, or PauseEnd scheduling/persistence fails, when the mutation is attempted, then the last committed state remains authoritative and a visible failure is shown.

## Implementation Notes

- Added schema v2 (45-byte record) with pause-end UTC and selected duration; valid v1 running/manual-paused records remain readable.
- Timed-pause lifecycle, deadline reconciliation, recovery, callback validation, and alternative changes use the existing FIFO coordinator and schedule/persist/cancel ordering. PauseEnd stores its haptic attempt with the resumed state before requesting the haptic.
- Middle starts a 30-minute break while running and resumes from manual or timed pause; top/bottom choose the available shorter/longer duration while timed-paused. Otherwise top/bottom open the native menu; while active, a long middle-button press also opens it.
- Direct hardware-control UI dispatch/render automation remains deferred to Story 1.8 because the host harness does not compile Pebble UI.

## Verification

**Commands:**
- `scripts/test.sh` -- C coordinator/store tests pass; Python wrapper tests pass (4 tests). Pebble SDK CLI wrapper reports that SDK 4.33.1 is not active in this host environment and marks the CLI-dependent case unavailable.
- `git diff --check` -- pass (no whitespace errors).

**Manual checks (if no CLI):**
- Inspect native control wiring and BREAK/countdown/alternative labels; host harness does not compile the Pebble UI. Physical Wakeup, haptic, and foreground behavior remain subject to Story 1.8.

## Review Triage Log

- patch — Top/bottom controls previously enqueued no-op events outside timed pause; they now return false so the UI opens the native menu, while timed-pause clicks still change duration.
- patch — Middle-control handling previously reported success after queue overflow; it now returns the enqueue result, and queue-full behavior is tested.
- patch — Clock refresh previously retried a due transition after a schedule/storage failure; retries now occur only for clean/queue states, and a regression test verifies no repeated schedule attempt.
- patch — The reduced phase-label layer risked clipping 24-point text; height was restored while retaining the separate controls layer.
- patch — Added overdue-recovery schedule/write failure tests to verify persisted timed-paused state and no haptic on an uncommitted PauseEnd.
- patch — Added restart coverage proving a committed PauseEnd haptic marker prevents a duplicate request.
- defer — Pebble SDK 4.33.1 is not active here and the host harness does not compile or exercise UI dispatch/rendering; validate app compilation, hardware-control mapping, labels, and layout in Story 1.8.
- patch — Identical invalid-time handling helpers were consolidated into one function.
- patch — Added malformed v2 pause-deadline high-word rejection coverage.
- patch — Added duration-change tests for both alternatives from a 10-minute pause, including their scheduled deadlines.
- defer — UI handler/render behavior has no automated host coverage; exercise control mapping and displayed text/layout during Story 1.8 target-device verification.
