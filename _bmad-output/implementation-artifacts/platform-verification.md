# Pebble Time 2 Platform Verification

**Status:** Partially verified — the earlier starter-app revision ran in the `emery` emulator and recognized button presses. The current diagnostic harness builds and installs successfully per owner report using the project-locked CLI, and the owner reports that the emulator Wakeup test succeeded. Physical Pebble Time 2 reminder capabilities remain unverified; individual emulator callback, foreground, haptic, and timing observations are not yet recorded.

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | `emery` (Pebble Time 2 target) | Owner reports successful install/run and button input in the emulator. Emulator version is not recorded. No emulator executable is installed or available on this sandbox `PATH`. |
| Python | 3.13.5 | `python3.13 --version` |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Project-locked Pebble Tool 5.0.40; global install is 5.1.2 | Owner reported global output `Pebble Tool v5.1.2 (active SDK: v4.33.1)`. Traceback path showed global tool environment using Python 3.15. Owner reports that using the project-locked CLI now works; `.python-version` selects Python 3.13. |
| App source and manifest | Diagnostic harness and manifest present | `src/c/sit_stand_move.c`, `wscript`, and `package.json` are present. The owner-reported button test refers to the earlier starter-app revision; the current diagnostic Wakeup harness builds and installs in the emulator per owner report. `package.json` targets `emery` (Pebble Time 2) and declares `sdkVersion: "3"`. |
| Pebble SDK package / exact CLI and SDK pins | CLI pin added; SDK pin unresolved | Project now pins Pebble Tool 5.0.40 in `pyproject.toml`; the manifest's `sdkVersion: "3"` identifies an API level, not an exact C SDK package. Owner's active SDK is reported as 4.33.1; SDK selection is not verified in this sandbox. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

The owner reports that the earlier starter-app revision built and ran in `emery`, where buttons were recognized. The global CLI was Pebble Tool 5.1.2 with active SDK 4.33.1; CLI output stated `SDK 4.33.1 is already installed.` Installing with that global tool failed in `pypkjs` / `libpebble2` during `WatchVersion` serialization (`KeyError: 'endpoint'` at `localhost:60529`); its traceback path used Python 3.15. The project now pins Tool 5.0.40 and Python 3.13, and the owner reports the diagnostic harness installs successfully using the project-locked CLI. The owner also reports that the emulator Wakeup test succeeded. These reports point to the locked toolchain resolving the install issue and emulator Wakeup delivery, although they do not isolate CLI-version and Python-version effects or provide callback logs and measurements. Emulator version, full successful output, and clean/repeated build are unrecorded. Physical-watch results have not yet been reported. The reported C SDK 4.33.1 still needs separate version control.

## Reproducibility and device test plan

For reproducible verification and physical-device acceptance:

### Emulator diagnostic harness

The current `src/c/sit_stand_move.c` is a temporary diagnostic app, not the session implementation. The repository pins Pebble Tool 5.0.40 (with `libpebble2` 0.0.31 and `pypkjs` 3.0.0) in `pyproject.toml` / `uv.lock`. From the project root, use that project environment for a controlled comparison with the global Tool 5.1.2 install:

```sh
uv run --locked pebble --version
uv run --locked pebble build
uv run --locked pebble install --emulator emery
```

This changes both the CLI version and the Python runtime relative to the failing global setup. Owner reports the project-locked combination works; the individual effect of CLI and Python versions is not isolated.

If the locked run succeeds and the cause must be isolated, repeat the install while changing only one variable at a time (same Python with the two CLI versions, then same CLI with Python 3.13 and 3.15), keeping the emulator version fixed. If the locked run fails, capture the full command/output, `python --version`, both global and locked `pebble --version` output, the selected SDK/toolchain path from the build configuration log, and the emulator version.

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
| Exact CLI and SDK versions | Owner reports global Pebble Tool `5.1.2` and active SDK `4.33.1`. Project pins Pebble Tool `5.0.40` / Python 3.13, and owner reports emulator install works with it. The manifest declares Pebble SDK API version `3`; exact C SDK package remains unresolved. |
| Existing host build artifact evidence | `build/emery/pebble-app.elf` (ARM EABI executable) and `build/sit_stand_go.pbw` are present. `build/config.log` records Waf 2.1.4 and an SDK path at `4.33.1`; no build invocation/exit record is available, so this is evidence of artifacts/configuration, not a verified reproducible build. |
| Sandbox build attempt | Failed before build — `pebble build` returned `/usr/bin/bash: line 1: pebble: command not found`. The existing artifact was not produced or verified by this sandbox attempt. |
| Emulator install attempt | Failed before install — `pebble install --emulator emery` returned `/usr/bin/bash: line 1: pebble: command not found`. No build artifact was produced. |
| Owner-reported emulator install failure (resolved by locked CLI) | Global Pebble Tool 5.1.2: `pebble install --emulator emery` reported `Couldn't launch pypkjs`; `libpebble2` raised `KeyError: 'endpoint'` serializing `WatchVersion` for `localhost:60529`. Owner reports the install now works when using the project-locked CLI (Tool 5.0.40 / Python 3.13). Full command output and emulator version are not recorded. |
| Owner-reported C build failure (resolved) | Initial `pebble build` failed because `WAKEUP_INVALID_ID` was undeclared and `wakeup_cancel()` returns `void`. The harness was corrected, and the owner subsequently confirmed `pebble build` succeeds. |
| Owner-reported harness build | Owner confirms `pebble build` succeeds after the Wakeup API corrections. Global CLI/SDK are Pebble Tool 5.1.2 / SDK 4.33.1. Full output, built target list, and clean/repeat-build result are not recorded. |
| Starter-app build/install smoke test | Earlier starter-app build/install and button input were owner-confirmed. The current diagnostic harness build is owner-confirmed successful. The global Tool 5.1.2 install failed with `KeyError: 'endpoint'`; owner reports install succeeds with project-locked Tool 5.0.40 / Python 3.13. |
| Emulator test | Earlier starter-app launch and button input were confirmed by the owner. Owner reports diagnostic harness installs using project-pinned Tool 5.0.40 / Python 3.13 and that the Wakeup test succeeded. Detailed observations were not included. |
| Emulator phase-deadline Wakeup | Owner reports Wakeup test succeeded in the emulator. Callback log/cookie, app foreground behavior, observed haptic delivery, and measured timing were not individually reported. |
| Emulator timed-pause Wakeup | Owner reports Wakeup test succeeded in the emulator. Callback log/cookie, app foreground behavior, observed haptic delivery, and measured timing were not individually reported. |
| Physical phase-deadline Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical timed-pause Wakeup | Not run — requires the watch and installed test app; occurrence, haptic, foregrounding, and timing are unobserved. |
| Normal/overdue timing, UTC validity/clock correction, tolerance, Wakeup limits, battery/reliability | Not measured — requires target-device tests; tolerance and required repeat count also need owner agreement. |

### Current sandbox verification

Commands run in this sandbox on 2026-10-31:

| Command | Result |
|---|---|
| `python3 --version` | `Python 3.13.5` |
| `uv --version` | `uv 0.12.3 (aarch64-unknown-linux-gnu)` |
| `command -v pebble`, `command -v pebble-tool`, `command -v pebble-emulator` | No executable found for any of the three commands. |
| `uv lock --check --offline` | Passed — `Resolved 53 packages in 1ms`; lockfile is consistent. This does not install or execute the CLI. |
| `pebble build` | Failed before build: `/usr/bin/bash: line 1: pebble: command not found`. |
| `pebble install --emulator emery` | Failed before install: `/usr/bin/bash: line 1: pebble: command not found`. |
| `file build/emery/pebble-app.elf build/sit_stand_go.pbw` | Existing ELF is 32-bit ARM EABI5; PBW is a ZIP archive. Existing artifacts do not establish that the current source builds reproducibly. |

The host-side failures above are this sandbox's own observed results; they do not contradict the owner's separate report of successful locked-CLI build/install and emulator Wakeup testing.

**Gate decision:** Not passed. This report does not establish Pebble Time 2 firmware capability. Continue after obtaining accessible installation instructions and running the test plan on the owner's host/emulator and physical watch; capture exact command output and device observations here.
