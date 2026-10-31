# Pebble Time 2 Platform Verification

**Status:** Partially verified — the owner reports that the starter app builds and runs in the `emery` emulator with Pebble Tool 5.1.2 and SDK 4.33.1, and recognizes button presses. Physical Pebble Time 2 reminder capabilities remain unverified.

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | `emery` (Pebble Time 2 target) | Owner reports successful install/run and button input in the emulator. Emulator version is not recorded. No emulator executable is installed or available on this sandbox `PATH`. |
| Python | 3.13.5 | `python3.13 --version` |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Pebble Tool 5.1.2 on owner's environment | Owner reported `pebble --version` output: `Pebble Tool v5.1.2 (active SDK: v4.33.1)`. The CLI is not installed on this sandbox `PATH`. |
| App source and manifest | Starter app and manifest present | `src/c/sit_stand_move.c`, `wscript`, and `package.json` are present; the C app is the Pebble starter screen, not a dedicated verification app. `package.json` targets `emery` (Pebble Time 2) and declares `sdkVersion: "3"`. |
| Pebble SDK package / exact CLI and SDK pins | Observed versions, project pins unresolved | Owner reports Pebble Tool 5.1.2 and SDK 4.33.1. The manifest's `sdkVersion: "3"` identifies an API level, not exact CLI/SDK package pins. The SDK is not installed in this sandbox. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

The owner reports running `pebble build` and `pebble install --emulator emery`; the emulator app recognized button presses. The reported toolchain was Pebble Tool 5.1.2 with active SDK 4.33.1, and the CLI output stated `SDK 4.33.1 is already installed.` This is owner-reported emulator smoke-test evidence; the full command output, a clean/repeated build, and emulator version are not recorded. The CLI is absent from this sandbox, and no physical-watch test was performed. The architecture's older `pebble-tool` 5.0.18 baseline is superseded as a candidate by the observed 5.1.2 environment, but the project's toolchain is not yet pinned in configuration.

## Reproducibility and device test plan

For reproducible verification and physical-device acceptance:

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
| Starter-app build/install smoke test | Owner reports running `pebble build` and `pebble install --emulator emery`; app runs and recognizes buttons. Toolchain: Pebble Tool 5.1.2, SDK 4.33.1. Full command output, emulator version, and repeat-build result are unrecorded, so reproducibility is not established. |
| Emulator test | Basic app launch and button input confirmed by owner on `emery`. This does not verify Wakeups, haptics, foregrounding, or timing. The sandbox itself has no emulator available. |
| Emulator phase-deadline Wakeup | Not run — reminder behavior is not implemented in the current starter app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Emulator timed-pause Wakeup | Not run — reminder behavior is not implemented in the current starter app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical phase-deadline Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical timed-pause Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Normal/overdue timing, UTC validity/clock correction, tolerance, Wakeup limits, battery/reliability | Not measured — requires target-device tests; tolerance and required repeat count also need owner agreement. |

**Gate decision:** Not passed. This report does not establish Pebble Time 2 firmware capability. Continue after obtaining accessible installation instructions and running the test plan on the owner's host/emulator and physical watch; capture exact command output and device observations here.
