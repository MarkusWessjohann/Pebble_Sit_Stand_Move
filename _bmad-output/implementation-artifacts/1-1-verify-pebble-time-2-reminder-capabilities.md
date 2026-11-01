---
title: 'Establish Pebble toolchain readiness and emulator evidence'
type: 'feature'
created: '2026-10-23'
status: 'in-progress'
baseline_commit: 'cca3e12d1c48944a2cfd1bb39ae732c2d5dee7c0'
route: 'dispatch'
review_loop_iteration: 0
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

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

- `_bmad-output/implementation-artifacts/platform-verification.md` -- records local checks, owner-reported CLI/build/install/emulator outcomes, evidence gaps, and the physical acceptance checklist.
- `_bmad-output/implementation-artifacts/sprint-status.yaml` -- tracks Story 1.1 as in progress until its narrowed toolchain/readiness criteria are evidenced; this story does not pass physical acceptance.
- `pyproject.toml`, `uv.lock`, and `.python-version` -- Pebble Tool 5.0.40 and Python 3.13.5 pins; SDK 4.33.1 is owner-confirmed active, but not pinned/verified by the project-locked configuration.
- `package.json` -- Pebble manifest with target `emery` and SDK API level 3; API level is not an exact C SDK package pin.
- `src/c/sit_stand_move.c` -- current diagnostic harness, not the session implementation.

## Tasks & Acceptance

**Execution:**
- [x] `_bmad-output/implementation-artifacts/platform-verification.md` -- distinguish locally observed environment/build/install attempts from owner-reported results, existing artifacts, and unresolved evidence.
- [x] `.python-version` -- pin the project-selected Python patch version to locally verified 3.13.5.
- [ ] Project toolchain configuration -- pin the owner-confirmed Pebble C SDK 4.33.1 alongside the CLI/Python pins and capture its selected version/path using the project-locked environment.
- [ ] `_bmad-output/implementation-artifacts/platform-verification.md` -- capture complete command output and artifact identity for clean/repeated build and install runs using the pinned workflow.
- [ ] `_bmad-output/implementation-artifacts/platform-verification.md` -- record separate emulator phase and timed-pause callback, haptic-request, app-open/foreground, and timing observations, identifying owner-reported versus locally verified evidence. The unsupplied callback/foreground/haptic/timing details and owner-report provenance are listed, but observations remain incomplete.
- [x] Carry the physical Pebble Time 2 acceptance scope into the pre-release Story 1.8 handoff; no physical-device result is required or asserted by Story 1.1.
- [x] `_bmad-output/implementation-artifacts/sprint-status.yaml` -- track Story 1.1 and Epic 1 as in progress while readiness evidence is actively being completed and incomplete.

**Acceptance Criteria:**
- [ ] Given the project-locked toolchain configuration, when CLI, Python, and C SDK versions are inspected, then exact CLI/Python pins and C SDK 4.33.1 selection are recorded, with the Pebble manifest API level distinguished from the SDK package.
- [ ] Given the pinned toolchain and diagnostic app, when a clean build and install are run repeatedly using documented commands, then command output, exit outcomes, target/artifact identity, and successful reproducibility are recorded; any owner-reported outcome is labeled as such and is not presented as a locally verified run.
- [ ] Given the diagnostic app is installed in the emulator and leaves the foreground, when phase and timed-pause Wakeups are tested separately, then available evidence records each Wakeup/callback result, haptic request (not inferred physical delivery), app open/foreground behavior, and observed timing; missing details and the evidence source are explicit. Owner-reported test success is recorded, but foreground state, callback/log details, haptic-request evidence, and timing were not supplied.
- [x] Given physical-device acceptance requires the finished app, when Story 1.1 is handed off, then Pebble Time 2 capability, timing, operational-limit, reliability, and finished-UI checks are assigned to pre-release Story 1.8 after Stories 1.2–1.7, and no physical result is claimed here.
- [x] Given any readiness evidence remains incomplete, when Story 1.1 status is updated, then the missing evidence is stated and the story remains open; it is not marked done.

## Implementation Notes

The repository has a diagnostic Pebble app harness targeting `emery`; `package.json` declares SDK API level 3, not an exact C SDK package. `pyproject.toml`/`uv.lock` pin Pebble Tool 5.0.40 and `.python-version` pins Python 3.13.5. The owner-supplied `pebble sdk list` output confirms SDK 4.33.1 is installed and active in the owner's environment, but the project does not pin it or establish that the locked CLI selects it. Existing artifacts include an `emery` ARM ELF and `.pbw`, and `build/config.log` records Waf 2.1.4 and host SDK/toolchain paths including 4.33.1; neither proves the current source builds reproducibly. The owner reports that a project-locked Tool 5.0.40 / Python 3.13 build and emulator install succeeded and that separate phase and timed-pause emulator Wakeup tests succeeded, but complete command output, repeated clean-build evidence, callback details, foreground behavior, haptic-request observations, and timing were not supplied. Locally, Python 3.13.5 and uv 0.12.3 are available; `pebble`, `pebble-tool`, and `pebble-emulator` are absent, so local `pebble --version`, `pebble build`, and `pebble install --emulator emery` fail before execution with `command not found`. `uv lock --check --offline` passes. These local checks and owner-reported results are distinguished in `platform-verification.md`. Physical acceptance is deferred to Story 1.8 with the finished app after Stories 1.2–1.7; no physical tests have been run. Timing tolerance and repeat count remain owner decisions for Story 1.8. The readiness criteria remain incomplete, so this story and Epic 1 are in progress, not done.

2026-11-01 owner evidence update: the owner supplied output from `pebble sdk list` showing SDK `4.33.1 (active)`. This establishes the active installed SDK in the owner's CLI environment, but not a project SDK pin or the SDK selected by the project-locked CLI. The clean/repeated build/install evidence and detailed emulator observations remain outstanding.

## Spec Change Log

- 2026-10-31 — Project owner explicitly renegotiated frozen intent: Story 1.1 now gates pre-implementation pinned toolchain/reproducible build readiness and available emulator evidence; finished-app Pebble Time 2 acceptance moves to pre-release Story 1.8 after Stories 1.2–1.7. No physical capability is claimed.

## Review Triage Log

- medium — The evidence-report creation tasks were marked complete despite requiring target-specific facts and test outcomes absent from the report. Narrowed the task to accurately describe the created blocker/evidence report, and left actual build/device results unchecked. Evidence: the report states CLI/SDK pins, build/install, emulator, physical Wakeup, timing, and battery checks are unavailable or not run.
- medium — Sprint tracking was advanced although the story explicitly requires retaining backlog/blocked when physical-device checks cannot be performed. Restored Story 1.1 and Epic 1 to backlog until the target acceptance evidence is available. Evidence: no target device is connected to this environment and no CLI/emulator is installed.
- medium — Acceptance described clock and Wakeup-boundary checks without explicit expected outcomes. Added the already-specified clock policy and documented eight-Wakeup/one-minute limits to the acceptance wording and report checklist.
- maybe-false — Acceptance does not set a numeric timing tolerance or number of repeats. Product owner agreement is needed before tests can yield a pass/fail result; no value is invented here.
- deferred — The Code Map says the repository lacks app source and a Pebble manifest, but `src/c/sit_stand_move.c` and `package.json` exist. This inventory drift predates the report changes and was already recorded in `deferred-work.md`.
- false — The suggested repeat-count acceptance addition edits this build's spec, which the review workflow excludes; the story already records that the owner must agree a sufficient repeat count before timing tests.
- false — The suggested one-minute-window clarification is already established in architecture as a spacing/exclusion constraint between scheduled wakeups and is used consistently in the report.
- false — The report asks to record boundary behavior and explicitly documents the eight-event maximum and one-minute spacing; the unrun target tests are clearly marked as blocked.
- medium — The report distinguishes scheduled-deadline-to-Wakeup-callback delay from scheduled-deadline-to-app-open/foreground delay and records haptic request/delivery separately. Evidence: the prior wording did not distinguish callback timing from app presentation timing.
- false — Adding test duration or reminder-cycle-count criteria would expand the frozen test intent; the report transparently leaves battery/reliability observations unmeasured pending physical-device testing.
- false — The report explicitly labels firmware and CLI/SDK pins blocked and the platform gate not passed, so the missing version evidence is accurately disclosed rather than falsely claimed as complete.
- Historical triage entries above describe the pre-renegotiation physical-device gate and are superseded where they conflict with the 2026-10-31 owner-approved scope change.

## Verification

**Manual checks (if no CLI):** Verify every result is identified as locally observed, owner-reported, or untested; confirm emulator evidence is not represented as physical-device acceptance and that physical tests are assigned to Story 1.8.

**Environment checks run:** `python3 --version`, `uv --version`, `command -v pebble`, `command -v pebble-tool`, `command -v pebble-emulator`, `python3 -m pip show pebble-tool`, `uv lock --check --offline`, `uv run --locked pebble --version`, `pebble --version`, `pebble build`, `pebble install --emulator emery`, `file build/emery/* build/sit_stand_go.pbw build/sit_stand_walk.pbw`, and `sha256sum build/emery/pebble-app.elf build/sit_stand_go.pbw build/sit_stand_walk.pbw`. Existing artifacts, `build/config.log`, `.python-version`, `pyproject.toml`, `uv.lock`, `package.json`, and the diagnostic harness were inspected. Exact outcomes and provenance are recorded in `platform-verification.md`.

**Owner-supplied SDK evidence:** `pebble sdk list` — `Installed SDKs: 4.33.1 (active)` (2026-11-01); confirms active SDK in the owner's environment, not project pin/locked selection.
