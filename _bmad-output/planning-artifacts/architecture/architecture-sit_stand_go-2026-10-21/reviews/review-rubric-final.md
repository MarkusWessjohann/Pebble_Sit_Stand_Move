# Final Good-spine Rubric Review

**Refreshed:** 2026-10-23

**Subject:** `ARCHITECTURE-SPINE.md`  
**Review date:** 2026-10-22  
**Stake:** Personal solo Pebble Time 2 app  
**Verdict:** **Conditional pass.** The spine defines the implementation contracts for session state, UTC timing and invalid-clock behavior, commands, persistence, wakeup identity/recovery, notification attempts, and failures. Handoff remains conditional on target firmware/SDK verification and resolution of measurable timing tolerance. Those gates remain unpassed/open; no device behavior is claimed as validated.

## Review basis

- Applied the good-spine checklist in `.agents/skills/bmad-architecture/references/reviewer-gate.md`.
- Compared the current spine with its linked PRD and product brief and reviewed prior rubric, divergence, and technology reports against the current text.
- Ran `uv run .agents/skills/bmad-architecture/scripts/lint_spine.py --workspace _bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21`: **0 mechanical findings**.
- No parent spine or existing brownfield application code was identified in the reviewed artifacts.

## Remaining gates and decisions

### Target firmware/device and reproducible toolchain verification — unpassed gate

- **Location:** AD-3, AD-7, AD-14; Stack; Deferred; PRD §5.1.
- **Finding:** Pebble Time 2 firmware/OS support for Wakeup after leaving the app, haptic delivery, and foreground/UI presentation has not been physically verified. The exact SDK package is not pinned; the `latest` selector is not a reproducible version. This gate has not passed.
- **Gate:** Record the target firmware/build and exact CLI/SDK identifiers; verify build/install and the required leave-app wakeup, haptic, and foreground behavior on the physical target before implementation. Do not report these outcomes as validated before that test.

### Measurable timing tolerance — open decision

- **Location:** AD-2, AD-10, AD-12; Deferred; PRD §§5.2 and 9.
- **Finding:** UTC epoch wall time remains authoritative through clock changes: forward changes can make a deadline due and backward changes defer it. Start with invalid UTC is rejected, leaves the session inactive, and shows a visible error. During a running session with invalid UTC, state is retained and scheduling/haptics are suppressed until valid UTC returns, then reconciliation occurs. This policy is settled; invalid-clock wording is not an open decision. The acceptable measured transition tolerance remains undecided.
- **Gate:** Agree and measure an acceptable transition tolerance on verified target firmware.

## Previously reported gaps — current verification

| Earlier concern | Current evidence | Assessment |
| --- | --- | --- |
| Canonical record and invalid-record handling | AD-9 fixes schema version, lifecycle, generation, phase, deadline/remainder, inactive/paused validity, persisted running `WakeupId`, and invalid-schema behavior. | Addressed |
| Missed-deadline behavior and cadence | AD-4/AD-12 define `now >= deadline`, ordered reconciliation, recovery rebasing even after one late deadline, on-time sequential cadence, and no stale haptic replay. | Addressed |
| Event arbitration and lifecycle commands | AD-10 requires FIFO processing and reconciliation before commands/callbacks, and defines repeated/invalid lifecycle commands. | Addressed |
| Wakeup identity and stale/early callbacks | AD-9/AD-11 persist and query `WakeupId`, encode mutation generation in the cookie, define increment/wrap behavior, reject mismatches, and retain/re-arm early callbacks. | Addressed |
| Schedule/persistence crash windows and cleanup boundary | AD-11 specifies `wakeup_cancel_all()` on every launch, then reconciliation of the last committed record; candidate schedule → persist matching record/ID → cancel old ID; candidate cancellation on write failure; harmless stale callbacks; and no one-alarm assurance before cleanup completes. | Addressed as a behavioral contract; exercise failure/cut points during operational verification |
| Haptic and app-display contract | AD-7/AD-12/AD-14 distinguish haptic from readable UI, specify best-effort at-most-once attempts, and fix haptic-only mode. | Contract addressed; target behavior unverified |
| Invalid lifecycle commands and retry boundary | AD-10/AD-11/AD-13 specify no-op cases, repair of the last committed state, explicit user invocation as retry trigger for failed proposals, and visible failure state. | Addressed |
| Extension choices, UTC clock policy, and haptic decisions | AD-15 fixes additive +5/+10 choices; AD-2/AD-10/AD-12 and the PRD settle UTC wall-time authority and invalid-clock state/reconciliation; Skip emits one haptic, Start/Pause/Resume/Stop/Extend emit none, and automatic deadlines/overdue recovery emit one for the current phase. | Resolved; do not carry forward as open architecture gaps |

## Checklist assessment

- **Divergence points and enforceable AD rules:** State ownership, event ordering, wakeup identity, recovery, notification attempts, and failure semantics are specified.
- **Deferred work:** Remaining items are the unpassed target firmware/SDK gate and measurable timing tolerance above. Extension values, haptic decisions, UTC authority, and invalid-clock behavior are settled.
- **Named technology:** Documented Pebble API limits inform the design; exact SDK pin and Pebble Time 2 runtime behavior remain unverified.
- **Brownfield and inheritance:** Not applicable on the evidence reviewed; no existing application codebase or parent spine was identified.
- **PRD/brief coverage:** FR-1–FR-6 and UJ-1 are represented, including fixed phase defaults, controls, recovery, and notification behavior.
- **Operational envelope:** Runtime ownership, leave-app boundary, failure handling, packaging, power, storage, and physical verification are stated or deferred.

## Gate disposition

Conditional pass for architecture completeness. Before implementation, identify and verify the exact target firmware/toolchain on Pebble Time 2, including leave-app Wakeup, haptic, and foreground/UI behavior; agree and measure timing tolerance on the verified target. These checks are outstanding. Invalid-clock behavior is specified and is not an open UX gate. No physical-device validation is asserted by this review.
