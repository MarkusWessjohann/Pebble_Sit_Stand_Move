# Epic 2 Context: Make the session view easier to read at a glance

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

The owner can read the current time and the daily tally more easily, and can see what each right-side hardware button does because its label sits next to that button. This is a presentation-only change to the single foreground session view on Pebble Time 2. Session behavior, timing, the coordinator, and persistence are unchanged. It builds on Epic 1's finished session view and does not reopen the waived physical-device acceptance story.

## Stories

- Story 2.1: Enlarge the time and tally, and place button labels beside the buttons

## Requirements & Constraints

- Current wall-clock time and the daily Sit/Stand/Move tally use the native Gothic 24 text style (the nearest native size to a requested 50% increase over Gothic 14). No custom or scaled font rendering.
- The enlarged tally must fit the display-safe area without clipping or truncation in every lifecycle state (inactive, running, `PAUSED`, `BREAK`, with and without an error). Layout may stack or compact it; labels stay literal and counts stay textually identifiable.
- Button labels sit at the right screen edge, level with their physical button, replacing the single centered `TOP … BOTTOM …` line. Top: `+5` running, shorter alternative duration while timed-paused. Middle: pause control, with `BREAK` timed-paused and `PAUSED` manually paused. Bottom: `+10` running, longer alternative while timed-paused. No labels when inactive; no `+5`/`+10` while manually paused.
- The countdown stays the strongest element; the phase icon and name stay directly below it. The content column narrows so it never overlaps the label column.
- Hardware button functions, menu behavior, coordinator logic, and persistence are unchanged. Existing tests keep passing.
- Text is authoritative; color and icons must never be the sole identifier of phase or state. Use `Move`, never `Walk`.
- Native Pebble styles only: no invented palette values, fonts, shadows, or decoration.
- Emulator evidence is recorded as emulator evidence; it is not physical-device evidence.

## Technical Decisions

- The UI only renders coordinator state and dispatches commands. It must not own or advance timers or resolve session timing.
- Target display is Pebble Time 2; exact metrics are platform-defined and must be verified rather than assumed.
- The native watch menu and its row order are unchanged.

## UX & Interaction Patterns

- Timed-pause alternatives: shorter on top, longer on bottom. For 5 active: top 10 / bottom 30. For 10 active: top 5 / bottom 30. For 30 active: top 5 / bottom 10.
- `BREAK` (timed) and `PAUSED` (manual) must stay distinguishable by text alone.
- Session view order, top to bottom: time, countdown, phase icon, phase name, tally.

## Cross-Story Dependencies

- Depends only on Epic 1's session view. The design docs (`DESIGN.md`, `EXPERIENCE.md`) were already updated to match this layout.
