---
title: Sit Stand Go — Watch Design
name: Sit Stand Go
description: Native Pebble Time 2 interface for a personal sit/stand/move reminder.
status: final
created: 2026-10-23
updated: 2026-11-03
sources:
  - ../../briefs/brief-sit_stand_go-2026-10-21/brief.md
  - ../../prds/prd-sit_stand_go-2026-10-21/prd.md
  - ../../architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md
typography:
  primary:
    note: Pebble native readable text style; platform-defined metrics
  secondary:
    note: Pebble native Gothic 24 text style for the clock and day tally (Epic 2, Story 2.1; was Gothic 14); platform-defined metrics
  timer:
    note: Pebble native large/readable text style; platform-defined metrics
---

# Sit Stand Go — Design Spine

## Brand & Style

Sit Stand Go is a watch-first, single-device reminder utility. Its visual language stays within native Pebble conventions: legible, information-led, and usable with a monochrome/high-contrast display. Semantic tokens inherit values from the Pebble platform; this document does not prescribe palette values, fonts, decorative treatments, or a custom brand layer.

The foreground session view gives visual priority to the phase countdown. The current time and today's completed phase counts remain secondary to it, though larger than originally specified (Gothic 24). Use `Move` consistently as the phase name.

## Colors

Use the native Pebble display palette and system contrast conventions. Concrete values are platform-defined and require target-device verification. Do not assign colors to phase identity or rely on color alone to communicate state.

## Typography

Use Pebble native text styles: `typography.primary`, `typography.secondary`, and `typography.timer`. Hierarchy is relative: the remaining phase time is most prominent; current time and day tally use `typography.secondary` (Gothic 24); phase identity remains readable. Exact typeface, sizes, weights, and truncation behavior are platform-defined.

## Layout & Spacing

Single Pebble Time 2 watch form factor. Use the native display-safe area and spacing; exact dimensions are platform-defined. On the foreground session screen, place content in this vertical order:

1. Current wall-clock time, secondary size (Gothic 24), at the top, centered on the screen width.
2. Remaining time for the active phase, large and primary.
3. Phase icon directly below the remaining time.
4. Current phase name (`Sit`, `Stand`, or `Move`) paired directly with its icon below the countdown. Items 2 to 4 form one block centered on the screen in both directions.
5. Bottom day tally of completed counts in one compact row, secondary size (Gothic 24), written `S3 T2 M1` (S = Sit, T = Stand, M = Move), so it fits without clipping.

The top, middle, and bottom physical buttons are the session's right-side controls. Each control's label sits in a column at the right screen edge, vertically level with its physical button and 5 px in from the edge. The centered block may extend under the label column's width, so label backgrounds are transparent and the countdown text must not collide with a label. The watch menu contains session-level commands. Do not introduce additional surfaces beyond the session view and native watch menu without a confirmed need.

Verify the finished app's layout, controls, legibility, and firmware-dependent behavior on Pebble Time 2 in Story 1.8 before release.

## Elevation & Depth

No custom elevation, shadow, or layered depth is specified. Use native Pebble surface treatment only.

## Shapes

Use native Pebble system geometry where applicable. No custom corner radii or ornamental shapes are specified.

## Components

| Component | Placement | Visual specification |
|---|---|---|
| Session view | Foreground app | One watch surface following the content order above. |
| Session time | Top of session view | Native Gothic 24 secondary text. |
| Phase remaining | Center/primary region of session view | Largest relevant native text. |
| Phase icon and name | Together below remaining time | Monochrome chair icon for `Sit`, standing-person icon for `Stand`, or moving-person icon for `Move`, paired with the readable phase label. Text remains authoritative; use suitable native assets and verify on target hardware. |
| Day tally | Bottom of session view | Native Gothic 24 secondary text in one compact row, `S3 T2 M1`, showing completed Sit / Stand / Move totals for today. |
| Top hardware button | Right screen edge, level with the top right-side physical button | Label shows `+5` while running and its current duration choice during a timed break. |
| Middle hardware button | Right screen edge, level with the middle right-side physical button | Label shows `II` while running (an ASCII stand-in for `Ⅱ`); its state label is `BREAK` during a timed break and `PAUSED` during indefinite manual pause. |
| Bottom hardware button | Right screen edge, level with the bottom right-side physical button | Label shows `+10` while running and its current duration choice during a timed break. |
| Watch menu | Native watch menu | Use native Pebble menu-row presentation. Show Start when inactive; while active, list Pause/Resume, Skip, then Stop. |
| Status message | Session view, when needed | Readable native text reports invalid clock, persistence, or wakeup/scheduling failures. Communicate failure without relying on color. |

When the session is indefinitely paused from the menu, show `PAUSED` with the frozen phase and remaining phase time. For a timed break, show `BREAK` with remaining break time. Keep these text states distinguishable without color.

## Do's and Don'ts

| Do | Don't |
|---|---|
| Inherit Pebble native styles and semantic system tokens. | Invent palette values, font families, gradients, shadows, or custom visual decoration. |
| Keep remaining phase time primary and use the agreed top-to-bottom information order. | Give the day tally or current clock more prominence than the active countdown (they are Gothic 24; the countdown stays larger). |
| Pair the phase icon directly with readable phase identity; retain `Move`. | Use color or icon alone to distinguish a phase. |
| Keep `+5`, pause/resume, and `+10` mapped to the top, middle, and bottom right-side buttons. | Reassign button functions or imply unverified hardware foregrounding/haptic behavior. |
| Use native monochrome/high-contrast-friendly presentation. | Rely on unverified palette specifics or platform behavior. |
| Verify countdown, phase name, state labels, daily tally, and errors are legible on target Pebble Time 2 hardware. | Treat generic contrast guidance as proof that the composed screen fits or is readable on the target display. |
