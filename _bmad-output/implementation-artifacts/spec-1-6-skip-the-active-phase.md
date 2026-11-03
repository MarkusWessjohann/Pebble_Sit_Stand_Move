---
title: 'Skip the active phase'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: 'c65bd71e779e1d4294a33c231af6e0617c89d9f9'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The menu shows a Skip row, but it does nothing. The watch owner cannot move on from a phase that does not fit the moment.

**Approach:** Add a coordinator-owned Skip command that advances immediately to the next phase (Sit → Stand → Move → Sit) at its default duration, commits and schedules it with the existing schedule/persist/cancel ordering, and requests one haptic. Wire the native menu Skip row to it.

## Boundaries & Constraints

**Always:** Reconcile a due deadline (`now >= deadline`, phase or PauseEnd) before applying Skip, then apply Skip to the resulting state. Skip never increments the tally and never carries remaining time forward. Increment generation; persist the haptic-attempt marker with the new record before calling the haptic API; cancel the prior Wakeup only after commit. On schedule/persist failure keep the committed state, show a visible error, and do not present the new phase as active. Keep all state/timing mutation in the coordinator.

**Never:** Count the skipped phase as completed, change default durations, add haptics to other commands, add UI-owned timing, or claim physical Pebble behavior verified (Story 1.8).

**Decision (owner, 2026-11-03):** Skip is also valid from `paused` and `timed_paused`. It advances to the next phase, leaves the pause (cancelling any pause-end Wakeup), and starts that phase `running` at its full default duration, with one haptic. It is a no-op while inactive.

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Skip running | Running phase, deadline future | Next phase, `now` + default duration, new Wakeup, tally unchanged, one haptic | Retain committed state; show error |
| Skip paused | `paused` or `timed_paused` | Next phase `running` at default duration; pause Wakeup cancelled; one haptic | Retain committed pause; show error |
| Skip at due deadline | `now >= deadline` | Reconcile due phase/PauseEnd first, then skip the resulting phase | If reconcile fails, drop Skip |
| Stale callback | Callback for the skipped phase or pause | Ignored; no transition or haptic | None |
| Invalid UTC | Clock invalid | Committed state kept; no scheduling or haptic | Clock error |

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c` -- add `EVENT_SKIP`/`prv_skip` modelled on `prv_resume` and `prv_wakeup`. Reuse `prv_next_phase`, `prv_duration`, `prv_next_generation`, `prv_schedule`, `prv_reset_tally_day`, `prv_commit_failure`, `prv_request_haptic`, and `prv_reconcile_before_command`. Set `haptic_generation = generation` before the haptic. Route menu row 1 in `prv_process` (`EVENT_MENU_SELECT`), and add Skip to `is_command`. The pre-command block covers only running/timed-paused; `prv_skip` reads its own clock for `paused`. Do not call `prv_reconcile_phase` for Skip.
- `src/c/session_coordinator.h` -- optionally expose `session_coordinator_skip()`; menu dispatch alone is sufficient.
- `src/c/sit_stand_move.c` -- `prv_menu_select` returns early for row 1 while active; remove that guard so Skip dispatches. No new render logic.
- `tests/session_coordinator_test.c` -- `test_menu_row_contract` asserts Skip is inert (around line 1046, and again while paused); update it. Add Skip tests using the fake clock, Wakeup, storage-failure, and haptic fixtures. Host tests do not compile Pebble UI.
- `_bmad-output/planning-artifacts/epics.md` Story 1.6 and `ARCHITECTURE-SPINE.md` AD-2, AD-7, AD-9–AD-12 are the authority.
- `scripts/test.sh` runs the C and Python tests; run it with `git diff --check`.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_coordinator.c` -- implement Skip from running, paused, and timed-paused with schedule/persist/cancel ordering and the haptic marker.
- [x] `src/c/sit_stand_move.c` -- let the Skip menu row dispatch while active.
- [x] `tests/session_coordinator_test.c` -- replace the inert-Skip assertions and cover the matrix, tally, and failure paths.

**Acceptance Criteria:**
- Given a running Sit, Stand, or Move, when Skip is selected, then the next phase in Sit → Stand → Move → Sit order starts at its default duration with a new deadline, Wakeup, and generation, and one haptic is requested.
- Given any phase is skipped, when the tally is read, then no count changed.
- Given the phase deadline is due when Skip is processed, when handled, then the due transition is reconciled first and Skip applies to the resulting phase.
- Given Skip from `paused` or `timed_paused`, when processed, then the next phase runs at its default duration, the pause Wakeup is cancelled, and a later pause-end callback does nothing.
- Given schedule or persistence fails during Skip, when attempted, then the committed state remains, an error is shown, and no haptic is requested.
- Given a callback for the skipped phase arrives later, when validated, then it is ignored with no transition or haptic.
- Given the app restarts after a committed Skip with the haptic marker set, when recovered, then no duplicate haptic is requested.
- Given UTC is invalid, when Skip is selected, then committed state is preserved and no scheduling or haptic occurs.

## Implementation Notes

- `prv_skip` is modelled on `prv_resume`: it builds a running candidate for the next phase at its default duration, clears pause fields, resets the tally day, bumps generation, sets `haptic_generation` before saving, then runs schedule, save, cancel-old, haptic. Skip never touches counts.
- Running and timed-paused Skip is reconciled by the existing pre-command block. Paused Skip has no deadline to reconcile, so `prv_skip` reads its own clock.
- Menu row 1 routes to `prv_skip`; the `sit_stand_move.c` early return that made the row inert is removed. `session_coordinator_skip()` is a new void wrapper, like `start`, `pause`, `resume` and `stop`.
- Skip from PAUSED or BREAK is an owner decision (2026-11-03) recorded in the frozen block. Waiver for Story 1.1 readiness was extended to this story in `epics.md` and `epic-1-context.md`.

## Review Triage Log

- patch (medium) — Skip's `prv_reset_tally_day` was unpinned: deleting it left every test green, and the first Skip after midnight from a pause would keep the previous day's counts. Added `DAY_SKIP_RUNNING` and `DAY_SKIP_PAUSED` cases to the midnight table test; verified they fail with the line removed. (verification-gap, blind-hunter)
- patch (low) — Skip's clock-overflow guard was untested because both clock tests used `fake_now = 0`. Added a PAUSED case at `INT32_MAX - 10` asserting `SESSION_ERROR_CLOCK`, an unchanged record, and no write, schedule or haptic. (verification-gap, edge-case-hunter, blind-hunter)
- patch (medium) — The rewrite of `test_menu_row_contract` dropped the only menu-row-0 Resume-from-PAUSED assertion. Added `test_menu_row_zero_resumes_paused_session`. (edge-case-hunter, blind-hunter)
- defer — The `sit_stand_move.c` guard removal and the Skip menu dispatch are not compiled or exercised by the host harness. Recorded in `deferred-work.md` for Story 1.8. (blind-hunter, edge-case-hunter, verification-gap)
- low (rejected) — Skip at an exact due deadline requests two haptics: the reconciled transition, then Skip. Each is a real committed transition, the spec orders reconcile first, and coalescing would add state and edit the frozen intent.
- low (rejected) — Skip from PAUSED with invalid UTC leaves a clock error until the next command. `prv_resume` and `prv_timed_pause` behave the same way, and the clock check does not run for paused sessions.
- false — The overflow guard sets the clock error inconsistently with `prv_invalid_time`: `prv_resume` and `prv_timed_pause` use the same `s_error = SESSION_ERROR_CLOCK` on their overflow guard, and the guard fires before any Wakeup exists to cancel.
- false — Menu row 1 may have changed meaning while inactive: `prv_skip` returns on inactive, and `test_menu_row_contract` still asserts row 1 while inactive schedules, cancels, and writes nothing.
- false — The invalid-UTC test lets the in-memory record diverge from committed: the test asserts lifecycle, phase and deadline against the committed record, and the cleared in-memory Wakeup id is the AD-10 behavior (suppress scheduling, keep committed state).
- low (rejected) — `session_coordinator_skip()` returns `void` and drops a skip on a full queue: `start`, `pause`, `resume` and `stop` have the same signature, and the queue error is surfaced.
- low (rejected) — `prv_skip` repeats the candidate/schedule/save/cancel sequence used by `prv_resume` and `prv_timed_pause`, and the failure test uses magic numbers and a noisy setup: no named harm, and extraction is a larger refactor.
- low (rejected) — No test drives generation exhaustion in Skip: no coordinator command has one, so this follows the existing pattern and is not specific to Skip.

## Verification

**Commands:**
- `scripts/test.sh` -- expected: C coordinator/store tests and Python wrapper tests pass. Result: pass; the Python wrapper reports that SDK 4.33.1 is not active here and marks the CLI-dependent case unavailable.
- `git diff --check` -- expected: no whitespace errors.

**Manual checks (if no CLI):**
- Inspect the menu Skip dispatch; the host harness does not compile the Pebble UI. Physical haptic/Wakeup behavior is Story 1.8.
