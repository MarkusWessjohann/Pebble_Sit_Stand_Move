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
