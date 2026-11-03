---
title: 'Enlarge the time and tally, and place button labels beside the buttons'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '38c1aa1973d8fc5a19dc8a9a36fe799121fd90c2'
context:
  - _bmad-output/implementation-artifacts/epic-2-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The session view's clock and daily tally are Gothic 14 and hard to read. The button hints are one centered line (`TOP +5 BOTTOM +10`) that does not sit beside the buttons it describes.

**Approach:** Render the clock and tally in Gothic 24. Replace the centered controls line with three labels in a column at the right screen edge, each level with its hardware button. Narrow the content column so nothing overlaps. Presentation only: coordinator, timing, persistence, and button functions are unchanged.

## Boundaries & Constraints

**Always:** Use native Gothic fonts only. Keep the countdown the strongest element and the order time, countdown, icon, phase name, tally. Keep the phase name literal (`Sit`, `Stand`, `Move`); text stays authoritative there. Keep the error line visible and unclipped in every state. Render coordinator state only; the view owns no timers. Keep label logic in step with the coordinator's timed-pause mapping (shorter top, longer bottom).

**Never:** Change `session_coordinator.*`, `session_store.*`, click handlers, menu rows, or tests of coordinator behavior. Add custom or scaled fonts, new surfaces, settings, or colors. Show `+5`/`+10` outside `running`. Claim Pebble Time 2 legibility verified from emulator or host output.

**Decision (owner):** The tally is one compact Gothic 24 row with short labels, format `S3 T2 M1` (S = Sit, T = Stand, M = Move; counts follow the letter). This overrides the literal-label wording in UX-DR15 and Story 2.1's tally criterion (`Sit`, `Stand`, `Move` with counts); the planning docs are not yet updated to match.

**Decision (owner):** While `running`, the middle label reads `II`, an ASCII stand-in for the `Ⅱ` glyph in `DESIGN.md`.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Running | `running` | Top `+5`, middle `II`, bottom `+10`, right edge, level with buttons | N/A |
| Timed break | `timed_paused`, pause 5 / 10 / 30 min | Top shorter, middle `BREAK`, bottom longer: 10/30, 5/30, 5/10 | N/A |
| Manual pause | `paused` | Middle `PAUSED` only; frozen phase and remaining time visible | N/A |
| Inactive | `inactive` | No button labels; `--:--` and `Ready` as today | N/A |
| Tally | Counts 3 / 2 / 1; and 12 / 10 / 3 | `S3 T2 M1`; `S12 T10 M3`; one row, unclipped, in every state | N/A |
| Error shown | Any state plus a coordinator error | Error text visible, no overlap with tally or labels | Visible text |

</frozen-after-approval>

## Code Map

- `src/c/sit_stand_move.c` -- sole file to change; not compiled by `scripts/test.sh`.
  - `prv_render` (`:78`): sets time (`:84`), countdown, phase text, `s_controls_text` (`:97` timed break, `:101-104` running, `:115` inactive) and tally (`:124`). The label text moves out of the single controls line into three per-button labels.
  - `prv_window_load` (`:238-277`): fixed layers: time `y0 h22`, countdown `y22 h52` (Bitham 30), icon `y74 h27`, phase `y99 h28`, controls `y127 h16`, tally `bounds.h-32 h14`, error `bounds.h-18 h18`. Widths use `bounds.size.w`; the content column must use a narrower width, and `prv_draw_icon` centers on its layer width.
  - `prv_window_unload` (`:279`) and the `static` layer/text buffers at the top (`:7-23`): add and release any new layers; `s_tally_text` is `char[48]`, ample for the compact tally.
  - Timed-break mapping (`:95-96`): reuse `shorter` / `longer`; do not reimplement it differently.
- Do not touch: `session_coordinator.c` (`session_coordinator_top_control` / `_bottom_control` / `_middle_control` define what each button does), click handlers (`:197-236`), menu code (`:141-195`).
- `scripts/test.sh` -- must still pass unchanged. `README.md` lists the build and emulator commands.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/sit_stand_move.c` -- Gothic 24 for the clock and tally; tally as one compact row `S{n} T{n} M{n}`; running middle label `II`.
- [x] `src/c/sit_stand_move.c` -- replace the controls line with three right-edge labels level with the buttons, per the I/O matrix; narrow the content column; resize the icon/phase layers.
- [x] `src/c/sit_stand_move.c` -- keep the error line visible in every state; release all new layers on unload.

**Acceptance Criteria:**
- Given any state, when rendered, then clock and tally use Gothic 24 and nothing overlaps or clips (no label, countdown, icon, phase name, tally, or error).
- Given `running`, `timed_paused`, `paused`, or `inactive`, when rendered, then the right-edge labels match the I/O matrix.
- Given the change, when `sh scripts/test.sh` runs, then it passes unchanged and no coordinator, store, click-handler, or menu code differs.

## Implementation Notes

- Implemented in `src/c/sit_stand_move.c` only. Clock and tally use Gothic 24; the tally is `S%u T%u M%u`. The single controls layer is replaced by `s_label_layers[3]` (Gothic 18 Bold, right-aligned, 78 px column at the right edge, centered at 1/4, 1/2, 3/4 of the screen height). Clock, countdown, icon and phase use a content column of `width - 78`. The tally spans the full width above the 18 px error line. New layers are destroyed on unload.
- Timed-break mapping reuses the existing `shorter`/`longer` values. Review patch: top/bottom labels print with the unit (`10m`, `30m`); the first pass had dropped it.
- Behavior change: while manually paused the phase text is now the plain phase name in Gothic 24 (was `PAUSED: Sit` in Gothic 18); `PAUSED` is the middle label.
- Matrix test audit: the I/O rows describe rendering in `sit_stand_move.c`, which `scripts/test.sh` does not compile and the host stubs cannot exercise (no TextLayer stubs). No test covers them. The spec's verification for these rows is the manual emulator checks, which have not run here. Under the audit rule these rows count as uncovered until someone runs them.
- Unverified estimates: the 1/4, 1/2, 3/4 button positions; the 78 px column and 40 px countdown height on emery; clock height 26 for Gothic 24. `package.json` also targets 144x168 platforms (aplite, basalt, diorite, flint) and round chalk; the narrowed column and full-width tally will not fit there. The spec targets Pebble Time 2.
- `sh scripts/test.sh` passes unchanged (coordinator tests and 4 Python tests). `sit_stand_move.c` was not built; no Pebble SDK in the sandbox.

- Owner layout changes after review (2026-11-03, direct edits, outside the frozen block): the countdown, icon, and phase name are one block centered on the full screen in both directions (`block_y = (h - 95) / 2`); the clock is centered on the full width; the label column sits 5 px further from the right edge (`LABEL_RIGHT_MARGIN`); label backgrounds are `GColorClear` because the full-width countdown layer overlaps the label layers. The narrowed `content_w` was removed. Not built or seen on a display. By arithmetic on 200x228 the middle label (y 103 to 125) and the countdown layer (y 66 to 106) overlap by about 3 px of height; their text is not expected to collide (digits end near x=150, `PAUSED` starts near x=135), unverified.

## Spec Change Log

## Review Triage Log

- medium (patch) — Timed-break top/bottom labels printed bare `10` / `30`; the previous line printed `10m` / `30m`, so the unit was lost on the only text that says what those buttons do. Fixed: labels now print `%dm`. (blind-hunter)
- low (rejected) — Paused state has no resume affordance and labels may disagree with handlers. The matrix and DESIGN.md specify `PAUSED` alone; `session_coordinator_top_control` / `_bottom_control` return false in `paused` (menu opens) and `_middle_control` resumes, so labels match handlers. (blind-hunter)
- false — `S%u T%u M%u` is cryptic. It is the owner's recorded decision in the frozen block, chosen over literal labels. (blind-hunter)
- low (rejected) — Repeated clear loop, magic `3`, no null checks, no static size guard on `s_label_text`. Matches existing style; longest label `PAUSED` needs 7 of 8 bytes; a helper or constant adds code for no named harm. (blind-hunter)
- false — Timed-break labels diverge from the coordinator for odd durations. `session_store_validate` accepts only 5/10/30 for `timed_paused`, and for those the view's 10/30, 5/30, 5/10 equals `prv_alternative_duration`; the mapping lines are unchanged from before. (edge-case-hunter)
- low (rejected) — Tally with three-digit counts wraps. A counter is one day's naturally completed phases (about one per 5 to 45 minutes); `S100 T100 M100` is about 14 characters, within the 200 px width. (edge-case-hunter)
- low (rejected) — 26 px clock layer may clip Gothic 24. Cannot tell from the diff; if true the harm is cosmetic. Covered by the emulator check. (blind-hunter, edge-case-hunter)
- maybe-false — Countdown wraps with three-digit minutes after many extends. By estimate `MMM:SS` is about 114 px against a 122 px column; settle with an emulator screenshot of a 100:00 countdown. Unverified medium. (blind-hunter, edge-case-hunter)
- maybe-false — Label 3 (160 to 182 px) touches the tally (starts at 182 px) on 228 px height with zero margin. Settle on the emery emulator. Unverified medium. (edge-case-hunter, blind-hunter)
- medium (defer) — Narrowed content column, 3/4-height label and full-width tally overlap on the 144x168 (aplite, basalt, diorite, flint) and round chalk targets still built from `package.json`. Intent targets Pebble Time 2 only (NFR1, spec Design Notes), so not fixed here. (edge-case-hunter, blind-hunter, verification-gap)
- low (defer) — `prv_render` label, phase and tally text are covered by no test: `sit_stand_move.c` is not compiled by `scripts/test.sh`. Mutants (swap `shorter`/`longer`, change `II`) keep every host test green. Same gap as Stories 1.6 and 1.7. (verification-gap)
- medium (defer) — Layout, overlap and the 1/4, 1/2, 3/4 button alignment have no recorded check. Needs emulator or Pebble Time 2. (verification-gap, blind-hunter)

## Design Notes

Pebble hardware buttons sit on the right edge at roughly the top, middle, and lower thirds of the case; the label column is placed to match and must be confirmed on a Pebble Time 2 or the emulator, not assumed. `PAUSED` and `BREAK` need a wider column than `+5`/`+10`; size the column for the widest label.

## Verification

**Commands:**
- `sh scripts/test.sh` -- expected: passes (the sandbox cannot build `sit_stand_move.c`; no Pebble SDK is installed here).
- `uv run --locked python scripts/pebble.py build` -- expected: builds; run on the host where the pinned SDK 4.33.1 is active.
- `uv run --locked python scripts/pebble.py install --emulator emery` -- expected: installs; inspect the states below.

**Manual checks (if no CLI):**
- On the emery emulator, screenshot inactive, running, timed break (5, 10, 30), manual pause, a two-digit tally, and an error state. Record emulator evidence as emulator evidence; Pebble Time 2 legibility stays an owner check.
