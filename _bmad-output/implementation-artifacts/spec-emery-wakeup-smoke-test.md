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

Verification is pending in this sandbox: Pebble Tool is not installed here, the RePebble documentation endpoint returned HTTP 403, and running `uv run pebble --version` attempted a blocked dependency download. Owner confirms the harness builds and installs in the emulator using the project's locked Pebble Tool 5.0.40 / Python 3.13 environment and reports that the Wakeup test succeeded. Physical device capability and detailed emulator callback, foreground, haptic, and timing observations remain unverified/unrecorded.

Owner supplied a traceback from `pypkjs` / `libpebble2`: packet serialization for `WatchVersion` raises `KeyError: 'endpoint'` while connecting to `localhost:60529`. The owner confirmed it occurs with global Pebble Tool 5.1.2 when running `pebble install --emulator emery`. This is an emulator communication failure, separate from compilation.

The owner then supplied a separate compiler diagnostic from `pebble build`: `WAKEUP_INVALID_ID` is undefined and `wakeup_cancel()` returns `void`. Updated the harness to use `(WakeupId)-1` as the failure/no-ID sentinel and call `wakeup_cancel()` without consuming a return value. The owner confirmed the subsequent build succeeds.

Owner reports that `pebble build` succeeds with the corrected harness. Global Tool 5.1.2 installation failed while launching pypkjs with `KeyError: 'endpoint'` during the emulator's WatchVersion handshake; installation through the project-locked CLI now works. The owner reports the emulator Wakeup test succeeded; detailed individual observations were not supplied.

Follow-up investigation found that the owner ran global Pebble Tool 5.1.2 under Python 3.15 (from the traceback path), while the repository selects Python 3.13 and `uv.lock` resolves Pebble Tool 5.0.40, `libpebble2` 0.0.31, and `pypkjs` 3.0.0. The owner reports that installation succeeds with the project-locked CLI. Since CLI and Python versions changed together, the exact cause is not isolated.

Pinned the direct project dependency to Pebble Tool 5.0.40 in `pyproject.toml`; the existing lockfile already resolved this exact version and its transitives. `uv lock --check --offline` succeeds. The locked CLI could not be executed in this sandbox because required Python wheels were not available and downloads were blocked. The owner reports that install succeeds using the locked environment.

The successful compatibility attempt changes both Python (global 3.15 to project 3.13) and Pebble Tool (5.1.2 to 5.0.40), so the individual cause is not isolated. To establish it, repeat with one variable changed at a time while keeping the emulator version fixed.

## Verification

**Commands:**
- `uv run --locked pebble build` -- expected: `emery` target builds without compiler errors using locked Pebble Tool 5.0.40.
- `uv run --locked pebble install --emulator emery` -- expected: app installs and opens in the emulator using the locked CLI.
- `uv run --locked pebble --version` -- expected: show locked Pebble Tool 5.0.40 under project Python 3.13. Compare with global Tool 5.1.2/Python 3.15; note that this changes both variables. The host SDK must resolve to the owner's installed 4.33.1 SDK.

**Manual checks:**
- Up schedules the phase-style Wakeup for 120 seconds; leave via Back without force-stopping, then record app launch, callback log/cookie, observed elapsed time, and haptic observation.
- Down schedules the timed-pause-style Wakeup for 180 seconds; repeat the same checks separately.
- Schedule a test and note its displayed Wakeup ID and deadline. Press Select before the deadline, then wait past that deadline and verify no callback/launch occurs; retain the cancellation result log.
