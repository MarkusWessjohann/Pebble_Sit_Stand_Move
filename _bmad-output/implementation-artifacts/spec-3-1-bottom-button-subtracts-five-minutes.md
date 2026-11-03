---
title: 'Bottom button subtracts 5 minutes instead of adding 10'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '613323257821f9c6e2fdcb3be52532a50cc0a663'
context:
  - _bmad-output/implementation-artifacts/spec-1-7-extend-the-active-phase.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** While a session is running, the bottom (down) button adds 10 minutes to the phase and is labelled `+10`. The owner wants it to shorten the phase instead.

**Approach:** While `running`, the bottom button subtracts 5 minutes from the active phase deadline and its label reads `-5`. It uses the same coordinator-owned schedule/persist/cancel path as Extend and stays silent. Top (`+5`), middle, the timed-pause mapping, and menu behavior are unchanged.

## Boundaries & Constraints

**Always:** Reconcile a due deadline before applying the change, then apply it to the resulting running phase. Subtract from the current deadline, so repeated presses and mixed `+5`/`-5` presses stack. Keep phase, default durations, tally counts and `haptic_generation` unchanged. Increment generation, schedule a Wakeup for the new deadline, save, then cancel the prior Wakeup. On schedule or persist failure keep the committed state and show a visible error. All state and timing mutation stays in the coordinator. A press's meaning is fixed when it is pressed (a timed-pause press that reconciles into running must not subtract).

**Never:** Request a haptic. Change the top button, the middle button, the timed-pause top/bottom behavior (shorter/longer break), menu rows, or add UI-owned timing. Show `-5` outside `running`. Claim Pebble Time 2 behavior verified from host or emulator output.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Subtract | Running, more than 5 min remaining | Deadline -= 300 s; new generation and Wakeup; tally, phase, haptic marker unchanged; no haptic; label `-5` | N/A |
| Stack | Repeated `-5` / mixed with `+5` | Deadline = original + sum of changes | N/A |
| Due deadline | `now >= deadline` | Reconcile due phase first, then subtract from the resulting phase | If reconcile fails, drop the press |
| Timed pause | `timed_paused` | Unchanged: bottom selects the longer break | N/A |
| Not running | `inactive`, `paused` | Bottom opens the menu; nothing changes; no `-5` label | N/A |
| Schedule or save fails | Wakeup or storage error | Committed record kept, candidate Wakeup cancelled, no haptic | Visible error |
| Invalid UTC | Clock invalid | Committed state kept; nothing scheduled | Clock error |
| Less than 5 min remaining | Running, remaining <= 300 s after reconcile | Deadline set to now + 60 s (clamp); if remaining is already <= 60 s, nothing changes and no error. Never ends the phase; no haptic | N/A |

**Decision (owner):** When 5 minutes or less remain, `-5` clamps the phase to 1 minute remaining. It never ends the phase by itself.

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c:12-13` -- `SESSION_EXTEND_BOTTOM_SECONDS = 10 * 60`. Change to a -5 minute value (or a separate constant).
- `src/c/session_coordinator.c:456` `prv_extend(int32_t seconds)` -- rejects `seconds <= 0` and adds `seconds` to the deadline with an `INT32_MAX` overflow guard. Generalise to accept a negative delta, add the clamp-to-60-seconds rule (deadline never moved earlier than now + 60 s), and keep the candidate/schedule/save/cancel order.
- `src/c/session_coordinator.c:936` `prv_process` -- routes top/bottom to `prv_extend` when `event->extend_seconds != 0`. Keep the routing, since a signed non-zero value still works.
- `src/c/session_coordinator.c:1057` `session_coordinator_bottom_control` -- passes `SESSION_EXTEND_BOTTOM_SECONDS` while running. Do not change the lifecycle gate.
- `src/c/sit_stand_move.c:115` -- `"+10"` label for the bottom control while running. Change to `"-5"`. Click handlers (`:230`) stay as they are.
- `tests/session_coordinator_test.c` -- the `600` bottom-extend expectations at `:1680` (stack), `:2256` (day rollover) and the bottom case in `test_extend_reconciles_due_deadline_first` (`:1744`) must be updated. Add tests for the matrix, including the clamp.
- `_bmad-output/planning-artifacts/epics.md` (FR7, UX-DR6, UX-DR12, UX-DR16, Stories 1.7 and 2.1) and `ux-designs/*/DESIGN.md:70`, `EXPERIENCE.md:83` -- mention bottom `+10`. Do not edit; note the drift in Implementation Notes.
- `scripts/test.sh` runs the C and Python host tests. `sit_stand_move.c` is not compiled by the host harness.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_coordinator.c` -- bottom control while running applies -5 minutes, with the clamp-to-1-minute rule -- the new behavior
- [x] `src/c/sit_stand_move.c` -- show `-5` as the running bottom label -- the new label
- [x] `tests/session_coordinator_test.c` -- update the existing `+10` expectations; cover subtract, stacking, due-deadline reconcile, failures, and the clamp and no-op short-remaining cases -- verify the matrix

**Acceptance Criteria:**
- Given a running session with more than 5 minutes remaining, when bottom is pressed, then the deadline moves earlier by 5 minutes once, a Wakeup for the new deadline is scheduled, and no haptic is requested.
- Given `+5` and `-5` presses, when each commits, then the deadline is the original plus the net change.
- Given a running session, when the view renders, then the bottom label reads `-5` and `+10` appears nowhere.
- Given 5 minutes or less remain, when bottom is pressed, then the phase is left with 1 minute remaining (or unchanged if already <= 1 minute), never ended.
- Given a timed pause, when bottom is pressed, then it still selects the longer break.

## Implementation Notes

- `prv_extend` now accepts a signed delta. `SESSION_EXTEND_BOTTOM_SECONDS` is `-5 * 60`; `SESSION_SUBTRACT_FLOOR_SECONDS` is 60. For a negative delta the new deadline is `max(deadline - 300, now + 60)`. If that is not earlier than the current deadline (<= 60 s remaining) it returns with no change, no write, no schedule and no error. The candidate/schedule/save/cancel order, `haptic_generation` and the press-time routing are untouched.
- `sit_stand_move.c` shows `-5` for the running bottom label. It is not compiled by the host harness and was not run on an emulator or Pebble Time 2 here.
- Planning-doc drift (not edited): `epics.md` FR7 (:39), UX-DR6 (:90), UX-DR12 (:102), UX-DR16 (:112), FR map (:124), Story 1.7 (:447) and Story 2.1 (:556), plus `ux-designs/ux-sit_stand_go-2026-10-23/DESIGN.md` (:70, :83) and `EXPERIENCE.md` (:36, :83), still describe the bottom button as `+10`. Spec `spec-1-7-extend-the-active-phase.md` is likewise historical.

## Review Triage Log

| Finding | Verdict | Route | Evidence |
|---------|---------|-------|----------|
| Clamp fired at 301-359 s remaining (`max(deadline - 300, now + 60)`), contradicting the matrix (clamp only at <= 300 s) | medium | patch (fixed) | Reproduced by reading `prv_extend`: at 301 s, subtracting 300 s left 1 s, so `+5`/`-5` did not cancel. Now clamps only when remaining <= 300 s. |
| Clamp boundary untested (359/360/361 s); 301 s grouped under "5 min or less" | medium | patch (fixed) | Test now has 300/299/61 s in the clamp group and 301/359/360/361/1000 s asserting a full -300 s. |
| `-5` label not covered by a host test (`sit_stand_move.c` not compiled by the harness) | low | defer | Pre-existing harness limit; manual emulator check stays in Verification. |
| No-op press at <= 60 s gives no feedback | false | rejected | Spec explicitly decides: nothing changes, no error. |
| `SESSION_EXTEND_BOTTOM_SECONDS` name/sign, `== 0` guard | low | rejected | `extend_seconds` is `int32_t`; only top/bottom set it; fix is naming only. |
| Overflow check before clamp | false | rejected | The clamp can only lower the deadline or hit the no-op branch, so it cannot exceed `INT32_MAX`. |
| Matrix rows `inactive`/`paused`/`timed_paused` and "no `+10` anywhere" lack new tests | false | rejected | Covered by existing `test_extend_is_not_available_outside_running` and timed-pause tests; `rg '\+10' src` shows no remaining label. |

- Review patch: the clamp now applies only when remaining <= 300 s (`deadline - now <= 300`), not whenever `deadline - 300 < now + 60`.

## Verification

**Commands:**
- `scripts/test.sh` -- expected: all C and Python host tests pass

**Manual checks (if no CLI):**
- Run on the Pebble emulator: while running, the bottom label shows `-5` and pressing it shortens the countdown by 5 minutes. Record it as emulator evidence only.
