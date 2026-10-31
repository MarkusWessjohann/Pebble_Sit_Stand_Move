# Sprint Change Proposal — Move Pebble Time 2 Acceptance to Release Readiness

**Date:** 2026-10-31
**Project:** sit_stand_go
**Trigger:** Story 1.1, Verify Pebble Time 2 reminder capabilities
**Status:** Partially approved; implementation halted pending Story 1.1 intent resolution

## 1. Issue Summary

The current plan makes physical Pebble Time 2 acceptance a prerequisite to starting implementation. The owner clarified that physical-watch testing will happen once the app is complete. This creates a sequencing mismatch: the device-specific acceptance requires the completed app, while the PRD, Architecture Spine, Epic 1, Story 1.1, and Epic context currently gate implementation on that acceptance.

Evidence: PRD §0 and §5.1, Architecture AD-3 and Deferred items, Epic NFR3/NFR9 and Story 1.1, and `epic-1-context.md` explicitly place physical-device verification before implementation. The current platform report records an owner-reported successful locked-CLI emulator build/install and Wakeup test, but exact SDK selection and detailed test observations remain incomplete; physical-device results do not yet exist.

## 2. Impact Analysis

### Epic impact

Epic 1 remains viable with the same product scope. Resequence the work so toolchain readiness precedes implementation, Stories 1.2–1.7 deliver the app, and a new Story 1.8 performs physical-device operational and UX acceptance with the finished app before release.

### Story impact

- **Story 1.1:** Narrow to reproducible CLI/SDK/toolchain setup, emulator build/install evidence, emulator observations, and preparation of the target acceptance plan. It no longer claims to prove target firmware feasibility.
- **Stories 1.2–1.7:** May proceed after the exact build toolchain is pinned and the build/install workflow is reproducible. Their implementation must preserve existing API contracts and visibly handle runtime persistence, scheduling, and clock failures; unverified physical behavior must not be described as proven.
- **New Story 1.8:** Verify the finished app on Pebble Time 2, covering firmware/toolchain identity, background phase and timed-pause Wakeups, haptic and foreground behavior, timing tolerance, overdue/clock cases, device limits, battery/reliability, and display/control legibility. This story is a release gate.

### Artifact conflicts and updates

- **PRD:** Amend Build Readiness, §5.1–5.2, MVP framing, relevant success metric wording, and open-question ownership so reproducible toolchain readiness is pre-implementation and physical acceptance is pre-release.
- **Product brief:** Update statements in the summary, solution, and open items that say device testing must occur before implementation.
- **Architecture Spine:** Update AD-3 and the Deferred sections so target verification is an explicit pre-release acceptance gate, while preserving the designed Wakeup/state/failure contracts and requiring the exact build toolchain before implementation.
- **Epics:** Update NFR3/NFR9, Epic 1 descriptions, Story 1.1 intent and criteria, and Story 1.3's dependency. Add Story 1.8 with the moved device-acceptance criteria.
- **UX:** Add a short note to DESIGN and EXPERIENCE that physical layout, controls, legibility, and firmware-dependent behavior are verified with the finished app before release. UX requirements and scope otherwise stay unchanged.
- **Implementation context and tracking:** Regenerate `epic-1-context.md`; add Story 1.8 to `sprint-status.yaml`. Keep Story 1.1 open until its narrowed toolchain evidence is complete; do not mark physical acceptance done in advance.
- **Platform evidence:** Reorganize `platform-verification.md` to distinguish pre-implementation toolchain/emulator evidence from the pending Story 1.8 target acceptance.

### Technical and MVP impact

No feature is removed and the MVP goal is unchanged. Implementation will temporarily rely on the documented API contract and emulator evidence; this carries uncertainty that the target firmware may not support all requested background behavior. Release remains blocked until Story 1.8 demonstrates support or the owner explicitly revises affected product scope after a failed check.

## 3. Recommended Approach

**Recommendation: Direct adjustment to the existing Epic 1 plan.** Keep the feature scope, split readiness from acceptance, and add the target-device acceptance story at the end of Epic 1.

- **Effort:** Medium — reconcile several planning artifacts and add one test/acceptance story.
- **Risk:** Medium — implementation can proceed sooner, but hardware-specific behavior remains uncertain until the complete app is tested.
- **Timeline:** Removes the physical-watch test as a start-of-development blocker; retains it as a hard pre-release gate. No calendar estimate is available.
- **Rollback:** Not recommended; there is no implementation to revert, and the requirements remain useful.
- **MVP review:** Not required unless Story 1.8 finds a required capability unsupported; then revisit the affected feature before release.

## 4. Detailed Change Proposals

The user approved these proposal directions incrementally on 2026-10-31; approval of this complete proposal and implementation is still pending.

### PRD

**Sections:** §0 Build Readiness; §5.1 Platform and background behavior; §5.2 Timing and reliability; §7 MVP scope; §8 success metrics; §9 open questions.

**Before:** Physical firmware/Wakeup/haptic/foreground verification is required “before implementation begins.”

**After:** Pin the exact CLI/SDK and demonstrate a reproducible build/install using the available workflow before implementation. Perform physical Pebble Time 2 acceptance with the finished app before release. Until then, implementation follows documented APIs and emulator evidence, while target behavior remains unverified. If a required capability fails, resolve the product impact before release.

**Rationale:** Aligns the gate with when the complete behavior can actually be tested without claiming device support early.

### Product brief

**Sections:** Executive Summary, Solution, and Open Questions.

**Before:** Firmware must be tested before implementation.

**After:** Establish reproducible toolchain readiness before implementation; perform physical behavior verification with the finished app before release. Keep the existing conditional revisit decision if timed-pause foreground/haptic support fails.

**Rationale:** The brief must not contradict the approved PRD sequence.

### Architecture

**Sections:** AD-3, Stack, Deferred acceptance/toolchain items, timing tolerance, packaging.

**Before:** Verify Wakeup support and notification behavior on target firmware before implementation; pin exact SDK and verify target build/install before implementation.

**After:** Exact CLI/SDK pin and reproducible build/install are pre-implementation prerequisites. Physical Wakeup, haptic, foreground, timing, operational-limit, and finished-UI verification are pre-release acceptance. Preserve API semantics, failure handling, and the requirement not to claim unverified behavior.

**Rationale:** Toolchain feasibility remains a genuine implementation prerequisite; device acceptance is scheduled when the product is testable.

### Epics and stories

**Requirements:** NFR3/NFR9; Epic 1 description; Story 1.1; Story 1.3 dependency; new Story 1.8.

**Before:** Target-device capability checks are a readiness gate for implementation and Story 1.1 blocks dependent reminder work.

**After:** Story 1.1 establishes pinned and reproducible CLI/SDK build/install plus available emulator evidence and prepares the physical acceptance plan. Stories 1.2–1.7 implement the product. Story 1.8 tests the finished app on physical hardware and blocks release until required results pass or the owner resolves a failure.

**Rationale:** Preserves the acceptance checks but orders them after the app exists.

### UX

**Files:** DESIGN.md and EXPERIENCE.md.

**Before:** Legibility, controls, and platform behavior require target verification, with timing not explicit.

**After:** Add that these are tested on Pebble Time 2 with the finished app as part of pre-release acceptance.

**Rationale:** Clarifies sequencing only; keeps all UX requirements.

### Implementation tracking and evidence

**Files:** `epic-1-context.md`, `sprint-status.yaml`, `platform-verification.md`.

**Before:** Story 1.1 and its context treat physical acceptance as a pre-implementation blocker.

**After:** Regenerate the context around the new dependency; track Story 1.8; retain Story 1.1 as open until exact toolchain and reproducibility records are complete; clearly separate emulator evidence and physical acceptance.

**Rationale:** Prevents stale planning state and false completion.

## 5. Implementation Handoff

**Scope classification:** Moderate — backlog/story restructuring and coordinated updates across product, architecture, UX, and implementation artifacts.

**Responsibilities:**

- **Product owner:** Approve this complete proposal; choose the measurable timing tolerance and repeat count before timing acceptance; decide scope changes if target capabilities fail.
- **Developer:** After approval, update planning artifacts and tracking consistently, regenerate Epic 1 context, then implement Stories 1.1–1.7 in order. Add Story 1.8 for the owner-led finished-app physical acceptance.
- **Architect/PM:** Revisit the design or product scope only if exact build feasibility fails or Story 1.8 exposes unsupported required behavior.

**Success criteria:**

1. A pinned exact CLI and C SDK toolchain builds and installs reproducibly using the agreed pre-implementation workflow.
2. Stories 1.2–1.7 can proceed without representing physical-device behavior as verified.
3. Story 1.8 captures device/firmware and toolchain identifiers, separate phase/timed-pause background results, haptic/app foreground behavior, timing observations against an owner-approved tolerance, clock/recovery and Wakeup-limit behavior, reliability/battery notes, and finished-UI legibility/control checks.
4. No release is declared until required target checks pass or the product owner explicitly resolves any unsupported behavior.

## Approval

- Incremental approval of proposal directions: recorded above.
- Approval of the general sequencing direction: **Approved by the project owner on 2026-10-31**.
- Subsequent decision: **Keep the existing Story 1.1 pre-implementation physical-device gate.**
- Approval of this proposal as written: **Superseded / not approved for implementation**, because it changes the frozen Story 1.1 intent without renegotiation.
- Implementation authorization: **Halted** until the owner explicitly renegotiates Story 1.1 intent or performs its current acceptance criteria.
