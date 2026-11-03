---
title: 'Establish Pebble toolchain readiness and emulator evidence'
type: 'feature'
created: '2026-10-23'
status: 'done'
baseline_commit: 'cca3e12d1c48944a2cfd1bb39ae732c2d5dee7c0'
route: 'dispatch'
review_loop_iteration: 0
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

**Closed by owner waiver (2026-11-03):** The owner set Story 1.1 to done. AC1 to AC3 were not met and AC5 says an incomplete story is not marked done, so this is a waiver, not acceptance. Still unrecorded: a build shown running under the project-locked Pebble Tool 5.0.40, complete clean and repeated build/install output with artifact identity, and the emulator Wakeup revision, version, callback, haptic-request, foreground and timing observations. Nothing below was changed. The 2026-11-02 status paragraph is kept as history.

**Current status (2026-11-02):** 2 of 5 acceptance criteria are met; AC1 to AC3 are open. The local C coordinator and four wrapper tests pass, and the uv lockfile check passes. The owner supplied `pebble --version` output from the external host: `Pebble Tool v5.1.2 (active SDK: v4.33.1)`, and identified `pebble install --emulator emery --logs` as the emulator command. The owner confirms the phase-deadline and timed-pause emulator tests were run and worked. AC3 remains open because the tested revision, emulator version, callback/haptic-request/foreground observations, and timing were not supplied. On 2026-11-03 the owner supplied raw `uv run --locked` output showing Pebble Tool v5.0.40 with active SDK v4.33.1, so the locked CLI and active SDK are now evidenced. The owner also pasted `build/config.log`, which gives the 4.33.1 toolchain path, but its PATH points at the global uv tool environment rather than the project-locked one, so AC1 stays open until a build is shown running under the locked CLI. AC2 still needs complete output/artifact identity for clean and repeated builds/installs. The Pebble CLI and emulator are unavailable only inside this sandbox; the owner reports the external host tools and emulator are working. Physical Pebble Time 2 acceptance belongs to Story 1.8. Open owner decisions are listed under Implementation Notes.

<frozen-after-approval reason="human-owned intent — renegotiated by the project owner on 2026-10-31">

## Intent

**Problem:** Implementation needs a pinned, reproducible Pebble toolchain and an available build/install workflow. The exact C SDK package and repeatable build/install evidence are not yet established. Physical-device acceptance requires the finished app and therefore follows Stories 1.2–1.7.

**Approach:** Pin and record the exact CLI, Python, and C SDK package/version; demonstrate a repeatable build/install using the available workflow; record available emulator build/install and separate phase/timed-pause Wakeup observations with their evidence source; and prepare the physical-device acceptance checks for Story 1.8. Report gaps explicitly and do not describe emulator results as physical-device verification.

**Decision:** The owner reports Pebble Time 2 access and emulator testing is available. Story 1.1 is a pre-implementation toolchain-readiness gate only. Physical Pebble Time 2 acceptance with the finished app moves to Story 1.8, after Stories 1.2–1.7 and before release. Emulator evidence supplements toolchain checks but does not prove physical-device capability.

## Boundaries & Constraints

**Always:** Distinguish locally verified checks from owner-reported results, and host/toolchain checks from emulator and physical-watch evidence. Record exact commands and outcomes. Treat emulator phase and timed-pause Wakeups separately and record only observations actually supplied or collected. Carry physical phase/timed-pause Wakeup, haptic, foreground, timing, clock, Wakeup-limit, battery/reliability, and finished-UI acceptance into Story 1.8. Do not claim a physical capability is verified without target-device evidence.

**Never:** Implement session behavior in this story or claim physical-device acceptance passed based on SDK documentation, emulator behavior, or host builds. Do not install packages in the sandbox or invent firmware, SDK, CLI, test results, or owner-provided observations.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Pinned toolchain | Project CLI/Python/C SDK configuration | Exact versions and installation/build commands are recorded | Treat API level as distinct from exact SDK package; leave unresolved pins open |
| Reproducible build/install | Minimal diagnostic app and pinned environment | Clean/repeated build and install outcomes, commands, and artifact identity are recorded | Record owner reports separately from locally executed commands; do not infer reproducibility from existing artifacts |
| Emulator Wakeup evidence | Separate phase and timed-pause test runs | Record available callback, haptic-request, app-open/foreground, and timing observations with provenance | Missing details remain unverified; emulator evidence is not physical evidence |
| Physical acceptance handoff | Finished app on Pebble Time 2 after Stories 1.2–1.7 | Story 1.8 covers device identity, background behaviors, clock/timing/limits, reliability, and UI/control acceptance before release | Not a Story 1.1 execution or pass condition |

</frozen-after-approval>

## Code Map

- `_bmad-output/implementation-artifacts/platform-verification.md` -- local checks, owner-reported outcomes, evidence gaps, and the Story 1.8 handoff.
- `_bmad-output/implementation-artifacts/sprint-status.yaml` -- tracks Story 1.1 and Epic 1 status.
- `pyproject.toml`, `uv.lock`, `.python-version`, `.pebble-sdk-version` -- pins: Pebble Tool 5.0.40, Python 3.13.5, SDK 4.33.1. `pyproject.toml` only requires Python `>=3.13`; `.python-version` fixes the patch version.
- `scripts/pebble.py`, `tests/test_pebble_wrapper.py` -- wrapper that blocks build/install unless the pinned SDK is active, with three stubbed unit tests. The tests have not run against real `pebble sdk list` output.
- `README.md` -- documents the wrapper commands for build and install, and the `clean` command added for AC2.
- `package.json` -- manifest with SDK API level 3, which is not a C SDK package pin. It lists seven target platforms (aplite, basalt, chalk, diorite, emery, flint, gabbro), not only `emery`.
- `src/c/sit_stand_move.c` -- the session app (Stories 1.2 and 1.3). The Wakeup diagnostic harness was added in `818abf5` and replaced in `be85b56`; its last revision is `ef33c2f`.

## Tasks & Acceptance

**Execution:**
- [x] `_bmad-output/implementation-artifacts/platform-verification.md` -- distinguish locally observed environment/build/install attempts from owner-reported results, existing artifacts, and unresolved evidence.
- [x] `.python-version` -- pin the project-selected Python patch version to locally verified 3.13.5.
- [x] `.pebble-sdk-version`, `scripts/pebble.py`, and `README.md` -- pin SDK 4.33.1 separately from the manifest API level and guard project build/install commands against a different active SDK.
- [ ] Project toolchain verification (AC1) -- capture the selected SDK version and path from a clean build under the project-locked environment; the sandbox cannot run Pebble commands.
- [ ] `_bmad-output/implementation-artifacts/platform-verification.md` (AC2) -- capture complete command output and artifact identity for clean and repeated build and install runs using the pinned workflow.
- [ ] `_bmad-output/implementation-artifacts/platform-verification.md` (AC3) -- record separate emulator phase and timed-pause callback, haptic-request, app-open/foreground, and timing observations, identifying owner-reported versus locally verified evidence.
- [x] Carry the physical Pebble Time 2 acceptance scope into the pre-release Story 1.8 handoff; no physical-device result is required or asserted by Story 1.1.
- [x] `_bmad-output/implementation-artifacts/sprint-status.yaml` -- track Story 1.1 and Epic 1 as in progress while readiness evidence is incomplete.

**Acceptance Criteria:**
- [ ] AC1: Given the project-locked toolchain configuration, when CLI, Python, and C SDK versions are inspected in the host environment, then exact CLI/Python pins and active C SDK 4.33.1 are recorded, with `package.json` API level distinguished from the C SDK package and the wrapper blocking commands when another SDK is active.
- [ ] AC2: Given the pinned toolchain and the app at a recorded commit, when a clean build and install are run repeatedly using documented commands, then command output, exit outcomes, target/artifact identity, and successful reproducibility are recorded; any owner-reported outcome is labeled as such and is not presented as a locally verified run.
- [ ] AC3: Given the emulator app build leaves the foreground, when phase and timed-pause Wakeups are tested separately, then available evidence records each Wakeup/callback result, haptic request (not inferred physical delivery), app open/foreground behavior, and observed timing; missing details and the evidence source are explicit.
- [x] AC4: Given physical-device acceptance requires the finished app, when Story 1.1 is handed off, then Pebble Time 2 capability, timing, operational-limit, reliability, and finished-UI checks are assigned to pre-release Story 1.8 after Stories 1.2–1.7, and no physical result is claimed here.
- [x] AC5: Given any readiness evidence remains incomplete, when Story 1.1 status is updated, then the missing evidence is stated and the story remains open; it is not marked done.

### Review Findings

- [x] [Review][Patch] Keep the readiness gate scoped to the explicitly waived stories — Restored the readiness prerequisite for implementation stories after the explicit waiver for Stories 1.2 and 1.3. [_bmad-output/planning-artifacts/epics.md:167]

**Rejected**

- false — AC1–AC3 remain open, but the story accurately lists the evidence gaps, remains `in-progress` rather than `done`, and satisfies AC5’s requirement not to mark incomplete readiness as done.
- false — The verification procedure already names clean/build, repeated runs, output and artifact-hash capture, emulator install/logs, and the platform report’s separate phase/timed-pause procedures; missing run results are explicitly recorded as open evidence.
- false — Existing artifact hashes are labeled as inspected files without build provenance, and the platform report explicitly says they do not establish reproducibility.
- false — The owner’s `pebble sdk list` output is paraphrased as `Installed SDKs: 4.33.1 (active)`; the raw multi-line output is separately stated as not captured.
- false — The architecture’s “Python 3.13” wording is compatible with the exact project pin of 3.13.5 recorded elsewhere.
- false — The wrapper’s real-CLI output remains untested, but this limitation is recorded as outstanding AC1 evidence; it is not represented as verified.
- false — Emulator callback, haptic-request, foreground, timing, and version details remain outstanding under AC3 and are not claimed as verified; the owner's success report is recorded as owner-reported.

## Implementation Notes

**Evidence status.** Each fact carries its source: *local* (run in the sandbox), *owner* (reported, no transcript), or *file* (inspected in the repository).

- *Pins (file).* Pebble Tool 5.0.40 (`pyproject.toml`, `uv.lock`), Python 3.13.5 (`.python-version`), SDK 4.33.1 (`.pebble-sdk-version`, added 2026-11-01). `package.json` declares SDK API level 3, which is not a package pin. The owner's global Tool 5.1.2 failed `pebble install --emulator emery` (`KeyError: 'endpoint'` in `libpebble2`, run under Python 3.15). The owner reports the locked 5.0.40 / Python 3.13 combination works; the effect of each variable was not isolated, so the reason for the pin is not established.
- *SDK (owner, 2026-11-01).* `pebble sdk list` printed `Installed SDKs: 4.33.1 (active)`. This confirms the active SDK in the owner's environment. It does not confirm the SDK the locked CLI selects. The raw multi-line output was not captured.
- *External host tools (owner, 2026-11-02).* The owner supplied `pebble --version` output: `Pebble Tool v5.1.2 (active SDK: v4.33.1)`, and reports the external build tools and emulator are working. The owner identified `pebble install --emulator emery --logs` as the emulator invocation. This is global Tool 5.1.2, not evidence from the project-locked Tool 5.0.40, and no command result or emulator log was included in the message.
- *Project-locked CLI and SDK (owner, 2026-11-03, raw output pasted).* `uv run --locked pebble --version` printed `Pebble Tool v5.0.40 (active SDK: v4.33.1)`. `uv run --locked pebble sdk list` printed `Installed SDKs:` then `4.33.1 (active)`, followed by an Available SDKs list (4.4, 4.5, 4.9.127, 4.9.148, 4.9.169, 4.17; the paste may be cut off). It was run twice with identical output. This is the first evidence from the project-locked Tool 5.0.40 rather than the global 5.1.2. It confirms the pinned Tool and the active SDK. It does not give the SDK path a build selects, which AC1 also needs.
- *Build configuration log (owner, 2026-11-03, `build/config.log` pasted).* Configured `Tue Nov 3 11:26:53 2026` by waf 2.1.4 (abi 20) from `~/Library/Application Support/Pebble SDK/SDKs/current/sdk-core/pebble/waf`. The compiler and archiver are `arm-none-eabi-gcc` 14.2.1 20241119 and `arm-none-eabi-ar`, found under `~/Library/Application Support/Pebble SDK/SDKs/4.33.1/toolchain/`. SDK platform directories were found for gabbro, flint, emery, diorite, chalk, basalt and aplite, so all seven manifest targets were configured. This supplies the SDK/toolchain path for 4.33.1. Two gaps: (1) the search PATH begins with `~/.local/share/uv/tools/pebble-tool/lib/python3.13/site-packages/pebble_tool/commands/sdk`, which is the globally installed uv tool environment, not the project-locked environment (`uv run --locked` would use the project's `.venv`). So this log does not show the build ran under the locked Tool 5.0.40. (2) `SDKs/current` is the path used to find waf, and the log does not show that `current` points to 4.33.1 (the toolchain paths do say 4.33.1). The source commit built was not stated, and the log was pasted from its tail with the start cut off.
- *Artifact hashes (owner and local, 2026-11-03).* The owner pasted `6026475d41345d8884417f8a61c33b838e3ef90f288c8ab36cc3ed979dc3f464` for `build/sit_stand_walk.pbw` (twice). The sandbox sees a different file there (`de795b009659c8453abdcf9d39e7e8567b784dd83bb5cbd30c55d840f701959e`, config log `11:34:13`), and the earlier recorded hash is a third value. These are different runs or files, so no reproducibility is claimed; details and the unresolved questions are in `platform-verification.md`.
- *Wrapper (file, 2026-11-01).* `scripts/pebble.py` runs `pebble sdk list` through the locked CLI and runs the requested command only if a line equals `4.33.1 (active)`. The three unit tests use stubbed output. The wrapper itself has not been run against a real CLI in any recorded environment. The owner's real `sdk list` output (see above) has `4.33.1 (active)` alone on its own line, which the guard's first accepted shape matches and a unit test already covers.
- *Build and install (owner).* The owner reports that the clean, build, and install commands all succeeded with SDK 4.33.1 under the locked CLI. Transcripts, exit codes, artifact identity, and the selected SDK path were not supplied.
- *Emulator Wakeup (owner, 2026-11-02).* The owner confirms separate phase-deadline and timed-pause emulator tests were run and worked. The tested revision, emulator version, callback logs, haptic-request evidence, foreground state, and timing were not supplied. That diagnostic harness no longer exists at HEAD (see Code Map), and which revision the owner tested is not recorded; these gaps keep AC3 open.
- *Existing artifacts (file).* `build/emery/pebble-app.elf`, `build/sit_stand_go.pbw`, and `build/sit_stand_walk.pbw` exist, and `build/config.log` records Waf 2.1.4 and SDK paths including 4.33.1. None shows that the current source builds reproducibly. Hashes are in `platform-verification.md`.
- *Sandbox (local, 2026-10-31).* `python3` is 3.13.5 and `uv` is 0.12.3. `pebble`, `pebble-tool`, and `pebble-emulator` are absent, so Pebble commands fail with `command not found`. `uv lock --check --offline` passes. `uv run --locked pebble --version` failed because uv could not download `greenlet==3.5.6` through the proxy. Full command outcomes are in `platform-verification.md`.

**Open owner decisions.** No value is invented here for any of these.

- Minimum number of clean build and install repeats that counts as "repeated".
- Definition of artifact identity (for example platform, app UUID, version, and sha256) and how differing `.pbw` hashes between runs are treated.
- Which revision the emulator Wakeup evidence applies to, given the harness was replaced.
- Whether `targetPlatforms` should be limited to `emery`; a full build compiles all seven.
- Closure rule: who runs the commands, and whether an item the owner cannot evidence blocks the story, is waived explicitly, or moves to Story 1.8.
- Timing tolerance and repeat count for Story 1.8, to be fixed before its timing tests start.

**Gate waiver.** Epics says Story 1.1 must be complete before implementation stories begin. The Story 1.2 spec records the owner's decision to proceed while 1.1 stays open, and Story 1.3 followed. The Epics dependency wording has not been updated to match.

## Spec Change Log

- 2026-10-31 — Project owner explicitly renegotiated frozen intent: Story 1.1 now gates pre-implementation pinned toolchain/reproducible build readiness and available emulator evidence; finished-app Pebble Time 2 acceptance moves to pre-release Story 1.8 after Stories 1.2–1.7. No physical capability is claimed.
- 2026-11-01 — Review follow-up (non-frozen sections only; the frozen block is unchanged). Renamed the story file and sprint key from `1-1-verify-pebble-time-2-reminder-capabilities` to `1-1-establish-pebble-toolchain-and-emulator-readiness`. Reworded AC2 from "diagnostic app" to "the app at a recorded commit", because the diagnostic harness no longer exists at HEAD. Recorded the owner's waiver that let Stories 1.2 and 1.3 proceed. AC numbers were added; criteria text is otherwise unchanged.

## Review Triage Log

### Earlier reviews (before the 2026-10-31 renegotiation)

- Resolved or false: evidence-report tasks were narrowed to describe the report, clock and Wakeup-limit outcomes were added to the acceptance wording, callback timing was separated from app-presentation timing, and suggestions to add test duration, cycle counts, or a repeat count were rejected as expanding the frozen intent or as already covered.
- Open: no numeric timing tolerance or repeat count exists. The owner must agree both before Story 1.8 timing tests; no value is invented here.
- Entries that described the physical-device gate are superseded by the 2026-10-31 scope change.
- Superseded 2026-11-01 — the entries restoring sprint tracking to backlog (status has been in-progress since the renegotiation) and deferring Code Map drift (Code Map corrected; `deferred-work.md` holds only the SDK-path entry) are removed from the history above.

### Review 2026-11-01 (adversarial, edge-case, structure, prose)

- medium — The emulator Wakeup evidence points to a harness that no longer exists at HEAD (`818abf5` to `ef33c2f`). Recorded in the Code Map and Evidence status; which revision was tested stays an owner decision.
- medium — Status narrative sat inside Tasks and ACs, and the sprint and Code Map text contradicted the in-progress status. Moved the narrative into a status summary and Evidence status, and gave the ACs numbers.
- medium — The Code Map said the manifest targets `emery`, but `package.json` lists seven platforms. Corrected; limiting platforms is an owner decision.
- medium — Notes contradicted themselves on whether the SDK is pinned, and repeated facts three times. Merged into one Evidence status block with provenance labels.
- low — Wording: "clean/build/install success", "SDK selected/version/path", and "actively being completed and incomplete". Reworded.
- maybe-false — The guard's exact-line match may not fit real `pebble sdk list` output. It is most likely one SDK per line, which the guard handles. Settle by capturing the raw output (AC1).
- false — "The wrapper has no test". `tests/test_pebble_wrapper.py` has three stubbed tests that pass; the gap is real CLI output, not a missing test.
- false — "The story title is mismatched with the epic". Title now matches in substance; the slug was the mismatch and is renamed.
- deferred to owner — Run count, artifact identity, closure rule, Story 1.8 tolerance, and platform scope (listed under Implementation Notes). They change the frozen intent or are product decisions.

### Review 2026-11-02 (blind content, edge-case, verification-gap)

- patch — `scripts/pebble.py` matched only a standalone SDK row, while owner-reported output can place the active SDK on `Installed SDKs:`. The guard now accepts both observed line shapes; added a wrapper test for the header-line format. `python3 -m unittest tests.test_pebble_wrapper` passes (4 tests).
- patch — Epic Story 1.1 still named a minimal diagnostic app after the diagnostic harness had been replaced. Updated its acceptance wording to the app at a recorded commit, consistent with this story.
- patch — Epic 1.2 dependency wording did not record the owner's waiver to proceed while Story 1.1 remains open. Recorded the 2026-10-31 authorization and clarified it does not close Story 1.1.
- patch — Architecture still described the C SDK pin as unresolved. Updated the Stack and Story 1.1 deferred-work entry to record project pin 4.33.1 and distinguish the missing locked-build SDK path and reproducibility evidence.
- patch — Platform verification table labeled the emulator install as resolved by the locked CLI without making clear this is owner-reported and not independently verified. Corrected the provenance label.
- defer — `src/c/session_coordinator.c` can retain a consumed Wakeup identity when early-callback repair scheduling or persistence fails. Failure is surfaced, but a reliable retry/recovery policy is not specified by this story; implementation belongs with coordinator behavior and should be decided before release.
- defer — `prv_valid_utc()` accepts every positive timestamp. A minimum plausible UTC boundary is not specified in the story or architecture; product/clock policy is needed before changing behavior.
- defer — The bounded coordinator queue drops events when full, including Stop/Wakeup events. Queue-pressure policy is a pre-existing coordinator design issue requiring a safe coalescing/reservation decision.
- defer — The fixed-height error text layer may not fit all visible errors on the watch. Verify message legibility/layout on Pebble Time 2 in Story 1.8.
- defer — The active menu includes Skip before Story 1.6 implements it. Story 1.2 explicitly established the menu surface and deferred the action; wire it in Story 1.6.
- defer — The native Pebble MenuLayer callback is not exercised by host tests; Story 1.2 already records this verification limitation and assigns actual UI behavior to the SDK/device pass.
- defer to owner — The owner has not decided whether repeatable build evidence covers all seven manifest platforms or only `emery`; the decision remains listed under Implementation Notes.
- defer to owner — The exact revision used for owner-reported emulator Wakeup tests is still unknown; this is already recorded in Implementation Notes and platform verification and requires host evidence.
- false — The active Skip row was reported as a silent no-op. Story 1.2 explicitly deferred Skip behavior to Story 1.6 while establishing the menu surface, so adding feedback or dispatch here would expand that story's scope.
- defer — Verification-gap review found native MenuLayer selection is not covered by host tests. Existing coordinator tests and source inspection are insufficient to exercise the SDK callback; record a native SDK/device test with Story 1.8 acceptance.

## Verification

**Commands (to run on a host with the Pebble CLI; the sandbox cannot):**
- `uv run --locked pebble --version` and `uv run --locked pebble sdk list` -- record raw output.
- `uv run --locked python scripts/pebble.py clean`, then `build`, with output and exit code captured. Record the SDK lines from `build/config.log` and `sha256sum build/*.pbw`. Repeat for the agreed run count.
- `uv run --locked python scripts/pebble.py install --emulator emery`, then `uv run --locked pebble logs`, recording the emulator version.
- `sh scripts/test.sh` -- host-side tests; they do not exercise the Pebble SDK.

**Locally run 2026-11-02:** `sh scripts/test.sh` passed (C coordinator and 4 wrapper tests); `uv lock --check --offline` passed. `python3 --version` returned 3.13.5 and `uv --version` returned 0.12.3. `pebble`, `pebble-tool`, and `pebble-emulator` are unavailable in this sandbox, so host SDK/build/install and emulator commands were not run here. The owner reports those tools work outside the sandbox; supplied host evidence is recorded in `platform-verification.md`.

**Manual checks:** Verify every result is labeled locally observed, owner-reported, or untested. Confirm emulator evidence is not presented as physical-device acceptance and that physical tests are assigned to Story 1.8. Haptic evidence from the emulator is a logged API call, not observed vibration.
