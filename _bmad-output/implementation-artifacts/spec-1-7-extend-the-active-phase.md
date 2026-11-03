---
title: 'Extend the active phase'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '160e827cdcf9cf844a23af1261afc5cd7d1def47'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** While a session is running, the top and bottom hardware buttons only open the menu. The watch owner cannot stay in the current phase when they need more time.

**Approach:** Add a coordinator-owned Extend command. Top adds 5 minutes and bottom adds 10 minutes to the running phase deadline. It commits and schedules with the existing schedule/persist/cancel ordering, and it is silent. Show the `+5` / `+10` labels in the running session view.

## Boundaries & Constraints

**Always:** Reconcile a due deadline (`now >= deadline`) before applying Extend, then apply it to the resulting running phase. Add to the current deadline, so extensions stack. Keep the phase, the default durations, `remaining_seconds`, the tally counts and `haptic_generation` unchanged. Increment generation, schedule a Wakeup for the new deadline, save the record, then cancel the prior Wakeup. On schedule or persist failure keep the committed state, show a visible error, and do not present the new deadline as active. Keep all state/timing mutation in the coordinator.

**Never:** Request a haptic, extend from `inactive`, `paused` or `timed_paused`, change the timed-pause top/bottom behavior, add UI-owned timing, add menu rows, or claim physical Pebble behavior verified (Story 1.8).

**Decision (planner, from FR7 and "Given a session is running"):** Extend is valid only while `running`. In `paused` the top/bottom buttons keep opening the menu, as they do now. In `timed_paused` they keep selecting the shorter/longer break.

**Decision (planner, from DESIGN.md and UX-DR6/DR12):** While `running`, the controls line reads `TOP +5 BOTTOM +10`, matching the existing timed-pause line.

**Decision (planner):** A button's meaning is fixed when it is pressed. A top/bottom press made during a timed pause that reconciles into running before it is processed must not extend the phase.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Extend +5 / +10 | Running, deadline future | Deadline += 300 / 600 s; new generation and Wakeup; tally, phase, haptic marker unchanged; no haptic | N/A |
| Stack | Two or more extensions | Deadline = original + sum | N/A |
| Due deadline | `now >= deadline` | Reconcile the due phase first, then extend the resulting phase | If reconcile fails, drop Extend |
| Not running | `inactive`, `paused` | Hardware control returns false (menu opens); nothing changes | N/A |
| Timed pause | `timed_paused` | Unchanged: top/bottom change the break length | N/A |
| Stale callback | Callback for the superseded deadline | Ignored; no transition or haptic | None |
| Schedule or save fails | Wakeup or storage error | Committed record kept, candidate Wakeup cancelled, no haptic | Visible error |
| Invalid UTC / overflow | Clock invalid, or new deadline > `INT32_MAX` | Committed state kept; nothing scheduled | Clock error |

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c` -- add `prv_extend(int32_t seconds)` modelled on `prv_change_timed_pause` (`:357`) and `prv_skip` (`:400`). It reads the clock with `prv_read_utc` and calls `prv_invalid_time` on failure. It copies `s_record`, bumps `deadline_utc` and generation, then runs `prv_schedule`, `session_store_save`, then cancels the old id; reuse `prv_reset_tally_day`, `prv_next_generation` and `prv_commit_failure`. Do not touch `haptic_generation` or call `prv_request_haptic`. Guard `deadline + seconds > INT32_MAX` with `SESSION_ERROR_CLOCK`.
- `src/c/session_coordinator.c` `prv_process` (`:876`) -- `EVENT_TOP_CONTROL` / `EVENT_BOTTOM_CONTROL` currently always call `prv_change_timed_pause`. Add an `extend_seconds` field to `SessionEvent` (0 when not an extend), set at enqueue time. Route to `prv_extend` when it is non-zero and the lifecycle is still `running`; otherwise keep `prv_change_timed_pause`. The pre-command block (`:815`) already reconciles and repairs a lost Wakeup for running sessions.
- `src/c/session_coordinator.c` `session_coordinator_top_control` / `_bottom_control` (`:985`) -- return false unless `running` or `timed_paused`. Pass `extend_seconds` 300 / 600 for `running`.
- `src/c/sit_stand_move.c` `prv_render` (`:98`) -- set `s_controls_text` to `TOP +5 BOTTOM +10` for `running`. The click handlers already re-render on `true`.
- `tests/session_coordinator_test.c` -- `test_control_enqueue_failure_is_reported` (`:1615`) asserts top/bottom return false while running; update it. Add Extend tests with the fake clock, Wakeup, storage-failure and haptic fixtures.
- `scripts/test.sh` runs the C and Python tests.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_coordinator.c` -- implement Extend and route the running top/bottom controls to it, keeping timed-pause behavior.
- [x] `src/c/sit_stand_move.c` -- show the running `+5` / `+10` control labels.
- [x] `tests/session_coordinator_test.c` -- update the running top/bottom assertion; cover the matrix, stacking, due-deadline reconcile, stale callback, failures, restart persistence and queue-full reporting.

**Acceptance Criteria:**
- Given a running session, when top or bottom is pressed, then the deadline moves by 5 or 10 minutes once, a Wakeup for the new deadline is scheduled, and no haptic is requested.
- Given several extensions, when each commits, then the deadline is the original plus the sum, and the next phase still gets its default duration.
- Given the deadline is due when Extend is processed, when handled, then the due transition is reconciled first and Extend applies to the resulting phase.
- Given schedule or persistence fails, when attempted, then the committed deadline and Wakeup stay, an error is shown, and no haptic is requested.
- Given a callback for the superseded deadline arrives, when validated, then it is ignored with no transition or haptic.
- Given the app restarts after a committed extension, when recovered, then the extended deadline is kept.
- Given a timed-pause press reconciles into running before it is processed, when handled, then the phase is not extended.

## Implementation Notes

- `prv_extend(seconds)` is modelled on `prv_change_timed_pause` and `prv_skip`: it reads the clock, relies on the pre-command block plus `prv_reconcile_before_command` for a due deadline, guards `deadline + seconds > INT32_MAX`, then runs candidate, schedule, save, cancel-old. It never touches `haptic_generation` and never requests a haptic.
- `SessionEvent.extend_seconds` is set when the button is pressed (300 top, 600 bottom, 0 in timed pause). `prv_process` routes to `prv_extend` only when it is non-zero, so a timed-pause press that reconciles into running does not extend the resumed phase.
- `session_coordinator_top_control` / `_bottom_control` now return true for `running` and `timed_paused`. `paused` and `inactive` still open the menu.
- `sit_stand_move.c` shows `TOP +5 BOTTOM +10` while running. It is not compiled by the host harness. The 17-character line in the 14pt controls layer is unverified on Pebble Time 2 (Story 1.8).
- Waiver for Story 1.1 readiness was extended to this story on 2026-11-03 in `epics.md` and `epic-1-context.md`.

## Spec Change Log

## Review Triage Log

- medium (patch) — Extend's `prv_reset_tally_day` is unpinned: removing it leaves every test green (mutant survived), so the first Extend after local midnight would persist the previous day's counts. Add `DAY_EXTEND_TOP` / `DAY_EXTEND_BOTTOM` cases to the midnight table test. (verification-gap, blind-hunter, edge-case-hunter)
- low (rejected) — Extend queued behind another command (Pause, Skip, Stop) lands on a different state or is dropped silently. Each command is a separate button/menu action drained in order; `prv_extend` re-checks `running`, and other commands behave the same way. No named harm. (blind-hunter, edge-case-hunter)
- low (rejected) — The pre-command repair block (CLOCK error or lost Wakeup) reloads and returns, dropping the Extend. Pre-existing behavior for every command (Skip, Pause, timed pause); the repair clears the error and the user presses again. (edge-case-hunter)
- low (rejected) — No cap on stacked extensions beyond the `INT32_MAX` guard. FR7 says extensions stack and names no cap; a cap would add behavior the intent does not ask for. (edge-case-hunter, blind-hunter)
- false — Invalid UTC cancels the committed Wakeup in memory. `prv_invalid_time` is the AD-10 behavior shared with `prv_skip` and `prv_resume`; the committed record is untouched and the test asserts that. (edge-case-hunter)
- false — Top/bottom no longer reach the menu while running. A long press on Select still opens it (`prv_menu_click_handler`); `paused` and `inactive` still fall through to the menu, and `test_extend_is_not_available_outside_running` asserts that. (blind-hunter)
- false — `s_controls_text` may be too small for the new label. It is `char[24]`; `TOP +5 BOTTOM +10` needs 18 bytes. (blind-hunter)
- false — Redundant reconcile in `prv_extend`. `prv_reconcile_before_command` is idempotent and `prv_timed_pause` and `prv_change_timed_pause` call it the same way. (blind-hunter)
- low (rejected) — Weak assertions in `test_extend_failures_retain_committed_state`, untested `prv_next_generation` failure branch. The cancel-count assertion already covers the schedule-failure case; generation exhaustion is untested for every command. (blind-hunter)
- low (rejected) — Spec/process notes: stale Code Map line numbers, status sync, Verification results, redundant waiver sentence in `epics.md`. Fix would edit the build's own spec or planning prose; sprint status is synced in the final step. (blind-hunter)
- defer — On Pebble Time 2, check `TOP +5 BOTTOM +10` fit and legibility in the 14pt controls line, and that top/bottom presses and the Extend Wakeup behave as intended. The host harness does not compile `sit_stand_move.c`. (blind-hunter, verification-gap)

### Review Findings (code review, 2026-11-03)

Layers: Blind Hunter, Edge Case Hunter, Verification Gap, Acceptance Auditor. 0 decision-needed, 0 patch, 0 new defer, 22 findings rejected. The Verification Gap layer found no gaps; the Acceptance Auditor found no AC violation.

Rejected:
- carried (low) — Extend queued behind Pause/Skip/Stop is dropped silently; button returns true. Same claim as the build review; `prv_extend` re-checks `running` like every other command. (blind-hunter, edge-case-hunter)
- carried (low) — Midnight tally reset contradicts "tally unchanged". Matches every other committing transition and the reset-at-midnight requirement; already pinned by `DAY_EXTEND_*`; the fix would edit the spec. (edge-case-hunter, acceptance-auditor, blind-hunter)
- carried (low) — `prv_next_generation` failure branch untested; same as all other commands. (blind-hunter, edge-case-hunter)
- carried (low) — Weak `fake_last_cancelled_id` assertion. It passes trivially in the schedule-failure case, but the cancel-count assertion on the next line covers it. (blind-hunter)
- carried (low) — Waiver wording repeats the 2026-11-03 phrase in `epics.md`; cosmetic planning prose. (blind-hunter, acceptance-auditor)
- carried (low) — Label and click handlers are not compiled by the host harness. Already in `deferred-work.md` for Story 1.8. (blind-hunter, verification-gap, acceptance-auditor)
- low — Overflow guard reports `SESSION_ERROR_CLOCK` for a cap problem, and the next press runs clock repair. Reachable only within about five minutes of INT32_MAX (January 2038); `prv_skip`, `prv_resume` and `prv_timed_pause` use the same error. (blind-hunter, edge-case-hunter)
- low — The running label stays visible while an error or lost Wakeup makes the first press repair-only. Every command behaves this way in that state; a label guard adds a branch for a transient state. (edge-case-hunter)
- low — Invalid-UTC test checks only the in-memory record. The invalid-clock path never writes (`prv_invalid_time`), and the overflow half asserts `s_write_attempts`. (edge-case-hunter)
- low — `prv_read_utc` failure inside `prv_extend` is unreachable via commands, because the pre-command block catches it first. Defensive and consistent with `prv_change_timed_pause`. (acceptance-auditor)
- low — Overflow guard not tested at the exact `INT32_MAX` boundary. The comparison is a single `>`; the near-boundary case is tested. (blind-hunter)
- low — A +5 pressed at a due deadline extends the next phase. The Story 1.7 AC requires reconcile first, then Extend on the resulting state. Only one haptic results (the transition; Extend is silent). (blind-hunter)
- low — Redundant `s_controls_text[0] = '\0'` before the running label. Negligible. (blind-hunter)
- low — `test_control_enqueue_failure_is_reported` lost its running-state false assertions. The new Extend tests cover top/bottom while running. (blind-hunter)
- false — Top/bottom no longer reach the menu: long Select still opens it, and the UX spec fixes top/bottom as +5/+10. No README or doc mentions the old behavior. (blind-hunter)
- false — Schedule and save failure paths are inconsistent. Schedule failure sets `SESSION_ERROR_WAKEUP` and save failure uses `prv_commit_failure(true)`, exactly as in `prv_skip` and `prv_resume`. (blind-hunter)
- rejected (edits the spec or process) — Spec `done` vs sprint `review`, stale Code Map line numbers, Verification results not recorded. Sprint status is synced in this step. (blind-hunter, acceptance-auditor)

## Verification

**Commands:**
- `scripts/test.sh` -- expected: C coordinator/store tests and Python wrapper tests pass; the Python wrapper may report SDK 4.33.1 inactive here.
- `git diff --check` -- expected: no whitespace errors.

**Manual checks (if no CLI):**
- Inspect the running controls label and the top/bottom handlers; the host harness does not compile the Pebble UI. Physical button, Wakeup and legibility behavior is Story 1.8.
