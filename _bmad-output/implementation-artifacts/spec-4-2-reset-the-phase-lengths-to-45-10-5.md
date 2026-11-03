---
title: 'Reset the phase lengths to 45/10/5'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
baseline_commit: '797111027944a0e845ffba5d57c912d9d43b7836'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After changing the Sit, Stand, and Move lengths there is no single action to return to the standard 45/10/5 cycle.

**Approach:** Add a `Reset` row after `Move` in the Settings window. Selecting it writes the three defaults in one persist write (`phase_settings_reset()`), reloads the rows, and keeps the window open. No preview plays, the haptic pattern is untouched, and a failed write leaves the previous values stored and shown. A running phase, a paused phase, and Wakeups are unchanged; the defaults apply when each phase next starts.

</frozen-after-approval>

## Implementation Notes

- `phase_settings_reset()` in `src/c/phase_settings.c/.h` writes the three defaults as the same 3-byte record under key `0x5355`, so one write replaces all three and no half-reset state exists. It leaves `session_store.c`, the haptic key, and the coordinator unchanged.
- `src/c/sit_stand_move.c`: the Settings window has eight rows; `Reset` is `RESET_ROW` after `Move`. Selecting it resets, reloads the rows, keeps the window open, and plays no preview. A failed write leaves the stored values, and the reload shows them.
- Five new `test_phase_lengths_reset_*` tests: reset from changed values, at the defaults, from no record, and from a corrupt record; a failed write leaving the old values; the haptic pattern, running phase, Wakeup, and generation unchanged; paused and timed-paused remaining time unchanged; persistence across relaunch, Stop, and Start. A deliberate break (reset keeping the stored Stand and Move) was caught by the first test.
- `DESIGN.md` and `EXPERIENCE.md` describe the `Reset` row.
- Not built, not seen on a display: the Pebble SDK is not available in this sandbox, and `sit_stand_move.c` is not compiled by the host harness.


## Review Triage Log

- No test covers the Reset row in `sit_stand_move.c` (Blind): deferred. The host harness has no MenuLayer stubs; same gap as Stories 1.6, 1.7, 2.1, 3.1, 3.2, and 4.1. Already recorded in `deferred-work.md` for the Settings window; extended there to the Reset row.
- Reset writes even when nothing changes (Blind): low, not fixed. One 3-byte write on a manual Select press; a guard adds a read and a branch for no user-visible gain.
- No confirmation or undo, no feedback at the defaults or after a failed save (Blind): false as a defect. Intent and acceptance criteria require no confirmation, no preview, and the old values staying shown; the reset is reversible with a few Select presses.
- `Reset` label is ambiguous next to the haptic rows (Blind): low, not fixed. The label and its position after the three length rows were approved in Story 4.2; a longer label would also have to fit the row width (unverified on a display).
- UI ignores the return value of `phase_settings_reset()` (Blind): false. The old values staying shown is the specified behavior, and it matches the haptic and length rows.
- Failure test is shallow, fake write is all-or-nothing (Blind): low, not fixed. The real storage write is one call, so no half-written record can occur; the test asserts that no value changed.
- Docs: `EXPERIENCE.md` lines 26 and 92 do not mention Reset (Blind): false. Line 26 is still correct and line 92 describes the Settings menu command; the behavior is specified in the Settings window row.
- No emulator or device evidence record (Blind): deferred, see the first row. Unverified items are stated in Implementation Notes.
