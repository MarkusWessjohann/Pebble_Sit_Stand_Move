# Epic 3 Context: Choose how the watch notifies me

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

The owner picks the haptic pattern in a Settings entry of the native watch menu: Short, Long, Double, or Until Select. Every haptic request (automatic phase transition, Skip, timed-pause end) uses the chosen pattern, and Until Select repeats until Select is pressed. The choice survives restarts and Stop. Session timing, scheduling, recovery, and the Epic 1 and 2 behavior are unchanged apart from the haptic call. Builds on Epics 1 and 2; does not reopen the waived physical-device story.

## Stories

- Story 3.1: Choose a haptic pattern in Settings
- Story 3.2: Repeat the alert until Select is pressed

## Requirements & Constraints

- Settings is the last native menu row in every lifecycle state (inactive, running, paused, timed-paused); existing rows keep their order and labels.
- The Settings window lists Short, Long, Double, Until Select in that order, marks the current choice in text (not color), saves on select with one preview, and returns to the previous screen. Back leaves the setting unchanged.
- Default is Short, identical to today's single short pulse. A missing, corrupt, or unsupported stored value falls back to Short with no visible error.
- The pattern applies to every haptic request. Start, manual Pause, Resume, Stop, and Extend stay silent whatever the pattern.
- A haptic is still one attempt per committed transition, with the haptic-attempt marker persisted before the call and never replayed after recovery.
- Until Select: repeats until Select, a 2-minute cap, Stop, or supersession by a newer transition. The Select press that dismisses it is consumed and enqueues no session event. Volatile, never resumed after relaunch. Shows the text cue `Select to stop` in place of the clock line.
- Haptic is the only notification channel; no other notification setting, alternate channel, or fallback.
- Text is authoritative; never rely on color. Native Pebble styles only; no custom fonts or decoration.
- Emulator evidence is recorded as emulator evidence, not physical-device evidence. Pebble Time 2 feel of each pattern is an owner check.

## Technical Decisions

- The pattern lives in its own persistent key, outside the versioned session record; the session record schema does not change.
- The Session Coordinator's single haptic call site reads the pattern; every haptic request goes through it.
- The UI renders coordinator state and dispatches commands; it must not own session timing.
- Select is also the middle control. While an alert repeats, the dismissal is decided before the middle-control dispatch.
- The host test harness compiles the coordinator and store with Pebble stubs; the UI file is not compiled there.

## UX & Interaction Patterns

- Menu order: Start, Settings when inactive; Pause/Resume, Skip, Stop, Settings when active.
- Settings is a native menu window; selecting a row previews once and pops back.
- While an Until Select alert repeats, `Select to stop` replaces the clock at the top of the session view and nothing else moves.

## Cross-Story Dependencies

- Story 3.2 builds on Story 3.1's persisted setting, Settings window, and haptic call site. Story 3.1 stands alone with three patterns.
- `DESIGN.md`, `EXPERIENCE.md`, the PRD, and the architecture spine (AD-14, AD-18) were updated ahead of implementation and already describe this epic.
