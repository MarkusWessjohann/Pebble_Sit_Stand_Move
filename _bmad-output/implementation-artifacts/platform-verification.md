# Pebble Time 2 Platform Verification

**Status:** Blocked — no CLI/SDK or emulator is available in this environment; physical-device behavior has not been tested.

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | Available to owner | No emulator executable is installed or available on this sandbox `PATH`. |
| Python | 3.13.5 | `python3.13 --version` |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Not installed | `command -v pebble`, `command -v pebble-tool`, and `python3.13 -m pip show pebble-tool` found no installation. |
| Pebble SDK | Not installed / exact version unknown | No app source, manifest, SDK package, or lock data exists in this repository. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

These are environment observations only. No CLI/SDK install, build, emulator launch, app install, or physical-watch test was run. The documented `pebble-tool` 5.0.18 / Python 3.13 baseline in the architecture is a candidate, not a verified pin; the SDK version remains unknown.

## Reproducibility and device test plan

Once the CLI instructions and exact compatible SDK package are available:

1. Record the Pebble Time 2 firmware/OS and emulator version. Install the CLI using the approved RePebble instructions, pin its exact version and the exact SDK package/version in the test app's build configuration, then capture all installation commands and version output.
2. Build a minimal watch app and record the exact build command, exit result, artifact identity, install command, and install result separately for emulator and physical watch. Repeat the clean build to establish reproducibility.
3. On the emulator, record supported/unsupported behavior for ordinary and overdue phase Wakeups, timed-pause Wakeup, haptic request, app foregrounding, and timing. Treat this as emulator-only evidence.
4. Before timing tests, agree with the product owner a measurable maximum transition delay and the number of repetitions required to accept it. On Pebble Time 2, repeat phase-deadline and timed-pause Wakeup tests separately after leaving the app (not force-terminating it). Record whether each Wakeup fires, whether haptic is requested and delivered, whether the UI opens/foregrounds, and measured delay from deadline. Do not define a pass without the agreed tolerance and repeat count.
5. On the physical watch, exercise overdue recovery, invalid UTC (Start stays inactive with a visible error; active committed state is preserved and scheduling/haptics suppressed), and forward/backward clock adjustments (forward correction makes due deadlines due; backward correction defers them). Exercise no more than eight scheduled Wakeups and verify at least one-minute spacing between scheduled wakeups; record boundary behavior, sustained reminder reliability, and battery observations. Keep app-level race, pause/resume, skip/extend, persistence, stale-callback, and crash-boundary acceptance with their implementing stories.
6. Mark the platform gate passed only when the required physical-watch checks and exact build/install record exist. If a behavior fails or remains untested, document it and keep dependent reminder implementation gated for a product decision.

## Results

| Check | Result |
|---|---|
| Exact target firmware/OS | Blocked — physical device not accessible from this sandbox. |
| Exact CLI and SDK pins | Blocked — CLI/SDK are not installed; installation guide returned 403. |
| Reproducible minimal-app build/install | Not run — no app source, CLI, or SDK available. |
| Emulator test | Not run — emulator unavailable in this environment. |
| Physical phase/timed-pause Wakeups, haptics, foregrounding | Not run — requires the watch and installed test app. |
| Timing tolerance, limits, battery/reliability | Not measured. |

**Gate decision:** Not passed. This report does not establish Pebble Time 2 firmware capability. Continue after obtaining accessible installation instructions and running the test plan on the owner's host/emulator and physical watch; capture exact command output and device observations here.
