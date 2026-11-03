---
title: 'Repeat the alert until Select is pressed'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '8ba2c249d4ca2cae2a4bb87b6343502c9ca49f3b'
context:
  - _bmad-output/implementation-artifacts/epic-3-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** A single pulse is easy to miss when the watch is off the wrist or the owner is busy. Story 3.1 added the pattern setting but no pattern that keeps going.

**Approach:** Add a fourth Settings row, `Until Select`. When chosen, a haptic request repeats one committed attempt at a fixed interval until Select, a 2-minute cap, Stop, or supersession, with `Select to stop` replacing the clock on the session view. The dismiss press is handled before the middle-control dispatch.

## Boundaries & Constraints

**Always:** The marker is persisted once, before the first vibration; repeats never write and never retry (AD-18). The alert is volatile: never persisted, never resumed after relaunch or recovery; recovery starts at most one new alert. Only one alert runs at a time; a newer haptic request replaces it. Up, Down, long-press Select, and menu commands keep their normal functions. With no alert repeating, Select is unchanged. Short, Long, and Double behave exactly as in Story 3.1. Text only; native styles.

**Decision (owner, 2026-11-03):** Only Select, the 2-minute cap, Stop, and supersession end an alert. Menu Pause, timed pause, Resume, and Extend do not; a paused session keeps vibrating until one of those four ends it.

**Decision (owner, 2026-11-03):** Select dismisses the alert on the session view only. While the menu or Settings window is on top, Select keeps its normal row-pick function and the alert continues until the owner returns to the session view, or until the cap.

**Never:** Change `SessionRecord`, the session store, timing, scheduling, recovery, or menu-row numbering. Persist alert state. Add any other setting or notification channel. Claim emulator results as Pebble Time 2 results.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Settings row | Settings window open | Four rows: Short, Long, Double, Until Select; current marked `* ` | N/A |
| Choose it | Until Select selected | Saved, one short preview (no repeat), back | Failed save: no preview, window stays |
| Alert starts | Until Select; Skip, automatic transition, or timed-pause end | Marker persisted, then first vibration, then repeats every 2 s | N/A |
| Dismiss | Alert repeating; Select on session view | Vibration and cue stop; no session event enqueued; no state change | N/A |
| Other buttons | Alert repeating; Up, Down, long-press Select | Alert continues; button works as before | N/A |
| Cap | 2 minutes after start | Alert stops by itself; cue gone | N/A |
| Stop | Alert repeating; Stop | Alert and cue stop | N/A |
| Supersede | Alert repeating; Skip | One alert, following the newer transition | N/A |
| Relaunch | Alert repeating; app restarts | No alert after relaunch; marker not replayed | N/A |
| Recovery | Overdue deadline on launch; Until Select | At most one new alert for the reconciled transition | N/A |
| Pause while alerting | Alert repeating; menu Pause or timed pause | Session pauses; alert continues until Select, cap, Stop, or supersession | N/A |
| Menu on top | Alert repeating; menu or Settings window open; Select | Picks the highlighted row; alert continues | N/A |
| Idle Select | No alert; Select | Middle control if active, menu if inactive | N/A |
| Other patterns | Short, Long, Double | No repeat, no cue | N/A |

</frozen-after-approval>

## Code Map

- `src/c/haptic_settings.{h,c}` -- add `HAPTIC_PATTERN_UNTIL_SELECT = 3` and a `HAPTIC_PATTERN_COUNT` sentinel; `get()` accepts 0..3; `vibrate(UNTIL_SELECT)` plays one short pulse (the preview).
- `src/c/haptic_alert.{h,c}` -- new, volatile: one `AppTimer`, 2000 ms interval, 60 ticks (2 min) cap; `start`, `stop`, `active`, `dismiss` (true if it was repeating). Calls `vibes_cancel()` on stop.
- `src/c/session_coordinator.c:101` -- `prv_request_haptic()`: for Until Select, after the first vibration call `haptic_alert_start()` (restarts if running). `:515` `prv_stop()`: stop the alert once the Stop commits. `:1004`/`:1025` init/deinit: stop it. `:1052` `session_coordinator_middle_control()`: first line `if (haptic_alert_dismiss()) return true;`, so a dismiss press enqueues nothing and the UI's existing re-render runs.
- `src/c/sit_stand_move.c:87` `prv_render()` -- clock line shows `Select to stop` while `haptic_alert_active()`; `:~232` settings titles and `PATTERN_COUNT` become four rows from the sentinel. The select click handler needs no change.
- `tests/pebble-stubs/pebble.h`, `tests/session_coordinator_test.c:180` -- the stub has one timer slot and `fake_run_timers` loops while it is pending, so a repeating timer would hang it. Route `app_timer_register` with `timeout_ms >= 1000` to a separate alert slot driven by a new `fake_alert_tick()`; add `vibes_cancel`; repeat vibrations skip the marker assertion (the marker is checked on the first).
- `scripts/test.sh` -- add `haptic_alert.c` to the compile line.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/haptic_settings.{h,c}` -- fourth pattern, sentinel, range -- one place owns the pattern set
- [x] `src/c/haptic_alert.{h,c}` -- repeat, cap, stop, dismiss -- volatile state owned in one module
- [x] `src/c/session_coordinator.c` -- start at the haptic site, stop on Stop and init/deinit, dismiss first in `middle_control` -- AD-18
- [x] `src/c/sit_stand_move.c` -- fourth Settings row and the cue -- UI only
- [x] `tests/pebble-stubs/pebble.h`, `tests/session_coordinator_test.c`, `scripts/test.sh` -- stub routing and a test per matrix row the host can run, including that a dismiss press changes no lifecycle, generation, or write count

**Acceptance Criteria:**
- Given Until Select, when a transition requests a haptic, then the marker is persisted before the first vibration and later ticks write nothing.
- Given an alert repeating, when Select is pressed, then it stops and no session state changes.
- Given the existing suite, when `sh scripts/test.sh` runs, then every earlier test passes unchanged.

## Implementation Notes

- New `src/c/haptic_alert.{c,h}`: volatile, one `AppTimer`, 2000 ms interval, 60 ticks (the 2-minute cap, counted without reading the clock). `stop` calls `vibes_cancel()`. If a repeat cannot arm its timer the alert marks itself inactive, so a cue never shows with nothing to end it (small addition beyond the spec).
- `haptic_settings`: `HAPTIC_PATTERN_UNTIL_SELECT = 3` plus a `HAPTIC_PATTERN_COUNT` sentinel; reads and writes accept `< COUNT`. Until Select's single vibration (and its Settings preview) is a short pulse.
- `prv_request_haptic()`: stops any running alert, vibrates, then starts the alert for Until Select. The marker is persisted earlier, as before, so repeats write nothing.
- Alert ends: `prv_stop()` after the Stop commits (a failed Stop leaves it running), `init`/`deinit`, supersession, cap, and dismiss. `session_coordinator_middle_control()` begins with `haptic_alert_dismiss()`, so a dismiss press enqueues nothing.
- `sit_stand_move.c`: fourth Settings row (count from the sentinel) and the cue in place of the clock. The menu and Settings windows never call `middle_control`, so Select there keeps its row-pick function (owner decision 2A).
- Tests: alert timers (`timeout_ms >= 1000`) use their own stub slot driven by `fake_alert_tick()`; `fake_run_timers` ignores it, so it cannot hang. Two existing assertions used `3` as the "unsupported value" (now valid); they use `HAPTIC_PATTERN_COUNT`. No other earlier test changed.
- Mutation audit on the host tests: 16 mutants, 15 killed after two test additions (repeat pulse type, init-without-deinit). One equivalent survivor: removing `s_ticks = 0` in `haptic_alert_start`, because `haptic_alert_stop` already zeroes it on every path in.
- Verified: `sh scripts/test.sh` passes. `sit_stand_move.c` was not built (no Pebble SDK): the fourth row, the cue, and its width (about 14 characters at Gothic 24) need the emulator. The cue relies on the existing 1 s refresh to appear and clear, and the Select handler's re-render after a dismiss.
- Emulator evidence (owner self-check, 2026-11-03, source: owner report; emulator, not Pebble Time 2): (4) pressing Select while an Until Select alert repeats stops it and does not start a 30-minute timed pause. The owner reported no problems. Not yet checked: the `Select to stop` cue (fit in the clock line, appearing and clearing), the 2-minute cap, Stop ending the alert, Skip superseding a running alert, the clock returning afterwards, and anything on Pebble Time 2.
- Emulator evidence (owner self-check, 2026-11-03, source: owner report; emulator, not Pebble Time 2): the `Select to stop` cue is displayed while an Until Select alert repeats. The owner did not report whether the text fits the clock line. Owner closed this story as done on 2026-11-03 with these still unchecked: cue fit and clearing, the 2-minute cap, Stop ending the alert, Skip superseding a running alert, the clock returning afterwards, and anything on Pebble Time 2. Host tests cover the cap, Stop, and supersession; nothing but the emulator covers the cue's fit.

## Spec Change Log

## Review Triage Log

Pass 1 (Blind Hunter 7, Edge Case Hunter 5, Verification Gap 3 plus 1 other). The first launch of all three failed with service errors and returned no findings; the second launch is what is logged here.

- medium (patch, applied) -- Nothing pinned the alert interval: any registered timeout of 1000 ms or more reached the alert slot, so changing `HAPTIC_ALERT_INTERVAL_MS` to 10000 kept every test green. Real. The fake now records the timeout and the marker test asserts 2000 after start and after a tick; mutants at 10000 and 1000 are both killed. (verification-gap)
- medium (patch, applied) -- The `prv_arm` NULL branch never ran, so deleting `s_active = false;` kept every test green and a stuck "alert" would swallow the next Select as a dismiss. Real. Added `fake_fail_alert_timer` and a test for a failed first arm and a failed re-arm; the mutant is killed. (verification-gap, blind-hunter)
- low (patch, applied) -- The comment `60 * 2 s = the 2-minute cap` was imprecise: first pulse, 59 repeats, last pulse at 118 s, tick 60 only stops. Reworded, no code change. (blind-hunter)
- low (rejected) -- Edge: `prv_recover` can leave an alert running if a mid-run reload gives an inactive, missing, or unreadable record. Not reachable: `prv_recover` runs mid-run only at the two call sites that have just loaded a committed record successfully, the alert only starts from a committed running or paused transition, and Stop already ends it. Adding a guard would protect a state not demonstrated. (edge-case-hunter)
- low (rejected) -- "Select to stop" can stay up to one second after the cap or a failed arm ends the alert. Real but bounded: `prv_refresh` re-renders every 1000 ms, and the Select handler re-renders after a dismiss. Cosmetic; a fix needs a stop callback or a new timer. (blind-hunter, edge-case-hunter)
- false -- Claim that the cue stays "until some unrelated redraw". Refuted by the 1 s refresh above. (blind-hunter)
- false -- Claim that the cue hides the countdown in every lifecycle. The cue replaces `s_time_text`, which feeds the clock layer `s_time_layer`; the countdown is `s_countdown_layer`. (blind-hunter)
- low (rejected) -- The alert cannot be dismissed from the menu or Settings window. Excluded by the intent itself: frozen owner decision 2A (Select dismisses on the session view only). (blind-hunter)
- low (rejected) -- A mid-run failed re-arm ends the alert without `vibes_cancel`. The pulse that just played is a short pulse that ends by itself, so cancelling would only cut it short; ending silently is the safe failure. (edge-case-hunter)
- low (rejected) -- The fake routes timers by a 1000 ms threshold. The coordinator's only other timer is 1 ms, the UI's 1000 ms refresh is not compiled by the host tests, and a shortened alert interval is now caught by the pinned-timeout assertion. (blind-hunter)
- low (rejected) -- Title arrays are not tied to `HAPTIC_PATTERN_COUNT` (no `_Static_assert`). The harm needs a fifth pattern that Epic 3 does not plan, and `sit_stand_move.c` cannot be built here, so a new compile-time construct could not be checked. (blind-hunter, edge-case-hunter, verification-gap other finding)
- low (rejected) -- `haptic_alert_start()` twice, and session endings that bypass `prv_stop`, have no direct test. Supersession tests start it twice through the coordinator, and Stop is the only transition to inactive. (blind-hunter)
- low (rejected) -- Claim check: the spec says every earlier test passes unchanged, but two earlier assertions changed (`3` became `HAPTIC_PATTERN_COUNT`) and the timer stub was rewritten. True, and unavoidable: `3` is now a valid pattern. The fix would be to edit this spec's wording, which this review may not do. Disclosed in Implementation Notes and in the summary. (edge-case-hunter)
- medium (defer) -- The `Select to stop` cue and the fourth Settings row in `sit_stand_move.c` have no automated check: dropping or inverting the `haptic_alert_active()` block keeps every host test green. Same gap as Stories 1.6, 1.7, 2.1, and 3.1; needs an emulator run. (verification-gap)

## Design Notes

The interval (2 s) and cap-by-tick-count (60 ticks, no clock read) are my choices; the owner fixed only the 2-minute cap. They use the native `vibes_short_pulse()`, so the feel is the platform's.

The dismissal sits inside `session_coordinator_middle_control()` because `sit_stand_move.c` is not compiled by the host tests; this keeps the ordering rule testable. `sit_stand_move.c` cannot be built here (no Pebble SDK); the cue width ("Select to stop", about 14 characters at Gothic 24) is an estimate to check on the emulator.

## Verification

**Commands:**
- `sh scripts/test.sh` -- expected: passes, including the new alert tests
- `uv run --locked python scripts/pebble.py install --emulator emery` -- expected: installs (not runnable in this sandbox)

**Manual checks (if no CLI):**
- On the emery emulator: choose Until Select, Skip, confirm it repeats, the cue shows, and Select stops it without starting a break; confirm the cap, Stop, and that the clock returns. Record as emulator evidence; Pebble Time 2 feel stays an owner check.
