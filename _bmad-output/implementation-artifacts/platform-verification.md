# Pebble Time 2 Platform Verification

**Status:** Pre-implementation readiness is incomplete: exact C SDK package/version selection and complete reproducible build/install evidence are missing. The owner reports successful project-locked build/install and separate phase and timed-pause emulator Wakeup tests, but detailed callback/foreground/haptic-request/timing observations are not recorded. Physical Pebble Time 2 acceptance with the finished app is not run and is assigned to Story 1.8 as a pre-release gate after Stories 1.2–1.7.

## Pre-implementation toolchain and emulator readiness (Story 1.1)

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware (Story 1.8) | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | `emery` (Pebble Time 2 target) | Owner reports successful install/run and button input in the emulator. Emulator version is not recorded. No emulator executable is installed or available on this sandbox `PATH`. |
| Python | 3.13.5 | Local `python3 --version`; `.python-version` now pins `3.13.5`. |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Project dependency pin: Pebble Tool 5.0.40; global install reported as 5.1.2 | `pyproject.toml` pins `pebble-tool==5.0.40`; `uv.lock` resolves 5.0.40. Owner reported global output `Pebble Tool v5.1.2 (active SDK: v4.33.1)`. Traceback path showed global tool environment using Python 3.15. The project CLI is not installed in this sandbox, so its runtime version was not locally invoked. |
| App source and manifest | Diagnostic harness and manifest present | `src/c/sit_stand_move.c`, `wscript`, and `package.json` are present. The owner-reported button test refers to the earlier starter-app revision; the current diagnostic Wakeup harness builds and installs in the emulator per owner report. `package.json` targets `emery` (Pebble Time 2) and declares `sdkVersion: "3"`. |
| Pebble SDK package / exact CLI and SDK pins | Owner-reported active SDK: 4.33.1; project-locked SDK selection/pin remains unverified | On 2026-11-01 the owner supplied `pebble sdk list` output: `Installed SDKs: 4.33.1 (active)`. This confirms 4.33.1 is installed and active in the owner's CLI environment. `package.json` declares `sdkVersion: "3"` (API level, not package/version). Existing host `build/config.log` references toolchain files under `SDKs/4.33.1` and SDK files under `SDKs/current`; the project does not yet pin this SDK version or prove the project-locked CLI selects it. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

The owner reports that the earlier starter-app revision built and ran in `emery`, where buttons were recognized. The global CLI was Pebble Tool 5.1.2 with active SDK 4.33.1; CLI output stated `SDK 4.33.1 is already installed.` The owner has since supplied `pebble sdk list` output showing `4.33.1 (active)`, confirming the installed/active SDK version in that CLI environment. Installing with the global tool failed in `pypkjs` / `libpebble2` during `WatchVersion` serialization (`KeyError: 'endpoint'` at `localhost:60529`); its traceback path used Python 3.15. The project now pins Tool 5.0.40 and Python 3.13, and the owner reports the diagnostic harness installs successfully using the project-locked CLI. The owner also reports that the emulator Wakeup test succeeded. These reports point to the locked toolchain resolving the install issue and emulator Wakeup delivery, although they do not isolate CLI-version and Python-version effects or provide callback logs and measurements. Emulator version, full successful output, and clean/repeated build are unrecorded. The project configuration still does not pin SDK 4.33.1 or demonstrate that the locked CLI selects that active SDK. Physical-watch results have not yet been reported.

## Pre-implementation verification procedure

Story 1.1 records pinned toolchain/build readiness and available emulator observations only. It does not establish physical-device capability.

### Emulator diagnostic harness

The current `src/c/sit_stand_move.c` is a temporary diagnostic app, not the session implementation. The repository pins Pebble Tool 5.0.40 (with `libpebble2` 0.0.31 and `pypkjs` 3.0.0) in `pyproject.toml` / `uv.lock`. From the project root, use that project environment for a controlled comparison with the global Tool 5.1.2 install:

```sh
uv run --locked pebble --version
uv run --locked pebble build
uv run --locked pebble install --emulator emery
```

This changes both the CLI version and the Python runtime relative to the failing global setup. Owner reports the project-locked combination works; the individual effect of CLI and Python versions is not isolated.

If the locked run is repeated, capture full command output and artifact identity. If the cause of the earlier global install failure must be isolated, change one variable at a time while keeping the emulator version fixed. If a run fails, capture the full command/output, `python --version`, locked `pebble --version`, selected SDK/toolchain path from the build configuration log, and emulator version.

In the emulator harness, press **Up**, then leave it normally so it is no longer foregrounded (for example, use Back; do not force-stop it); after 120 seconds, record whether the app launches, whether a haptic request is logged/observed, and whether the log contains `TEST_WAKEUP_LAUNCH` and `TEST_WAKEUP` with cookie `101`. Repeat separately with **Down** for the 180-second timed-pause-style test with cookie `102`. **Select** requests cancellation of scheduled test Wakeups; verify that no callback log and no app launch occur after the cancelled deadline. Record elapsed time and emulator version. A callback log's `haptic_requested=1` means the app called the haptic API; it does not establish physical haptic delivery or physical-device capability. If compilation fails, retain exact compiler output before changing API calls.

1. Pin and record the exact CLI, Python, and C SDK package/version in the project build configuration; `package.json`'s SDK API level is not an exact SDK package pin.
2. Build/install the diagnostic app using the pinned workflow. Record exact commands, full output, exit results, artifact identity, and clean/repeated build outcomes. Label owner-reported evidence separately from locally executed commands.
3. Record available emulator phase and timed-pause observations separately, including callback occurrence, haptic request (not physical delivery), app open/foreground, timing, cancellation behavior, and evidence source.

## Pre-release physical-device acceptance (Story 1.8)

Run only after Stories 1.2–1.7 are complete. Use the finished app on the Pebble Time 2 and record physical results separately from the toolchain/emulator evidence above. Story 1.8 contains the complete acceptance criteria; key evidence includes:

1. Record device model, firmware/OS, finished build artifact, and CLI/SDK identifiers; install and exercise the finished app.
2. Separately schedule phase-deadline and timed-pause Wakeups, then normally leave the app so it is no longer foregrounded (do not force-terminate it); record callback, haptic request and observed delivery, foreground/open behavior, phase state, and timing. Agree a measurable tolerance and repeat count with the owner before timing acceptance.
3. Test timed pause choices, same-phase resume with saved remaining time, manual early resume and stale-callback suppression, overdue pause-end recovery, Skip, manual Pause/Resume, Stop, Extend, deadline/action races, cancellation/rescheduling, and phase overdue recovery.
4. Test invalid UTC and forward/backward clock corrections; verify Wakeup maximum and spacing, `notify_if_missed = false`, sustained reliability, and battery observations.
5. Test finished-UI layout, native controls, countdown, phase/state/tally/error legibility on the target display. Include persistence/schedule/cancel/launch-repair failures and crash boundaries; assert committed lifecycle/state, Wakeup identity/generation, cleanup/retry, visible errors, and haptic count.
6. Story 1.8 is the release gate. Observe one complete 45/10/5 cycle on the finished device. The owner reviews battery/reliability observations for release suitability; unacceptable or inconclusive results keep the gate open. Record failures or untested checks, keep it open, and block release until the owner resolves affected requirements. These physical checks do not block the start of Stories 1.2–1.7.

## Results

| Check | Result |
|---|---|
| Exact target firmware/OS (Story 1.8) | Not recorded — physical acceptance has not been run. |
| Exact CLI and SDK versions (Story 1.1) | Project dependency pins Pebble Tool `5.0.40`; `.python-version` pins Python `3.13.5`, which matches the locally available Python. Owner supplied `pebble sdk list` output confirming SDK `4.33.1` is installed and active; owner reports the locked CLI/Python setup works. `package.json` SDK API level is `3`; project-pinned SDK version and locked-environment SDK selection remain unresolved. |
| Existing host build artifact evidence (Story 1.1) | `build/emery/pebble-app.elf` (ARM EABI executable), `build/sit_stand_go.pbw`, and `build/sit_stand_walk.pbw` are present. `build/config.log` records Waf 2.1.4 and host SDK/toolchain paths including `4.33.1`; no build invocation/exit record is available, so these are existing artifact/configuration evidence, not a verified reproducible build. SHA-256: `pebble-app.elf` `ad5fa2081e1bff12cbb09ed93fe657b0302943e55e77472dcb33472be047ae39`; `sit_stand_go.pbw` `14a650f58f5e56d9fc55255959dda38a5126077282bcc2c4764e606dfc1c6f6c`; `sit_stand_walk.pbw` `066e3ee65a5741b9667305d87e8de7317b26063455fee9af39aa5d90b245f180`. |
| Sandbox CLI/build/install attempts | `pebble --version`, `pebble build`, and `pebble install --emulator emery` each exited before execution with `/usr/bin/bash: line 1: pebble: command not found` (shell status 127). The existing artifacts were not produced or verified by these attempts. |
| Owner-reported emulator install failure (resolved by locked CLI) | Global Pebble Tool 5.1.2: `pebble install --emulator emery` reported `Couldn't launch pypkjs`; `libpebble2` raised `KeyError: 'endpoint'` serializing `WatchVersion` for `localhost:60529`. Owner reports the install now works when using the project-locked CLI (Tool 5.0.40 / Python 3.13). Full command output and emulator version are not recorded. |
| Owner-reported C build failure (resolved) | Initial `pebble build` failed because `WAKEUP_INVALID_ID` was undeclared and `wakeup_cancel()` returns `void`. The harness was corrected, and the owner subsequently confirmed `pebble build` succeeds. |
| Owner-reported harness build | Owner confirms `pebble build` succeeds after the Wakeup API corrections. Global CLI/SDK are Pebble Tool 5.1.2 / SDK 4.33.1. Full output, built target list, and clean/repeat-build result are not recorded. |
| Starter-app build/install smoke test | Earlier starter-app build/install and button input were owner-confirmed. The current diagnostic harness build is owner-confirmed successful. The global Tool 5.1.2 install failed with `KeyError: 'endpoint'`; owner reports install succeeds with project-locked Tool 5.0.40 / Python 3.13. |
| Emulator test | Earlier starter-app launch and button input were confirmed by the owner. Owner reports diagnostic harness installs using project-pinned Tool 5.0.40 / Python 3.13 and that the Wakeup test succeeded. Detailed observations were not included. |
| Emulator phase-deadline Wakeup (Story 1.1) | Owner-reported success only. The owner did not supply whether the app had left foreground, callback/launch logs or cookie, haptic-request evidence, app-open/foreground observation, emulator version, or measured timing. Physical haptic delivery is not inferred. |
| Emulator timed-pause Wakeup (Story 1.1) | Owner-reported success only. The owner did not supply whether the app had left foreground, callback/launch logs or cookie, haptic-request evidence, app-open/foreground observation, emulator version, or measured timing. Physical haptic delivery is not inferred. |
| Physical phase-deadline and timed-pause Wakeups (Story 1.8) | Not run — finished app/physical target acceptance pending; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical normal/overdue timing, UTC validity/clock correction, tolerance, Wakeup limits, battery/reliability, UI | Not measured — Story 1.8 acceptance with finished app is pending; tolerance and required repeat count need owner agreement. |

### Current sandbox verification

Commands run in this sandbox on 2026-10-31:

| Command | Result |
|---|---|
| `python3 --version` | `Python 3.13.5` (exit 0). |
| `uv --version` | `uv 0.12.3 (aarch64-unknown-linux-gnu)` (exit 0). |
| `command -v pebble`; `command -v pebble-tool`; `command -v pebble-emulator` | No executable found for any; each lookup failed (exit 1). |
| `python3 -m pip show pebble-tool` | `WARNING: Package(s) not found: pebble-tool` (exit 1). |
| `uv lock --check --offline` | Passed (exit 0) — `Resolved 53 packages in 1ms`; lockfile is consistent. This does not install or execute the CLI. |
| `pebble --version`; `pebble build`; `pebble install --emulator emery` | Each failed before execution (exit 127): `/usr/bin/bash: line 1: pebble: command not found`. No build/install run occurred. |
| `uv run --locked pebble --version` | Failed before the CLI launched: uv could not download `greenlet==3.5.6` because the package host connection was blocked. The locked CLI version was therefore not locally executed. |
| `file build/emery/* build/sit_stand_go.pbw build/sit_stand_walk.pbw` | Existing `pebble-app.elf` is a 32-bit ARM EABI5 executable; both PBW files are ZIP archives. These existing artifacts do not establish that the current source builds reproducibly. |
| `sha256sum build/emery/pebble-app.elf build/sit_stand_go.pbw build/sit_stand_walk.pbw` | Hashes recorded in “Existing host build artifact evidence” above; artifact identity only, not build provenance. |

The local source of the CLI pin is `pyproject.toml` (`pebble-tool==5.0.40`) plus `uv.lock` (resolved 5.0.40); `.python-version` now pins the locally verified Python `3.13.5`. Neither this sandbox nor the checked-in configuration supplies an exact C SDK package/version pin. No package installation was attempted.

The host-side failures above are this sandbox's own observed results; they do not contradict the owner's separate report of successful locked-CLI build/install and emulator Wakeup testing.

**Story 1.1 decision:** In progress, not done. SDK 4.33.1 is confirmed by owner-supplied `pebble sdk list` output as installed and active in the owner's CLI environment, but the project does not pin it or verify SDK selection through the locked CLI. Complete clean/repeated build/install output and artifact provenance are unavailable locally. `.python-version` pins Python 3.13.5. Owner-reported build/install and separate phase/timed-pause emulator test success are recorded as reports only; callback details, haptic requests, foreground behavior, timing, and emulator version were not supplied. The sandbox has no installed Pebble CLI or emulator, and uv could not download the locked environment dependency, so those runs could not be reproduced here.

**Story 1.8 decision:** Pending. No physical-device acceptance is asserted. Run the finished-app checks after Stories 1.2–1.7, including a complete 45/10/5 cycle and separate background phase/timed-pause Wakeup tests. The project owner reviews battery/reliability evidence; inconclusive or unacceptable results keep the gate open. Do not release until required checks pass or the owner resolves an unsupported requirement.
