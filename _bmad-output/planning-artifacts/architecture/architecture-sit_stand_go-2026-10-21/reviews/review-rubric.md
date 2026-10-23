# Good-spine Rubric Review

**Subject:** `ARCHITECTURE-SPINE.md`  
**Review date:** 2026-10-21  
**Stakes:** Personal Pebble Time 2 reminder app  
**Verdict:** **Not ready for implementation handoff yet.** The spine captures the product cycle and its central state/time decisions unusually well, and the mechanical lint is clean. Before handoff, close the target-device feasibility gate, make the sole-owner/wakeup rules enforceable at module boundaries, and pin the remaining product/runtime decisions that could otherwise be chosen differently by separate implementers.

## Review basis

- Applied the Good-spine checklist in `.agents/skills/bmad-architecture/references/reviewer-gate.md`.
- Compared the spine with the source PRD at `_bmad-output/planning-artifacts/prds/prd-sit_stand_go-2026-10-21/prd.md`, the PRD reconciliation report, and the architecture memlog.
- Ran `uv run .agents/skills/bmad-architecture/scripts/lint_spine.py --workspace _bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21`: **0 mechanical findings**.
- Checked the cited Rebble SDK 4 and developer-guides pages. They establish that SDK 4 APIs are documented; they do not establish an actively supported/current compiler toolchain or Pebble Time 2 firmware compatibility. No target-device test evidence was present in the reviewed artifacts.
- No parent spine or existing application codebase was identified in the supplied artifacts, so inheritance and brownfield-ratification checks are not applicable on current evidence.

## Findings

### High — Resolve before implementation handoff

#### H1. Pebble Time 2 background behavior remains a release-blocking unknown

- **Location:** AD-3, its operational boundary, Stack, and Deferred (lines 37–43, 88–93, 130–134).
- **Checklist concern:** The product's central promise is background reminders; the spine makes target verification mandatory but leaves both the target firmware and the result unknown. “Leaves the app” is not operationally defined: foreground loss, dismissal/closure, app relaunch, device reboot/power-off, and notification/vibration availability can behave differently. Explicit termination is out of scope, but the other cases are not fully separated into pass/fail expectations.
- **Divergence/consequence:** Different implementers or testers could treat different app lifecycle and device conditions as satisfying “background,” or proceed with an API/firmware combination that cannot deliver the required reminders.
- **Recommended disposition:** **Discuss, then defer implementation until resolved.** Record exact Pebble Time 2 firmware/OS and SDK/toolchain, test cases and observed results for each in-scope lifecycle condition, and the product decision if a required case fails. Keep explicit termination excluded as already agreed.

#### H2. The single-writer and wakeup consistency rules need a mandatory implementation boundary

- **Location:** Design Paradigm, AD-1 and AD-3, Persistence/Wakeup identity conventions, Structural Seed (lines 21, 25–29, 37–43, 83–85, 95–116).
- **Checklist concern:** AD-1 says only the coordinator writes lifecycle/phase/deadline state and events are serialized; AD-3 requires persisted state and replacement/cancellation of stale wakeups. These are sound decisions, but the module layout is explicitly only a suggestion and does not require the store, UI, or wakeup adapter to prevent direct state writes or scheduling. “After every state mutation” and “persist … and replace/cancel” also leave crash/interleaving consistency to implementer interpretation.
- **Divergence/consequence:** Separate units can create alternate writers or update persisted state and scheduled wakeups in incompatible orders, reintroducing exactly the divergent phase/deadline behavior the ADs are meant to prevent.
- **Recommended disposition:** **Autofix in the spine before handoff.** Make the coordinator/session module the required public command/event boundary; keep the authoritative record private behind it; require the wakeup adapter to report events rather than mutate state; specify the persisted-record-before-schedule ordering and stale-generation rejection as required contracts. The structural seed can remain illustrative, but must not weaken those boundaries.

### Medium — Close or explicitly gate before independent implementation

#### M1. Near-simultaneous user actions and due wakeups have no arbitration rule

- **Location:** AD-1, AD-2, AD-4; Deferred timer tolerance/device-clock changes (lines 29, 35, 45–49, 136).
- **Checklist concern:** The PRD requires a single understandable active phase when controls occur near a transition. The spine serializes events, but does not specify what serialization means when a deadline is due at the same time as pause, skip, extend, or stop—for example, whether stop suppresses an already-due notification or whether pause captures zero remaining time before transition reconciliation.
- **Divergence/consequence:** Implementations may produce different active phases, deadlines, and notifications for the same boundary interaction despite using one coordinator.
- **Recommended disposition:** **Defer to a named coordinator contract before implementation.** Define event ordering/precedence for due-deadline reconciliation versus each user command, plus the accepted timing tolerance. Tests should exercise boundary cases on the verified target.

#### M2. Deferred extension choices can produce different v1 interfaces and timing

- **Location:** AD-6 and Deferred extension-duration choices/UI navigation (lines 57–61, 137); PRD FR-6 open item.
- **Checklist concern:** The spine requires a user-selected extension but leaves the available values and selection interaction to product/UX. That is a valid open product decision, but builders could independently choose different values, and a chosen value directly changes the phase deadline and notification timing.
- **Divergence/consequence:** Distinct implementations could claim FR-6 while offering different choices or interpreting the selected amount differently.
- **Recommended disposition:** **Defer to UX/product, with a handoff gate.** Keep implementation blocked until the offered values and interaction are specified; retain AD-6's rule that extension adds only to the current phase and never changes defaults.

#### M3. Vibration and notification-mode configurability are not closed consistently

- **Location:** AD-7 and Deferred (lines 63–67, 137); PRD §9 question 6.
- **Checklist concern:** AD-7 says vibration is the “initial” mode and subject to device verification, while the PRD leaves open whether notification mode is configurable. The spine does not say whether v1 is fixed to vibration, whether fallback modes are permitted, or whether mode selection may become a setting.
- **Divergence/consequence:** One implementation may add settings or use a fallback while another ships vibration-only, creating different scope and behavior under the same FR-3 mapping.
- **Recommended disposition:** **Discuss/defer to product and target verification.** State a v1 mode and any permitted fallback behavior; treat configurability as out of scope unless explicitly adopted.

#### M4. SDK 4 documentation is verified, but the named build technology is not verified-current or reproducible

- **Location:** Stack and Deferred SDK/toolchain version pin (lines 88–93, 133).
- **Checklist concern:** The cited Rebble site documents SDK 4, but the spine has no pinned compiler/toolchain/package version and no evidence that the selected toolchain is currently obtainable and compatible with Pebble Time 2. A documentation landing page is not enough to verify those properties.
- **Divergence/consequence:** Builders can select different SDK distributions or compiler versions, encounter incompatible builds, or rely on different device support assumptions.
- **Recommended disposition:** **Defer to build setup before implementation.** Verify and record the exact supported SDK/toolchain distribution and version, build invocation/environment, and device compatibility. If the toolchain is intentionally legacy, mark it as a deliberate compatibility choice rather than implying current support.

#### M5. Timer tolerance and wall-clock discontinuities are deferred without decision criteria

- **Location:** AD-2 and Deferred timer tolerance/device-clock changes (lines 31–35, 136); PRD §5.2 and §9.
- **Checklist concern:** UTC epoch deadlines handle timezone/DST display changes, but do not decide what should happen if the device clock is manually moved forward/backward, corrected by synchronization, or otherwise discontinuous. “Define and test during implementation” does not give builders a shared acceptance threshold or expected behavior.
- **Divergence/consequence:** Implementations may preserve wall-clock deadlines versus elapsed duration differently, and report different timing reliability for the same clock change.
- **Recommended disposition:** **Defer as an explicit open runtime policy.** Decide expected behavior for material clock jumps and a measurable timing tolerance after target feasibility testing; test that policy rather than leaving it to implementation discretion.

## Checklist assessment

| Good-spine criterion | Assessment |
| --- | --- |
| Fixes real divergence points for the level below | **Mostly.** The single owner, deadline source, fixed 45/10/5 behavior, control semantics, wakeup identity, and recovery behavior address the core seams. H2 and M1 leave enforceability and event-boundary outcomes underspecified. |
| Every AD Rule is enforceable and prevents its stated divergence | **Partial.** AD-2, AD-4, AD-6, and AD-7 express observable constraints. AD-1/AD-3 need required module/API and state/scheduling boundaries (H2); AD-5 and AD-8 are clear scope/ownership constraints. |
| Deferred items cannot let units diverge | **Not yet.** Extension choices, notification mode, timing arbitration/tolerance, and device-clock behavior remain capable of producing different implementations (M1–M3, M5). SDK/toolchain is similarly unpinned (M4). |
| Named technology verified-current | **Not established.** SDK 4 documentation is accessible, but active/current toolchain availability and Pebble Time 2 compatibility are unverified (H1, M4). |
| Ratifies rather than contradicts a brownfield codebase | **Not applicable on supplied evidence.** No application repository or existing app code was present in the reviewed artifacts. |
| PRD capabilities represented | **Largely yes.** The capability map binds FR-1–FR-6 and UJ-1, and AD-6 captures important action semantics; notification payload, fixed defaults, recovery, scope limits, and watch-first runtime ownership are represented. Remaining background acceptance and v1 notification-mode decisions need closure (H1, M3). |
| Inherited decisions preserved | **Not applicable on supplied evidence.** No parent spine was identified. |
| Every owned dimension decided, deferred, or open, including operational envelope | **Mostly explicit, but not handoff-ready.** Runtime/firmware, toolchain, storage encoding, UX choices, timing, packaging/install/update, test setup, release channel, and power/reliability are all named as deferred. There is no cloud/provider/service operations dimension to decide for this watch-owned personal app; AD-5 explicitly excludes phone/cloud runtime dependencies. The operationally central firmware/runtime and timing deferrals need concrete criteria/owners/gates (H1, M1, M4, M5). |

## Positive evidence

- The spine's coverage map traces each FR and UJ-1 to architecture decisions.
- AD-6 preserves the fixed defaults, fresh-session start, stop semantics, pause behavior, skip behavior, and extension semantics.
- AD-4 resolves missed deadlines deterministically and avoids replaying stale notifications.
- The persistence convention, wakeup identity, and failure convention provide useful consistency anchors.
- Reminder-only and watch-owned scope are explicit; no cloud, phone runtime, analytics, sensing, or scheduling creep is invited.
- Mechanical spine lint passes without findings.
