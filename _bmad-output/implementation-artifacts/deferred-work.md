- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Capture the selected Pebble C SDK/toolchain path and artifact identity from a clean locked build using SDK 4.33.1.
  evidence: `.pebble-sdk-version` and `scripts/pebble.py` now pin/enforce 4.33.1; owner reports clean/build/install success, but full output, artifact identity, and actual build-selected SDK/toolchain path are not yet recorded.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Decide a coordinator recovery policy for early Wakeup callback repair failures.
  evidence: `src/c/session_coordinator.c` surfaces schedule/storage errors, but after a callback is consumed a failed replacement can leave the committed running record with no effective alarm.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Define and enforce a minimum plausible UTC timestamp for session clock validity.
  evidence: `prv_valid_utc()` accepts any positive timestamp; the story and architecture do not define a lower valid-time bound.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Decide how the coordinator queue preserves critical events under queue pressure.
  evidence: `prv_enqueue()` drops events when the bounded queue is full, including Stop and Wakeup events; safe coalescing or reserved capacity needs a design decision.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Verify error-message fit and legibility on Pebble Time 2.
  evidence: The current fixed-height text layer may not fit all error strings; target UI verification belongs to Story 1.8.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Exercise native Pebble menu selection callbacks on the SDK or device.
  evidence: Host tests call coordinator menu dispatch directly and do not compile or invoke the native MenuLayer callback; Story 1.2 records this gap for device verification.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Decide whether reproducible build evidence must cover all target platforms or only the `emery` target.
  evidence: The manifest lists seven platforms and a full build compiles all seven; Story 1.1 records target scope as an owner decision.
- source_spec: `_bmad-output/implementation-artifacts/1-1-establish-pebble-toolchain-and-emulator-readiness.md`
  summary: Record the exact source revision used for the owner-reported emulator Wakeup observations.
  evidence: The reports concern a replaced diagnostic harness, but the tested revision was not supplied; the procedure is revision-specific.

## Deferred from: code review of spec-1-3-follow-the-cycle-and-recover-from-missed-deadlines (2026-11-02)

- Stop cannot commit while a due transition keeps failing to schedule. `prv_reconcile_before_command` returns false on a failed reconcile (`src/c/session_coordinator.c:176-190`), so `prv_process` drops Stop and a second Stop fails the same way. Accepted by the owner: Stop stays blocked until Wakeup scheduling works again. Verified at HEAD with a probe test. Revisit if Story 1.8 device testing shows real scheduling failures; this conflicts with Story 1.2 AC3 only in that failure state.

## Deferred from: code review of spec-1-5-take-and-end-a-timed-pause (2026-11-02)

- PauseEnd (on-time and overdue recovery) requests a haptic but no explicit app-open/foreground call. The Pebble Wakeup launch opens the app and the SDK has no separate foreground call; Story 1.3 accepted the same wording. Verify on Pebble Time 2 in Story 1.8 whether a PauseEnd Wakeup raises the app from the menu window and from the background.

## Deferred from: code review of spec-1-6-skip-the-active-phase (2026-11-03)

- source_spec: `_bmad-output/implementation-artifacts/spec-1-6-skip-the-active-phase.md`
  summary: Exercise the Skip menu row dispatch, resulting view, and Skip haptic from PAUSED, BREAK, and running states on the SDK or Pebble Time 2.
  evidence: The host harness does not compile `sit_stand_move.c`; removing the early return in `prv_menu_select` and the menu-to-coordinator dispatch are covered only by coordinator-level tests. Story 1.8 owns UI and haptic verification.
- source_spec: `_bmad-output/implementation-artifacts/spec-1-6-skip-the-active-phase.md`
  summary: On Pebble Time 2, check what the owner feels when Skip is tapped at a due phase deadline, which requests two back-to-back haptics (the reconciled transition, then Skip).
  evidence: Owner decided on 2026-11-03 to keep this behavior (AC3 reconciles first, then applies Skip to the resulting phase, so Skip lands one phase past the one displayed). The race lasts under a second in the foreground. Whether the watch plays, merges or drops the second pulse is unverified.

## Deferred from: code review of spec-1-7-extend-the-active-phase (2026-11-03)

- source_spec: `_bmad-output/implementation-artifacts/spec-1-7-extend-the-active-phase.md`
  summary: Exercise the running top/bottom Extend presses, the `TOP +5 BOTTOM +10` label fit and legibility, and the Extend Wakeup on the SDK or Pebble Time 2.
  evidence: The host harness does not compile `sit_stand_move.c`; the label, click handlers and the 17-character line in the 14pt controls layer are inspected only. Story 1.8 owns UI and device verification.

## Deferred from: code review of spec-2-1-enlarge-the-time-and-tally-and-place-button-labels-beside-the-buttons (2026-11-03)

- source_spec: `_bmad-output/implementation-artifacts/spec-2-1-enlarge-the-time-and-tally-and-place-button-labels-beside-the-buttons.md`
  summary: Decide whether the 144x168 (aplite, basalt, diorite, flint) and round chalk targets still built from `package.json` need a layout, or should be dropped from `targetPlatforms`.
  evidence: The new layout uses a fixed 78 px label column, a label centered at 3/4 of the screen height, and a full-width tally at `h - 46`. On 168 px height the third label (about 115 to 137 px) overlaps the tally (starts at 122 px). Intent targets Pebble Time 2 only (NFR1), so the spec left this out.
- source_spec: `_bmad-output/implementation-artifacts/spec-2-1-enlarge-the-time-and-tally-and-place-button-labels-beside-the-buttons.md`
  summary: On the emery emulator and Pebble Time 2, check the label alignment with the buttons, the 100:00 countdown, the clock height at Gothic 24, and the label/tally gap, in all lifecycle states and with an error shown.
  evidence: The positions (1/4, 1/2, 3/4 of height), the 78 px column, the 26 px clock layer and 40 px countdown layer are estimates; the sandbox has no Pebble SDK. Label 3 ends at 182 px and the tally starts at 182 px, so the margin is zero by calculation. Unverified.
- source_spec: `_bmad-output/implementation-artifacts/spec-2-1-enlarge-the-time-and-tally-and-place-button-labels-beside-the-buttons.md`
  summary: Cover the right-edge label text, phase text and `S%u T%u M%u` tally format with a host test, for example by extracting the formatting into a pure function that `scripts/test.sh` compiles.
  evidence: `sit_stand_move.c` is not compiled by the host harness. Swapping `shorter` and `longer`, or changing `II`, leaves every test passing. The same gap is already recorded for Stories 1.6 and 1.7.

- source_spec: `_bmad-output/implementation-artifacts/spec-3-2-rename-app-to-sit-stand-walk.md`
  summary: Check that "Sit Stand Walk" fits the Pebble launcher on small-screen platforms.
  evidence: Unverified (maybe-false, would be low-to-medium); needs an emulator or device look at the app list.

## Deferred from: code review of spec-3-1-choose-a-haptic-pattern-in-settings (2026-11-03)

- source_spec: `_bmad-output/implementation-artifacts/spec-3-1-choose-a-haptic-pattern-in-settings.md`
  summary: Cover the Settings menu row mapping and Settings window flow with a host test, for example by moving the row count and Settings-row index into an SDK-free function that `scripts/test.sh` compiles.
  evidence: `sit_stand_move.c` is not compiled by the host harness. Changing the Settings-row constant (`1`/`3`), dropping the early `return` in `prv_menu_select`, reordering `s_pattern_titles`, or popping on a failed save keeps every host test green. Same gap as Stories 1.6, 1.7, and 2.1; unverified until an emulator run.
- source_spec: `_bmad-output/implementation-artifacts/spec-3-1-choose-a-haptic-pattern-in-settings.md`
  summary: Check whether a lifecycle change while the menu is open (a wakeup ending a timed pause, or recovery) can make a menu row act on a different command than the one shown.
  evidence: Unverified (medium if real). The row index is derived from live lifecycle at draw and again at select, as the pre-Settings menu already did; the Settings row only adds a constant offset. Settling it needs an emulator run with a wakeup firing while the menu is open.

## Deferred from: code review of spec-3-2-repeat-the-alert-until-select-is-pressed (2026-11-03)

- source_spec: `_bmad-output/implementation-artifacts/spec-3-2-repeat-the-alert-until-select-is-pressed.md`
  summary: On the emery emulator and Pebble Time 2, check the `Select to stop` cue (fits the clock line at Gothic 24, appears and clears), the fourth Settings row, and Select dismissal on the session view.
  evidence: `sit_stand_move.c` is not compiled by the host harness and the sandbox has no Pebble SDK. Dropping or inverting the `haptic_alert_active()` block keeps every host test green. About 14 characters at Gothic 24 against a full-width, 26 px clock layer is an estimate. Same gap as Stories 1.6, 1.7, 2.1, and 3.1.
- source_spec: `_bmad-output/implementation-artifacts/spec-3-2-repeat-the-alert-until-select-is-pressed.md`
  summary: Decide whether Select should also dismiss a repeating alert while the menu or Settings window is on top.
  evidence: Owner decision 2A (2026-11-03) limits dismissal to the session view, so an alert that starts while a menu is open keeps vibrating until the owner backs out, or until the 2-minute cap. Revisit if that proves annoying in use.

## Deferred from: code review of spec-4-1-set-the-length-of-sit-stand-and-move-in-settings (2026-11-03)

- source_spec: `_bmad-output/implementation-artifacts/spec-4-1-set-the-length-of-sit-stand-and-move-in-settings.md`
  summary: On the emery emulator and Pebble Time 2, check the seven-row Settings window: `Stand 60 min` fits without clipping, the list scrolls, the selection stays on the stepped row after the reload, and a step plays no preview and keeps the window open.
  evidence: `sit_stand_move.c` is not compiled by the host harness and the sandbox has no Pebble SDK. Breaking the row index math, the reload, or the stay-open behavior keeps every host test green. Same gap as Stories 1.6, 1.7, 2.1, 3.1, and 3.2.
- source_spec: `_bmad-output/implementation-artifacts/spec-4-1-set-the-length-of-sit-stand-and-move-in-settings.md`
  summary: Amend the PRD (section 7.2, out of scope: "User-configurable default Phase lengths") and the architecture spine (AD-6 and the "Default durations" convention) so they no longer say the lengths are fixed.
  evidence: Epic 4 and `epics.md` (FR12 to FR15, NFR7 amended) allow configurable lengths, but the PRD and architecture still state the opposite, so the planning documents contradict each other.
