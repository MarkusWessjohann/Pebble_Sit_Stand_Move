---
title: 'Add emery Wakeup smoke-test harness'
type: 'feature'
created: '2026-10-31'
status: 'in-progress'
baseline_commit: 'da81c1cc805611532c9f2b114c2a7fdff76c9a47'
route: 'oneshot'
review_loop_iteration: 0
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The current starter app verifies only button input, so it cannot exercise Pebble Wakeup delivery, launch behavior, or haptic requests in the emulator.

**Approach:** Replace the starter screen with a small diagnostic screen that lets the tester schedule a short phase-style Wakeup or timed-pause-style Wakeup, leave the app, and record callback, launch reason, and haptic-request evidence. Clearly label these as emulator checks, not product functionality or physical-watch acceptance.

</frozen-after-approval>

## Implementation Notes

Implemented a temporary diagnostic watch screen in `src/c/sit_stand_move.c`: Up schedules a 120-second phase-style Wakeup, Down schedules a 180-second timed-pause-style Wakeup, and Select requests cancellation of scheduled test Wakeups. The callback logs Wakeup identity/cookie and requests a short vibration; app launch logs the launch reason. Scheduling failure is shown on screen. This is test-only scaffolding, not session functionality.

Verification is pending: Pebble Tool is not installed in this sandbox, the RePebble documentation endpoint returned HTTP 403, and running `uv run pebble --version` attempted a blocked dependency download. Owner must build and run the harness with the working Pebble Tool 5.1.2 / SDK 4.33.1 environment. Do not interpret the current source as compiled or as evidence of emulator/physical Wakeup support.

Owner supplied a traceback from `pypkjs` / `libpebble2`: packet serialization for `WatchVersion` raises `KeyError: 'endpoint'` while connecting to `localhost:60529`. Initially the invoked command was unknown; the owner has now confirmed it occurs with `pebble install --emulator emery`. This is an emulator communication failure, separate from compilation.

The owner then supplied a separate compiler diagnostic from `pebble build`: `WAKEUP_INVALID_ID` is undefined and `wakeup_cancel()` returns `void`. Updated the harness to use `(WakeupId)-1` as the failure/no-ID sentinel and call `wakeup_cancel()` without consuming a return value. The next owner build is required to verify these corrections against the installed SDK.

Owner reports that `pebble build` now succeeds with the corrected harness. `pebble install --emulator emery` fails while launching pypkjs with `KeyError: 'endpoint'` during the emulator's WatchVersion handshake. Runtime installation and Wakeup tests are blocked pending resolution of that tooling issue.

## Verification

**Commands:**
- `pebble build` -- expected: `emery` target builds without compiler errors.
- `pebble install --emulator emery` -- expected: app installs and opens in the emulator.

**Manual checks:**
- Up schedules the phase-style Wakeup for 120 seconds; leave via Back without force-stopping, then record app launch, callback log/cookie, observed elapsed time, and haptic observation.
- Down schedules the timed-pause-style Wakeup for 180 seconds; repeat the same checks separately.
- Schedule a test and note its displayed Wakeup ID and deadline. Press Select before the deadline, then wait past that deadline and verify no callback/launch occurs; retain the cancellation result log.
