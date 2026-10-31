# Pebble Time 2 Platform Verification

**Status:** Partially verified — the owner reports that the earlier starter-app revision built and ran in the `emery` emulator with Pebble Tool 5.1.2 and SDK 4.33.1, and recognized button presses. The current diagnostic harness has not been built or run. Physical Pebble Time 2 reminder capabilities remain unverified.

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | `emery` (Pebble Time 2 target) | Owner reports successful install/run and button input in the emulator. Emulator version is not recorded. No emulator executable is installed or available on this sandbox `PATH`. |
| Python | 3.13.5 | `python3.13 --version` |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Pebble Tool 5.1.2 on owner's environment | Owner reported `pebble --version` output: `Pebble Tool v5.1.2 (active SDK: v4.33.1)`. The CLI is not installed on this sandbox `PATH`. |
| App source and manifest | Diagnostic harness and manifest present | `src/c/sit_stand_move.c`, `wscript`, and `package.json` are present. The owner-reported button test refers to the earlier starter-app revision; the current C source is an unbuilt diagnostic Wakeup harness. `package.json` targets `emery` (Pebble Time 2) and declares `sdkVersion: "3"`. |
| Pebble SDK package / exact CLI and SDK pins | Observed versions, project pins unresolved | Owner reports Pebble Tool 5.1.2 and SDK 4.33.1. The manifest's `sdkVersion: "3"` identifies an API level, not exact CLI/SDK package pins. The SDK is not installed in this sandbox. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

The owner reports that the earlier starter-app revision built and ran in `emery`, where buttons were recognized. The reported toolchain was Pebble Tool 5.1.2 with active SDK 4.33.1; CLI output stated `SDK 4.33.1 is already installed.` The current diagnostic harness also builds successfully per owner report. However, installing it fails while `pypkjs` / `libpebble2` serializes an emulator `WatchVersion` packet (`KeyError: 'endpoint'` at `localhost:60529`), so no harness launch or Wakeup test has run. The full successful build output, emulator version, and clean/repeated build are unrecorded. The CLI is absent from this sandbox, and no physical-watch test was performed. The architecture's older `pebble-tool` 5.0.18 baseline is superseded as a candidate by the observed 5.1.2 environment, but the project's toolchain is not yet pinned in configuration.

## Reproducibility and device test plan

For reproducible verification and physical-device acceptance:

### Emulator diagnostic harness

The current `src/c/sit_stand_move.c` is a temporary diagnostic app, not the session implementation. In the owner's Pebble Tool 5.1.2 / SDK 4.33.1 environment, from the project root:

```sh
pebble build
pebble install --emulator emery
```

In the app, press **Up**, then leave it normally (for example, use Back; do not force-stop it); after 120 seconds, record whether the app launches, a haptic is observed, and the log contains `TEST_WAKEUP_LAUNCH` and `TEST_WAKEUP` with cookie `101`. Repeat separately with **Down** for the 180-second timed-pause-style test with cookie `102`. **Select** requests cancellation of scheduled test Wakeups; verify no callback/launch occurs at its scheduled deadline. Record the observed elapsed time and emulator version. A callback log's `haptic_requested=1` means the app called the haptic API; it does not establish physical haptic delivery. If compilation fails, retain the exact compiler output and report it before changing API calls.

1. Record the Pebble Time 2 firmware/OS and emulator version. Install the CLI using the approved RePebble instructions, pin its exact version and the exact SDK package/version in the test app's build configuration, then capture all installation commands and version output.
2. Build a minimal watch app and record the exact build command, exit result, artifact identity, install command, and install result separately for emulator and physical watch. Repeat the clean build to establish reproducibility.
3. On the emulator, record supported/unsupported behavior for ordinary and overdue phase Wakeups, timed-pause Wakeup, haptic request, app foregrounding, and timing. Treat this as emulator-only evidence.
4. Before timing tests, agree with the product owner a measurable maximum transition delay and the number of repetitions required to accept it. On Pebble Time 2, repeat phase-deadline and timed-pause Wakeup tests separately after leaving the app (not force-terminating it). Measure scheduled-deadline-to-Wakeup-callback delay and scheduled-deadline-to-app-open/foreground delay separately; also record whether each haptic is requested and delivered. Do not define a pass without the agreed tolerance and repeat count.
5. On the physical watch, exercise overdue recovery, invalid UTC (Start stays inactive with a visible error; active committed state is preserved and scheduling/haptics suppressed), and forward/backward clock adjustments (forward correction makes due deadlines due; backward correction defers them). Exercise no more than eight scheduled Wakeups and verify at least one-minute spacing between scheduled wakeups; record boundary behavior, sustained reminder reliability, and battery observations. Keep app-level race, pause/resume, skip/extend, persistence, stale-callback, and crash-boundary acceptance with their implementing stories.
6. Mark the platform gate passed only when the required physical-watch checks and exact build/install record exist. If a behavior fails or remains untested, document it and keep dependent reminder implementation gated for a product decision.

## Results

| Check | Result |
|---|---|
| Exact target firmware/OS | Blocked — physical device not accessible from this sandbox. |
| Exact CLI and SDK versions | Owner reports Pebble Tool `5.1.2` and active SDK `4.33.1` in the emulator environment. The project configuration does not pin these versions; the manifest declares Pebble SDK API version `3`. |
| Existing host build artifact evidence | `build/emery/pebble-app.elf` (ARM EABI executable) and `build/sit_stand_go.pbw` are present. `build/config.log` records Waf 2.1.4 and an SDK path at `4.33.1`; no build invocation/exit record is available, so this is evidence of artifacts/configuration, not a verified reproducible build. |
| Sandbox build attempt | Failed before build — `pebble build` returned `/usr/bin/bash: line 1: pebble: command not found`. The existing artifact was not produced or verified by this sandbox attempt. |
| Emulator install attempt | Failed before install — `pebble install --emulator emery` returned `/usr/bin/bash: line 1: pebble: command not found`. No build artifact was produced. |
| Owner-reported emulator install failure | `pebble install --emulator emery` reports `Couldn't launch pypkjs`; after the app is listed as loaded, `libpebble2` raises `KeyError: 'endpoint'` serializing the `WatchVersion` packet for `localhost:60529`. Emulator connection/install does not complete. This is separate from compilation. |
| Owner-reported C build failure | `pebble build` compiled through the `gabbro` target before `src/c/sit_stand_move.c` failed: `WAKEUP_INVALID_ID` was undeclared and `wakeup_cancel()` returns `void`. Harness changed to remove that macro use and not consume a cancellation return value. Rebuild pending; no successful harness build is claimed. |
| Owner-reported harness build | Owner confirms `pebble build` succeeds after the Wakeup API corrections. CLI/SDK are Pebble Tool 5.1.2 / SDK 4.33.1. Full output, built target list, and clean/repeat-build result are not recorded. |
| Starter-app build/install smoke test | Earlier starter-app build/install and button input were owner-confirmed. The current diagnostic harness build is owner-confirmed successful with Tool 5.1.2 / SDK 4.33.1. Its install fails on the pypkjs/libpebble2 `KeyError: 'endpoint'`; reproducibility is not established. |
| Emulator test | Earlier starter-app launch and button input were confirmed by the owner. Current diagnostic harness does not install because pypkjs fails during emulator communication. No current Wakeup, haptic, foregrounding, or timing result is available. |
| Emulator phase-deadline Wakeup | Not run — a diagnostic harness is now present in `src/c/sit_stand_move.c` (Up schedules a 120-second phase-style Wakeup). It has not been built or run in this sandbox; occurrence, haptic, foregrounding, and timing remain unobserved. |
| Emulator timed-pause Wakeup | Not run — a diagnostic harness is now present in `src/c/sit_stand_move.c` (Down schedules a 180-second timed-pause-style Wakeup). It has not been built or run in this sandbox; occurrence, haptic, foregrounding, and timing remain unobserved. |
| Physical phase-deadline Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical timed-pause Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Normal/overdue timing, UTC validity/clock correction, tolerance, Wakeup limits, battery/reliability | Not measured — requires target-device tests; tolerance and required repeat count also need owner agreement. |

**Gate decision:** Not passed. This report does not establish Pebble Time 2 firmware capability. Continue after obtaining accessible installation instructions and running the test plan on the owner's host/emulator and physical watch; capture exact command output and device observations here.
