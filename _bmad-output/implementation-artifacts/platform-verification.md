# Pebble Time 2 Platform Verification

**Status:** Blocked — no CLI/SDK or emulator is available in this environment; physical-device behavior has not been tested.

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | Available to owner | No emulator executable is installed or available on this sandbox `PATH`; emulator install was not possible. |
| Python | 3.13.5 | `python3.13 --version` |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Not installed | `command -v pebble`, `command -v pebble-tool`, and `python3 -m pip show pebble-tool` found no installation. |
| App source and manifest | Starter app and manifest present | `src/c/sit_stand_move.c`, `wscript`, and `package.json` are present; the C app is the Pebble starter screen, not a dedicated verification app. `package.json` targets `emery` (Pebble Time 2) and declares `sdkVersion: "3"`. |
| Pebble SDK package / exact CLI and SDK pins | Unresolved | The manifest's `sdkVersion: "3"` identifies an API level, not exact CLI/SDK package pins. Existing build configuration records an SDK/toolchain path at version `4.33.1`, but the SDK is not installed in this sandbox and the CLI version/command is not recorded. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

These are environment observations only. The documented `pebble-tool` 5.0.18 / Python 3.13 baseline in the architecture is a candidate, not a verified pin. A local SDK/toolchain configuration log reports `4.33.1`, but does not identify an independently reproducible CLI command or a verified package pin. No CLI/SDK install, successful build in this sandbox, emulator launch, successful app install, or physical-watch test was performed.

## Reproducibility and device test plan

Once the CLI instructions and exact compatible SDK package are available:

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
| Exact CLI and SDK pins | Blocked — CLI is not installed and the installation guide returned 403. The manifest declares Pebble SDK API version `3`; existing build configuration references SDK/toolchain `4.33.1`, but neither establishes the CLI version or a reproducible version pin. |
| Existing host build artifact evidence | `build/emery/pebble-app.elf` (ARM EABI executable) and `build/sit_stand_go.pbw` are present. `build/config.log` records Waf 2.1.4 and an SDK path at `4.33.1`; no build invocation/exit record is available, so this is evidence of artifacts/configuration, not a verified reproducible build. |
| Sandbox build attempt | Failed before build — `pebble build` returned `/usr/bin/bash: line 1: pebble: command not found`. The existing artifact was not produced or verified by this sandbox attempt. |
| Emulator install attempt | Failed before install — `pebble install --emulator emery` returned `/usr/bin/bash: line 1: pebble: command not found`. No build artifact was produced. |
| Reproducible minimal-app build/install | Blocked — CLI/SDK unavailable; the repository starter app has not been built or installed. |
| Emulator test | Not run — emulator unavailable in this environment and no app could be installed. |
| Emulator phase-deadline Wakeup | Not run — no emulator or installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Emulator timed-pause Wakeup | Not run — no emulator or installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical phase-deadline Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical timed-pause Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Normal/overdue timing, UTC validity/clock correction, tolerance, Wakeup limits, battery/reliability | Not measured — requires target-device tests; tolerance and required repeat count also need owner agreement. |

**Gate decision:** Not passed. This report does not establish Pebble Time 2 firmware capability. Continue after obtaining accessible installation instructions and running the test plan on the owner's host/emulator and physical watch; capture exact command output and device observations here.
