# Epic 4 Context: Choose how long each phase lasts

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

The owner sets the length of Sit, Stand, and Move in the Settings window (5 to 60 minutes in steps of 5) and can reset them to 45/10/5. A new length applies to phases that start afterwards. The running phase, pauses, Wakeups, timed-pause choices, and Extend are unchanged, and the daily tally keeps counting naturally completed phases. Lengths survive restarts and Stop. Builds on Epics 1 to 3; does not reopen the waived physical-device story.

## Stories

- Story 4.1: Set the length of Sit, Stand, and Move in Settings (done)
- Story 4.2: Reset the phase lengths to 45/10/5

## Requirements & Constraints

- Defaults stay Sit 45, Stand 10, Move 5; a never-changed app behaves exactly as today. The third phase keeps the name `Move` (never `Walk`).
- Allowed values are 5, 10, ... 60 minutes per phase. A stored value that is missing, wrong-sized, out of range, or not a multiple of 5 falls back to that phase's default with no visible error; the other phases keep their valid values.
- Lengths live in their own persistent key, separate from the haptic key and the versioned session record; the record schema does not change.
- A changed length is read only when a phase starts: automatic transition, Skip, Start, and the full-length rebase on overdue recovery. A running phase keeps its deadline and Wakeup; a paused or timed-paused phase keeps its saved remaining time; nothing is rescheduled by a settings change.
- Timed-pause choices (5, 10, 30) and Extend amounts are unchanged. The tally counts naturally completed phases regardless of length; skipped and in-progress phases stay excluded.
- Settings window rows: the four haptic rows as today, then `Sit`, `Stand`, `Move`, then `Reset` (Story 4.2). Each length row shows its value as text (`Sit 45 min`). Selecting a length row adds 5 minutes, wraps 60 to 5, saves, updates the row, and keeps the window open with no preview. A failed save leaves the old value shown and stored. Haptic rows behave as before (save, one preview, return).
- Emulator evidence is recorded as emulator evidence, not physical-device evidence.

## Technical Decisions

- The Session Coordinator stays the only owner of timing; the UI only writes the setting and never computes deadlines.
- Lengths are one 3-byte record (Sit, Stand, Move) under key `0x5355` in `src/c/phase_settings.c`, validated per byte; one write replaces all three, so Reset cannot leave a half-reset state.
- One lookup, `prv_duration()` in the coordinator, feeds cycle advance, Skip, Start, and recovery, and now reads the configured length.
- Anything that assumes a 60-minute cycle must use the sum of the three configured lengths instead (the whole-cycle fast-forward in `prv_reconcile_phase`).
- The host test harness compiles the coordinator and stores against Pebble stubs; the UI file is not compiled there, and the test persist stub must learn the new key.

## UX & Interaction Patterns

- One native Settings window; no new surface. Long rows such as `Stand 60 min` must fit without clipping; the menu scrolls natively.
- `DESIGN.md` and `EXPERIENCE.md` get the length rows in the same change.

## Cross-Story Dependencies

- Story 4.2 builds on Story 4.1's persisted lengths and Settings rows (done). `sit_stand_move.c` is not compiled by the host harness.
- The PRD (section 7.2) and architecture spine (AD-6, "Default durations" convention) still say lengths are fixed; they are not yet amended for Epic 4.
