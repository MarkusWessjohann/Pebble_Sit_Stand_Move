---
title: 'Choose a haptic pattern in Settings'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '077b71564acb5d2f7f4e7fece48373c1dc549840'
context:
  - _bmad-output/implementation-artifacts/epic-3-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The watch always gives one short pulse. The owner wants to choose how reminders feel, and there is no Settings surface to do it.

**Approach:** Add a `Settings` row to the native menu that opens a native window with `Short`, `Long`, `Double`. The choice persists in its own storage key and is read at the coordinator's single haptic call site. `Until Select` is Story 3.2.

## Boundaries & Constraints

**Always:** Settings is the last menu row in every lifecycle state; existing rows keep order and labels. Default is Short and matches today's behavior. The current choice is marked in text. A haptic stays one attempt per committed transition, with the marker persisted before the vibration. Start, Pause, Resume, Stop, and Extend stay silent. A missing, corrupt, or unsupported stored value falls back to Short with no error. Text only; native styles.

**Never:** Change `SessionRecord`, its schema, `SESSION_STORE_KEY`, timing, scheduling, recovery, click handlers, or coordinator menu-row numbering. Add `Until Select`, repeats, a cue, or any other notification setting. Persist or log anything beyond the pattern. Claim emulator results as Pebble Time 2 results.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Default | No stored value; Skip | One short pulse | N/A |
| Chosen pattern | Long or Double saved; Skip, automatic transition, or timed-pause end | One long or double pulse; marker persisted first | N/A |
| Silent commands | Any pattern; Start, Pause, Resume, Stop, Extend | No vibration | N/A |
| Persistence | Pattern saved; Stop, Start, relaunch | Same pattern | N/A |
| Bad stored value | Wrong size or out-of-range byte | Short | No error shown |
| Save fails | Write fails on select | Stored choice and marker unchanged; no preview; window stays open | Marker not moving is the signal |
| Select a row | Row chosen in Settings | Saved, one preview of that pattern, back to the menu | N/A |
| Back | Back pressed in Settings | Setting unchanged | N/A |
| Menu rows | Inactive / active | Start, Settings / Pause or Resume, Skip, Stop, Settings | N/A |

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c:101` -- `prv_request_haptic()`, the only haptic call site (`vibes_short_pulse()`). Change it to read the pattern and vibrate it; keep the `HAPTIC_REQUEST` log prefix and add ` pattern=%d` at the end.
- `src/c/session_coordinator.c:873-906` -- menu rows 0, 1, 2 map to Start/Pause/Resume, Skip, Stop. Do not change; Settings is handled by the UI and never sent here.
- `src/c/session_store.c` -- shows the persist pattern and key `0x5353`. Do not change.
- `src/c/sit_stand_move.c:155-209` -- menu rows, draw, select, window load/unload; `:339-355` window create/destroy. Add the Settings row and window here.
- `tests/pebble-stubs/pebble.h`, `tests/session_coordinator_test.c:50-120` -- stubs only serve key `0x5353`, only `vibes_short_pulse`, which asserts the marker. Add the second key and the long/double stubs.
- `scripts/test.sh` -- compile line must include the new `.c`.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/haptic_settings.h`, `src/c/haptic_settings.c` -- new: `HapticPattern` {SHORT=0, LONG=1, DOUBLE=2}; `haptic_settings_get()` (own key `0x5354`, 1 byte, fallback Short); `haptic_settings_set()` returns false on write failure; `haptic_settings_vibrate(pattern)` -- one place that owns the setting
- [x] `src/c/session_coordinator.c` -- `prv_request_haptic()` uses get and vibrate; add the pattern to the log line -- every haptic request already flows through it
- [x] `src/c/sit_stand_move.c` -- Settings row (inactive 2 rows, active 4), Settings window with three rows marked `* `, select saves, previews, pops; Settings row never goes to the coordinator -- UI only
- [x] `tests/pebble-stubs/pebble.h`, `tests/session_coordinator_test.c`, `scripts/test.sh` -- stubs and tests for each I/O row that the host can exercise
- [x] `_bmad-output/planning-artifacts/ux-designs/ux-sit_stand_go-2026-10-23/DESIGN.md`, `EXPERIENCE.md` -- only if the built behavior differs from them; they already describe Settings (no change: the four-row list includes Until Select, which is Story 3.2)

**Acceptance Criteria:**
- Given any lifecycle state, when the menu opens, then Settings is last and the other rows are unchanged.
- Given no saved choice, when a haptic is requested, then it is one short pulse as today.
- Given Long or Double is saved, when Skip, an automatic transition, or a timed-pause end requests a haptic, then that pattern plays once.
- Given the change, when `sh scripts/test.sh` runs, then every existing test passes unchanged.

## Implementation Notes

- New `src/c/haptic_settings.{c,h}` owns the setting (key `0x5354`, one byte, Short fallback). Native `vibes_short/long/double_pulse` only.
- `prv_request_haptic()` reads the pattern on every request; log line ends ` pattern=%d`.
- `sit_stand_move.c`: Settings row last (inactive 2 rows, active 4), handled in the UI via `prv_is_settings_row`; the coordinator's row numbers are untouched. Settings window marks the current row `* `.
- Review-time change: the save-then-preview decision moved from the UI select handler into `haptic_settings_choose()` so the host tests cover the failed-save and preview matrix rows. The UI calls it and pops only on true.
- Host stubs: second persist key, long/double vibes, `fake_haptic_pattern`; the preview path skips the marker assertion because it has no session.
- Verified: `sh scripts/test.sh` passes (new haptic tests plus all existing tests unchanged). `sit_stand_move.c` was not built: no Pebble SDK in the sandbox. The Menu rows, Back, and window flow have no host test and need the emulator.

## Spec Change Log

## Review Triage Log

Pass 1 (Blind Hunter 8, Edge Case Hunter 8, Verification Gap 1). Verdicts below.

- medium (patch, applied) -- Settings window cursor starts on row 0, so Select pressed straight away overwrites a saved Long/Double with Short. Real: `prv_settings_window_load` set no selected index. Fixed by preselecting the stored row. (blind-hunter, edge-case-hunter)
- medium (defer) -- Menu row mapping is derived from live lifecycle at draw and again at select; a lifecycle change with the menu open (for example a wakeup ending a timed pause, or recovery) can shift row indices. Pre-existing: the old menu already mapped rows 0/1/2 from live lifecycle at select, and Story 3.1 only adds a constant-offset Settings row. Marked unverified: settling it needs an emulator run with a wakeup while the menu is open. (blind-hunter, edge-case-hunter)
- low (rejected) -- `prv_is_settings_row` calls `session_coordinator_get_view` per row, which can enqueue clock-check and rollover events. Pre-existing pattern: `prv_menu_num_rows` and `prv_menu_draw_row` already did this. Guards in `get_view` (`s_clock_check_queued`, `s_rollover_requested_day`) bound the extra events to one each. Fix needs plumbing for negligible harm. (edge-case-hunter)
- low (rejected) -- Failed save gives no feedback beyond the unmoved `*`. This is the specified behavior in the frozen I/O matrix ("Marker not moving is the signal"). A write failure on one byte is rare; adding error UI is new surface. (blind-hunter, edge-case-hunter)
- false -- Range check `pattern < HAPTIC_PATTERN_SHORT` can fail the build (`-Wtype-limits`). Compiled `haptic_settings.c` with `-Wall -Wextra -Wtype-limits -Werror` on the host: clean. The SDK's own flags cannot be checked here. (blind-hunter, edge-case-hunter)
- low (rejected) -- Read error and missing value are both treated as Short, with no log. Specified: a missing, corrupt, or unsupported value falls back to Short with no error. (edge-case-hunter, blind-hunter)
- low (rejected) -- Persistence key defined twice (`HAPTIC_SETTINGS_KEY`, test `HAPTIC_KEY`), no version byte. The test mirrors the session-store test pattern (`STORE_KEY`); a one-byte enum needs no schema. (blind-hunter)
- low (rejected) -- `haptic_settings_get` does two persist reads per row draw, and now on the alert path. Three rows, one byte each; no measurable harm and caching would add a second source of truth, which AD-14 forbids. (blind-hunter)
- low (rejected) -- `PATTERN_COUNT` is a magic 3 not tied to the enum, so Story 3.2's fourth pattern could desync titles. Story 3.2 edits this exact table; a sentinel here is speculative for this story. (blind-hunter)
- low (rejected) -- Preview/pop returns to the menu window, not the session view. Matches the specified behavior ("return to the previous screen"). (blind-hunter)
- low (rejected) -- Preview test covers only Double, not Short/Long previews; `s_vibe_is_preview` flag could stay set after a failed assert. The mutation run killed "long plays double" via the request path; a failed assert aborts the whole test process, so a stale flag cannot mask anything. (blind-hunter, edge-case-hunter)
- false -- Claim "Never change click handlers" is violated because `prv_menu_select` changed. That is the menu row handler the spec requires (Settings row routing); the hardware button handlers (`prv_select/up/down/menu_click_handler`) are unchanged per the diff. The existing `vibes_short_pulse` stub body was reshaped, but every existing test still passes unchanged. (edge-case-hunter)
- low (rejected) -- Matrix test audit: `read` condition `persist_read_data(...) != sizeof` never reached by tests. Unreachable through the fake, which fails the size check first; a real SDK failure on a size-matched read is not demonstrated. (verification-gap, other findings)
- medium (defer) -- Settings row routing, window flow, and Settings-row/enum mapping in `sit_stand_move.c` have no automated test; the file is not compiled by `scripts/test.sh` and the sandbox has no Pebble SDK. Demonstrated mutants (change the `3`/`1` row constant, drop the early `return`, reorder the titles, pop on failure) all keep the host tests green. Pre-existing gap, already recorded for Stories 1.6, 1.7 and 2.1; the spec routes it to an emulator check. (verification-gap, blind-hunter, edge-case-hunter)

## Design Notes

Reading the setting at each request, not caching it, avoids stale state and a second source of truth (AD-14). The preview uses `haptic_settings_vibrate` directly, not the coordinator, so it neither touches the haptic marker nor needs a session.

Patterns use the native Pebble calls `vibes_short_pulse()`, `vibes_long_pulse()`, and `vibes_double_pulse()`; no custom `VibePattern`, so the feel is the platform's. These were chosen without asking because the owner named the patterns, not their timings.

`sit_stand_move.c` is not compiled by the host tests and the sandbox has no Pebble SDK, so the UI cannot be built here.

## Verification

**Commands:**
- `sh scripts/test.sh` -- expected: passes, including the new haptic setting tests
- `uv run --locked python scripts/pebble.py install --emulator emery` -- expected: installs (not runnable in this sandbox)

**Manual checks (if no CLI):**
- On the emery emulator: open the menu inactive and active and confirm Settings is last; choose each pattern and confirm the `*` moves and one preview plays; Skip and confirm the logged `pattern=`; relaunch and confirm the choice stays. Record as emulator evidence; Pebble Time 2 feel stays an owner check.
