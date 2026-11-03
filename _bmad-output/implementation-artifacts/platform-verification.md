# Pebble Time 2 Platform Verification

**Status:** Pre-implementation readiness is incomplete: SDK 4.33.1 is pinned and guarded as the active host SDK for documented project commands, but the SDK/toolchain path selected by the locked CLI and complete reproducible build/install evidence have not been captured. On 2026-11-02 the owner supplied external-host output `Pebble Tool v5.1.2 (active SDK: v4.33.1)` and identified `pebble install --emulator emery --logs` as the emulator command, reporting that host build tools and emulator work. This is the global CLI version; it does not establish a run with the project-locked Tool 5.0.40. The owner previously reported successful clean/build/install and separate phase and timed-pause emulator Wakeup tests, but detailed output and callback/foreground/haptic-request/timing observations are not recorded. The Wakeup diagnostic harness behind those emulator reports was replaced by the session app in `be85b56`; its last revision is `ef33c2f`, which predates the SDK pin and wrapper (`7fa97ee`). Physical Pebble Time 2 acceptance with the finished app is not run and is assigned to Story 1.8 as a pre-release gate after Stories 1.2–1.7.

## Pre-implementation toolchain and emulator readiness (Story 1.1)

## Target and environment evidence

| Item | Observed value | Evidence/status |
|---|---|---|
| Target hardware (Story 1.8) | Pebble Time 2 | Owner confirmed access; device is not connected to this sandbox, so firmware/OS is unrecorded. |
| Emulator | `emery` (Pebble Time 2 target) | Owner reports successful install/run and button input in the emulator. Emulator version is not recorded. No emulator executable is installed or available on this sandbox `PATH`. |
| Python | 3.13.5 | Local `python3 --version`; `.python-version` now pins `3.13.5`. |
| uv | 0.12.3 | `uv --version` |
| Pebble CLI | Project dependency pin: Pebble Tool 5.0.40; global install reported as 5.1.2 | `pyproject.toml` pins `pebble-tool==5.0.40`; `uv.lock` resolves 5.0.40. Owner reported global output `Pebble Tool v5.1.2 (active SDK: v4.33.1)`. Traceback path showed global tool environment using Python 3.15. The project CLI is not installed in this sandbox, so its runtime version was not locally invoked. |
| App source and manifest | Session app and manifest present | `src/c/sit_stand_move.c`, `wscript`, and `package.json` are present. `src/c/sit_stand_move.c` is the session app from Stories 1.2 and 1.3; the Wakeup diagnostic harness (added `818abf5`, replaced `be85b56`, last revision `ef33c2f`) is no longer in the tree. The owner-reported button test refers to the earlier starter-app revision, and the owner-reported Wakeup tests to the harness; which revision the owner actually built is not recorded. `package.json` declares `sdkVersion: "3"` and lists seven `targetPlatforms` (aplite, basalt, chalk, diorite, emery, flint, gabbro), so a full build compiles all seven; the emulator target in the reports is `emery` (Pebble Time 2). |
| Pebble SDK package / exact CLI and SDK pins | Project pins: Pebble Tool 5.0.40, Python 3.13.5, SDK 4.33.1 | `.pebble-sdk-version` pins `4.33.1`; `scripts/pebble.py` checks `pebble sdk list` using the locked CLI and refuses build/install commands unless 4.33.1 is active. On 2026-11-02 the owner supplied external-host output `Pebble Tool v5.1.2 (active SDK: v4.33.1)`; on 2026-11-01 the owner reported `pebble sdk list` output confirming `4.33.1 (active)`. The v5.1.2 output is the global CLI, not the project-locked Tool 5.0.40. `package.json` declares `sdkVersion: "3"` (API level, not package/version). The SDK/toolchain selected by a locked build and its path have not been captured. |
| CLI installation documentation | Not retrieved | `curl -L --max-time 30 --fail --silent --show-error https://developer.repebble.com/sdk/` returned HTTP 403 in this environment. |

The owner reports that the earlier starter-app revision built and ran in `emery`, where buttons were recognized. The global CLI was Pebble Tool 5.1.2 with active SDK 4.33.1; CLI output stated `SDK 4.33.1 is already installed.` The owner has supplied `pebble sdk list` output showing `4.33.1 (active)`. Installing with the global tool failed in `pypkjs` / `libpebble2` during `WatchVersion` serialization (`KeyError: 'endpoint'` at `localhost:60529`); its traceback path used Python 3.15. The project pins Tool 5.0.40, Python 3.13.5, and SDK 4.33.1 in `.pebble-sdk-version`; its README commands use `scripts/pebble.py` to guard build/install on SDK 4.33.1 being active. The owner reports clean/build/install success, but full command output, artifact identity, and selected SDK/toolchain path are not yet recorded. The owner also reports emulator Wakeup success; emulator version and callback, haptic-request, foreground, and timing details remain incomplete. Physical-watch results have not yet been reported.

## Pre-implementation verification procedure

Story 1.1 records pinned toolchain/build readiness and available emulator observations only. It does not establish physical-device capability.

### Emulator diagnostic harness (revision `ef33c2f`)

The diagnostic harness no longer exists at HEAD, where `src/c/sit_stand_move.c` is the session app. The Up/Down/Select procedure below applies only to the harness revision `ef33c2f` (check it out in a separate worktree; it has no `.pebble-sdk-version` and no `scripts/pebble.py`, so the wrapper is unavailable there and the active SDK must be confirmed by hand with `pebble sdk list`). The repository pins Pebble Tool 5.0.40 (with `libpebble2` 0.0.31 and `pypkjs` 3.0.0) in `pyproject.toml` / `uv.lock`. From the project root, use that project environment for a controlled comparison with the global Tool 5.1.2 install:

```sh
uv run --locked pebble --version
uv run --locked pebble build
uv run --locked pebble install --emulator emery
```

This changes both the CLI version and the Python runtime relative to the failing global setup. Owner reports the project-locked combination works; the individual effect of CLI and Python versions is not isolated.

If the locked run is repeated, capture full command output and artifact identity. If the cause of the earlier global install failure must be isolated, change one variable at a time while keeping the emulator version fixed. If a run fails, capture the full command/output, `python --version`, locked `pebble --version`, selected SDK/toolchain path from the build configuration log, and emulator version.

In the emulator harness at `ef33c2f`, press **Up**, then leave it normally so it is no longer foregrounded (for example, use Back; do not force-stop it); after 120 seconds, record whether the app launches, whether a haptic request is logged/observed, and whether the log contains `TEST_WAKEUP_LAUNCH` and `TEST_WAKEUP` with cookie `101`. Repeat separately with **Down** for the 180-second timed-pause-style test with cookie `102`. **Select** requests cancellation of scheduled test Wakeups; verify that no callback log and no app launch occur after the cancelled deadline. Record elapsed time and emulator version. A callback log's `haptic_requested=1` means the app called the haptic API; it does not establish physical haptic delivery or physical-device capability. If compilation fails, retain exact compiler output before changing API calls.

1. Pin and record the exact CLI, Python, and C SDK package/version in the project build configuration; `package.json`'s SDK API level is not an exact SDK package pin.
2. Build/install the app at a recorded commit using the pinned workflow, starting with `uv run --locked python scripts/pebble.py clean`. Record the commit, exact commands, full output, exit results, artifact identity, and clean/repeated build outcomes. The run count and the definition of artifact identity are owner decisions listed in Story 1.1; a `.pbw` hash may differ between runs if it embeds timestamps, so state how a difference is treated. Label owner-reported evidence separately from locally executed commands.
3. Record available emulator phase and timed-pause observations separately, including callback occurrence, haptic request (not physical delivery), app open/foreground, timing, cancellation behavior, and evidence source.

## Pre-release physical-device acceptance (Story 1.8)

Story 1.8 in `_bmad-output/planning-artifacts/epics.md` is the single source for the physical acceptance criteria; this report no longer copies them. Run it only after Stories 1.2–1.7 are complete, with the finished app, and record physical results separately from the toolchain and emulator evidence above. Physical checks do not block the start of Stories 1.2–1.7.

The operator-run protocol and evidence template for Story 1.8 is `_bmad-output/implementation-artifacts/device-acceptance-1-8.md`. Record physical results there, not here.

Open items carried from Story 1.1, to settle before Story 1.8 timing tests start:

- A measurable timing tolerance and a repeat count, agreed with the owner. No value is invented here.
- Exact device model, firmware/OS, build artifact, and CLI/SDK identifiers.
- Unacceptable or inconclusive battery or reliability results keep the gate open until the owner decides.

## Results

| Check | Result |
|---|---|
| Exact target firmware/OS (Story 1.8) | Not recorded — physical acceptance has not been run. |
| Exact CLI and SDK versions (Story 1.1) | Project pins Pebble Tool `5.0.40`, Python `3.13.5`, and SDK `4.33.1` in `.pebble-sdk-version`; `scripts/pebble.py` enforces 4.33.1 as active for documented build/install commands. Owner supplied `pebble sdk list` output confirming it is active. `package.json` SDK API level is `3`; locked build SDK/toolchain selection/path still needs captured build evidence. |
| Existing host build artifact evidence (Story 1.1) | `build/emery/pebble-app.elf` (ARM EABI executable), `build/sit_stand_go.pbw`, and `build/sit_stand_walk.pbw` are present. `build/config.log` records Waf 2.1.4 and host SDK/toolchain paths including `4.33.1`; no build invocation/exit record is available, so these are existing artifact/configuration evidence, not a verified reproducible build. SHA-256: `pebble-app.elf` `ad5fa2081e1bff12cbb09ed93fe657b0302943e55e77472dcb33472be047ae39`; `sit_stand_go.pbw` `14a650f58f5e56d9fc55255959dda38a5126077282bcc2c4764e606dfc1c6f6c`; `sit_stand_walk.pbw` `066e3ee65a5741b9667305d87e8de7317b26063455fee9af39aa5d90b245f180`. |
| Sandbox CLI/build/install attempts | `pebble --version`, `pebble build`, and `pebble install --emulator emery` each exited before execution with `/usr/bin/bash: line 1: pebble: command not found` (shell status 127). The existing artifacts were not produced or verified by these attempts. |
| Owner-reported emulator install failure (owner-reported as resolved by locked CLI; not independently verified) | Global Pebble Tool 5.1.2: `pebble install --emulator emery` reported `Couldn't launch pypkjs`; `libpebble2` raised `KeyError: 'endpoint'` serializing `WatchVersion` for `localhost:60529`. Owner reports the install now works when using the project-locked CLI (Tool 5.0.40 / Python 3.13). Full command output and emulator version are not recorded. |
| Owner-reported C build failure (resolved) | Initial `pebble build` failed because `WAKEUP_INVALID_ID` was undeclared and `wakeup_cancel()` returns `void`. The harness was corrected, and the owner subsequently confirmed `pebble build` succeeds. |
| Owner-reported harness build | Owner confirms `pebble build` succeeds after the Wakeup API corrections. The revision is not recorded and the harness predates the SDK pin and wrapper. Global CLI/SDK are Pebble Tool 5.1.2 / SDK 4.33.1. Full output, built target list, and clean/repeat-build result are not recorded. |
| Starter-app build/install smoke test | Earlier starter-app build/install and button input were owner-confirmed. The current diagnostic harness build is owner-confirmed successful. The global Tool 5.1.2 install failed with `KeyError: 'endpoint'`; owner reports install succeeds with project-locked Tool 5.0.40 / Python 3.13. |
| Emulator test | Earlier starter-app launch and button input were confirmed by the owner. Owner reports the diagnostic harness installs using project-pinned Tool 5.0.40 / Python 3.13 and that the Wakeup test succeeded. Detailed observations were not included, and the harness is no longer buildable from HEAD. |
| Emulator phase-deadline Wakeup (Story 1.1) | Owner confirms the test was run and worked (2026-11-02). The tested revision is unrecorded; the owner did not supply whether the app had left foreground, callback/launch logs or cookie, haptic-request evidence, app-open/foreground observation, emulator version, or measured timing. Physical haptic delivery is not inferred. |
| Emulator timed-pause Wakeup (Story 1.1) | Owner confirms the test was run and worked (2026-11-02). The tested revision is unrecorded; the owner did not supply whether the app had left foreground, callback/launch logs or cookie, haptic-request evidence, app-open/foreground observation, emulator version, or measured timing. Physical haptic delivery is not inferred. |
| Physical phase-deadline and timed-pause Wakeups (Story 1.8) | Not run — finished app/physical target acceptance pending; occurrence, haptic, foregrounding, and timing are unobserved. |
| Physical normal/overdue timing, UTC validity/clock correction, tolerance, Wakeup limits, battery/reliability, UI | Not measured — Story 1.8 acceptance with finished app is pending; tolerance and required repeat count need owner agreement. |

### Sandbox verification (2026-10-31)

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

The local source of the CLI pin is `pyproject.toml` (`pebble-tool==5.0.40`) plus `uv.lock` (resolved 5.0.40); `.python-version` pins Python `3.13.5`; `.pebble-sdk-version` pins SDK `4.33.1` and `scripts/pebble.py` checks that it is active before invoking the locked CLI. No package installation was attempted. The sandbox cannot execute the Pebble commands, so actual SDK selection/path still needs owner build output.

The host-side failures above are this sandbox's own observed results; they do not contradict the owner's separate report of successful locked-CLI build/install and emulator Wakeup testing.

### Current sandbox verification (2026-11-02)

Commands run locally in this sandbox:

| Command | Result |
|---|---|
| `python3 --version` | `Python 3.13.5` (exit 0). |
| `uv --version` | `uv 0.12.3 (aarch64-unknown-linux-gnu)` (exit 0). |
| `command -v pebble`; `command -v pebble-tool`; `command -v pebble-emulator` | No executable found for any; Pebble CLI and emulator are unavailable in this sandbox. |
| `uv lock --check --offline` | Passed (exit 0): lockfile remains consistent. This does not install or execute the CLI. |
| `sh scripts/test.sh` | Passed (exit 0): C session coordinator tests passed and all 4 Python wrapper tests passed. Wrapper tests use stubbed `pebble sdk list` output, including the owner-reported header-line shape, and do not verify actual CLI execution or SDK selection. |

These sandbox checks verify the host-side test suite and lockfile only. The CLI and emulator are unavailable in this sandbox; the owner reports they work on the external host. The owner supplied the version output and emulator invocation noted above, but those do not include SDK path/build records required for AC1–AC2 or the emulator observations required for AC3. No Pebble packages were installed in this sandbox.

**Story 1.1 decision:** In progress, not done. The project pins Pebble Tool 5.0.40, Python 3.13.5, and SDK 4.33.1; the documented wrapper requires 4.33.1 to be active before project build/install commands run. The owner supplied external-host output showing global Pebble Tool 5.1.2 with SDK 4.33.1 active, reports clean/build/install success, and confirms the separate phase-deadline and timed-pause emulator tests were run and worked. Full command output, locked-CLI-selected SDK/toolchain path, artifact identity, tested emulator revision/version, callback/haptic-request/foreground observations, and timing remain to be captured. The supplied emulator invocation is `pebble install --emulator emery --logs`; its result/logs were not included. The emulator reports refer to a harness no longer in the tree and predating the wrapper, so they cannot be reproduced from HEAD without checking out `ef33c2f`. The wrapper's SDK check has stubbed unit tests only and has not been tested against actual output from the project-locked CLI. CLI and emulator are unavailable in this sandbox but reportedly work on the external host. On 2026-11-02, `sh scripts/test.sh` passed the C coordinator and all 4 wrapper unit tests, and `uv lock --check --offline` passed; neither result substitutes for Pebble host/emulator evidence.

**Story 1.8 decision:** Pending. No physical-device acceptance is asserted. Run the finished-app checks after Stories 1.2–1.7, including a complete 45/10/5 cycle and separate background phase/timed-pause Wakeup tests. The project owner reviews battery/reliability evidence; inconclusive or unacceptable results keep the gate open. Do not release until required checks pass or the owner resolves an unsupported requirement.

## Owner-reported locked-CLI output (2026-11-03)

Source: owner-reported, raw output pasted into the session. Not run in this sandbox. Run from the project root against HEAD `fe2830d` or earlier; the exact commit was not stated.

```text
$ uv run --locked pebble --version
Pebble Tool v5.0.40 (active SDK: v4.33.1)

$ uv run --locked pebble sdk list        (run twice, identical output)
Installed SDKs:
4.33.1 (active)

Available SDKs:
4.4
4.5
4.9.127
4.9.148
4.9.169
4.17
```

What this establishes: the project-locked CLI is Pebble Tool 5.0.40 and SDK 4.33.1 is active in the owner's environment, matching `pyproject.toml`, `uv.lock` and `.pebble-sdk-version`. The output shape is `Installed SDKs:` on one line and `4.33.1 (active)` on the next, which `scripts/pebble.py` accepts.

What it does not establish: the SDK/toolchain path a build selects (`build/config.log`), a clean or repeated build/install, artifact identity, or any emulator Wakeup observation. AC1 stays open for the SDK path; AC2 and AC3 are unchanged.

## Owner-reported build configuration (2026-11-03)

Source: owner-reported, contents of `build/config.log` pasted into the session. Not run in this sandbox. The commit built was not stated.

| Item | Value in the log |
|---|---|
| Configured | `Tue Nov 3 11:26:53 2026`, waf 2.1.4 (abi 20) |
| waf location | `~/Library/Application Support/Pebble SDK/SDKs/current/sdk-core/pebble/waf` |
| Toolchain | `~/Library/Application Support/Pebble SDK/SDKs/4.33.1/toolchain/arm-none-eabi/bin` (`arm-none-eabi-gcc` 14.2.1 20241119, `arm-none-eabi-ar`) |
| Moddable tools | `/var/tmp/pebble-sdk/SDKs/4.33.1/toolchain/moddable-tools` |
| Platforms configured | gabbro, flint, emery, diorite, chalk, basalt, aplite (all seven in `package.json`) |
| Pebble tool in PATH | `~/.local/share/uv/tools/pebble-tool/lib/python3.13/site-packages/pebble_tool/commands/sdk` |

What this establishes: the SDK 4.33.1 toolchain path and compiler version a build selected on the owner's machine, and that the full build configures all seven platforms.

What it does not establish: that the build ran under the project-locked Pebble Tool 5.0.40. The PATH entry above is the global uv tool install (`uv/tools/pebble-tool`); a locked run would show the project `.venv` instead. It also does not show that `SDKs/current` points at 4.33.1, and it holds no build, install, exit-code or artifact-hash evidence.

To close this from the host, from the project root at HEAD:

```sh
git rev-parse HEAD
uv run --locked python -c "import pebble_tool; print(pebble_tool.__file__)"
ls -l "$HOME/Library/Application Support/Pebble SDK/SDKs/"
uv run --locked python scripts/pebble.py clean
uv run --locked python scripts/pebble.py build 2>&1 | tee build-1.log; echo "exit=${PIPESTATUS[0]}"
grep -n "pebble_tool/commands/sdk" build/config.log | head -1
shasum -a 256 build/*.pbw
```

Expect the `pebble_tool.__file__` path and the `config.log` PATH line to point into the project `.venv`.

## Artifact hashes observed 2026-11-03 (AC2 artifact identity, not yet a reproducibility result)

Three different SHA-256 values exist for `build/sit_stand_walk.pbw`. None is yet tied to a recorded commit and command sequence, so none establishes or disproves reproducibility.

| Source | Evidence | `sit_stand_walk.pbw` SHA-256 |
|---|---|---|
| Earlier inspection (file) | Existing artifact, no build record | `066e3ee65a5741b9667305d87e8de7317b26063455fee9af39aa5d90b245f180` |
| Owner-reported, pasted twice | `sha256sum build/*.pbw` on the owner's host; which build and commit were not stated | `6026475d41345d8884417f8a61c33b838e3ef90f288c8ab36cc3ed979dc3f464` |
| Local (sandbox, shared workspace mount) | File in `build/`, 122380 bytes, zip entries stamped `11:34` (host local time); `build/config.log` says configured `11:34:13` | `de795b009659c8453abdcf9d39e7e8567b784dd83bb5cbd30c55d840f701959e` |

Local observations of the 11:34 build, taken from the workspace without running Pebble commands: `appinfo.json` hash prefix `8313744f71e15887`; `build/emery/pebble-app.elf` prefix `fa44368f796971e5`; per-platform `pebble-app.bin` hash prefixes `aplite cd9a3ae3a935be36`, `basalt c8d2634b97f5f7c2`, `chalk fb1359e5c1f2caf6`, `diorite 916c397194ff619a`, `emery 32497340a9f1c3fe`, `flint 0304d02cdc9b9daa`, `gabbro 6fabf59a72933d5d`. The archive holds 22 files.

What this does not show:
- The owner's earlier `config.log` said configured `11:26:53`; the workspace log says `11:34:13`. So the owner's pasted log and the workspace build are different runs, and the owner's `6026475d...` does not match the workspace file. Whether it hashes the 11:26 build or another build is unknown.
- A `.pbw` is a ZIP archive with per-entry timestamps, so byte-identical content can still hash differently between builds. That is a hypothesis, not a finding; comparing the per-platform `pebble-app.bin` hashes across two clean builds would test it. This bears on the open owner decision on what counts as artifact identity.
- The `build/config.log` of the 11:34 build still puts `~/.local/share/uv/tools/pebble-tool/.../pebble_tool/commands/sdk` on PATH (14 matches) and no project `.venv`. Whether that proves the global tool ran, rather than the locked one, is unresolved, because Pebble Tool may add its own install directory to PATH. AC1 stays open.
