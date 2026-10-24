---
title: 'Verify Pebble Time 2 reminder capabilities'
type: 'feature'
created: '2026-10-23'
status: 'in-progress'
baseline_commit: 'b6bff4f162d9faf463926afea3fce6a71b4ec59e'
route: 'dispatch'
review_loop_iteration: 0
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Implementation depends on Pebble Time 2 firmware supporting background Wakeups that can request haptics and open or foreground the app, but the target firmware and exact reproducible toolchain have not yet been verified.

**Approach:** Establish and record the exact target and pinned CLI/SDK versions, use the RePebble CLI/emulator setup for reproducible host-side build checks where applicable, and execute a focused physical-device acceptance plan on the available Pebble Time 2. Report unsupported or untested behaviors explicitly and keep dependent reminder behavior gated until the product owner decides how to proceed.

**Decision:** Pebble Time 2 hardware is available, and an emulator is also available. Emulator evidence supplements host/toolchain checks but does not substitute for physical-device capability verification.

## Boundaries & Constraints

**Always:** Distinguish host/emulator checks from physical-watch evidence. The owner confirms Pebble Time 2 access and an emulator is available. Record commands and observed results; treat phase and timed-pause Wakeups separately. Measure ordinary transition timing and document an agreed tolerance, Wakeup limits/spacing, and battery/reliability observations. Test after leaving the app in the background; explicit app termination is out of scope. Do not claim a capability is verified without target-device evidence.

**Never:** Implement session behavior or claim this platform gate passed based only on SDK documentation, emulator behavior, or host builds. Do not install packages in the sandbox or invent firmware, SDK, CLI, test results, or timing tolerance.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Toolchain available | Pinned CLI/SDK and minimal app | Reproducible build/install commands and results recorded | Record exact failing command and block dependent implementation |
| Wakeup after leaving app | Separate phase and timed-pause deadlines | For each, record delivery, haptic request/delivery, app open/foreground, and observed timing | Mark unsupported or unverified; do not infer success from the other Wakeup type |
| Clock/deadline checks | Normal and overdue deadline, invalid UTC, forward/backward adjustment | Record observable outcomes and measured normal-transition tolerance | Record any untested condition as a gate, not a pass |
| Device limits | Repeated reminders at supported spacing | Record Wakeup count/spacing constraints and battery/reliability observations | Document limit or instability and leave dependent behavior gated |

</frozen-after-approval>

## Code Map

- `_bmad-output/implementation-artifacts/sprint-status.yaml` -- Story 1.1 is the first backlog story; its status must reflect evidence, not just a prepared test plan.
- `_bmad-output/planning-artifacts/epics.md` -- Story 1.1 acceptance criteria define version recording, reproducible build/install, background Wakeup/haptic/foreground tests, timing tolerance, and device limits.
- `_bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` -- documented CLI/Python baseline, Wakeup design constraints, and physical-device gate; exact SDK package remains unresolved there.
- Repository has no app source, Pebble manifest, lock data, or existing device test report. `pebble`/`pebble-tool` are not available on PATH; Python 3.13.5 is available. The user-provided RePebble SDK documentation URL returned HTTP 403 from this environment, so its CLI/emulator installation instructions remain to be verified. Do not treat this environment as a completed build/install test.

## Tasks & Acceptance

**Execution:**
- [x] `_bmad-output/implementation-artifacts/platform-verification.md` -- create an evidence report distinguishing recorded environment facts, unavailable measurements, and unresolved gates -- provide an auditable basis for the pre-implementation decision.
- [ ] `_bmad-output/implementation-artifacts/platform-verification.md` -- record actual minimal-app build/install outcomes and separate phase/timed-pause Wakeup observations from emulator and physical target -- make reproducibility and device evidence explicit.
- [x] `_bmad-output/implementation-artifacts/sprint-status.yaml` -- keep Story 1.1 and Epic 1 at backlog while required physical-device acceptance evidence is unavailable; advance status only when the acceptance evidence is complete.

**Acceptance Criteria:**
- Given a target watch and selected toolchain, when verification is reported, then the exact Pebble Time 2 firmware/OS and pinned CLI/SDK versions are recorded.
- Given the pinned toolchain and minimal test app, when the documented procedure is run, then exact build/install commands and outcomes demonstrate reproducibility on the target or record a concrete blocker.
- Given the app has left the foreground, when phase-deadline and timed-pause Wakeups are tested separately, then each result records Wakeup occurrence, haptic behavior, app open/foreground behavior, and observed timing.
- Given target-device timing and repeated-Wakeup tests, when the report is finalized, then normal and overdue deadline results are recorded; invalid UTC leaves Start inactive with a visible error and preserves active committed state while suppressing scheduling/haptics; forward clock correction makes a reached deadline due and backward correction defers it; no more than eight Wakeups are scheduled and the one-minute exclusion window is respected; an owner-agreed measurable transition tolerance and the observations supporting it are recorded, with untested or failed criteria clearly gated.
- Given any required capability is unsupported or remains unverified, when the report is reviewed, then the limitation is explicit and Story 1.1 is not marked done for the platform gate.

## Implementation Notes

Environment checks found Python 3.13.5 and uv 0.12.3, but no `pebble`, `pebble-tool`, or `pebble-emulator` executable and no installed `pebble-tool` Python package. The RePebble SDK URL returned HTTP 403. No project app source or manifest exists. Recorded these facts and a host/emulator plus physical-watch test plan in `platform-verification.md`; no install/build/device tests were run. Story remains blocked at its device/toolchain gate. Before scheduling tests, the owner must agree a measurable transition tolerance and what repeated observations are sufficient to accept it.

## Spec Change Log


## Review Triage Log

- medium — The evidence-report creation tasks were marked complete despite requiring target-specific facts and test outcomes absent from the report. Narrowed the task to accurately describe the created blocker/evidence report, and left actual build/device results unchecked. Evidence: the report states CLI/SDK pins, build/install, emulator, physical Wakeup, timing, and battery checks are unavailable or not run.
- medium — Sprint tracking was advanced although the story explicitly requires retaining backlog/blocked when physical-device checks cannot be performed. Restored Story 1.1 and Epic 1 to backlog until the target acceptance evidence is available. Evidence: no target device is connected to this environment and no CLI/emulator is installed.
- medium — Acceptance described clock and Wakeup-boundary checks without explicit expected outcomes. Added the already-specified clock policy and documented eight-Wakeup/one-minute limits to the acceptance wording and report checklist.
- maybe-false — Acceptance does not set a numeric timing tolerance or number of repeats. Product owner agreement is needed before tests can yield a pass/fail result; no value is invented here.

## Verification

**Manual checks (if no CLI):** Confirm report values are sourced from the physical target or exact tool output; confirm no untested firmware-dependent behavior is represented as verified.

**Environment checks run:** `python3.13 --version`, `uv --version`, `command -v pebble`, `command -v pebble-tool`, `command -v pebble-emulator`, `python3.13 -m pip show pebble-tool`, and the RePebble documentation fetch. Results and limitations are recorded in `platform-verification.md`.
