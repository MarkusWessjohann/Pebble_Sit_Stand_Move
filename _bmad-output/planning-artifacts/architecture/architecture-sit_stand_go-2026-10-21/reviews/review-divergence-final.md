# Final adversarial divergence review

**Subject:** `ARCHITECTURE-SPINE.md`  
**Re-evaluated:** 2026-10-23 against the current spine, PRD, and brief.  
**Verdict: No unresolved semantic implementation divergences remain in areas 1–8; operational execution and target verification remain unpassed gates.** The spine states the shared observable state, event, wakeup, recovery, and haptic contracts, including launch cleanup and injected-failure/crash-cut acceptance checks. Pebble Time 2 behavior remains unverified; this is a target feasibility gate, not evidence that device behavior has passed.

## Re-evaluation of prior gaps

| # | Prior divergence | Current spine evidence | Assessment |
| --- | --- | --- | --- |
| 1 | Inactive/paused record field validity | AD-9 requires inactive to have no active phase/deadline/wakeup ID, and paused to retain phase plus positive remaining time without deadline/wakeup ID; invalid schema is treated as inactive with a recoverable error. | Closed |
| 2 | Session identity vs mutation generation | AD-9/AD-11 define a positive 31-bit mutation generation, all increment points, monotonic scope across restarts, cookie use, and wrap handling; it is explicitly not a separate session ID. | Closed |
| 3 | Durable Wakeup identity/discovery | AD-9 persists the running `WakeupId`; AD-11 queries it with `wakeup_query(id)` and explicitly avoids duplicate scheduling because the API has no list-by-cookie operation. | Closed |
| 4 | Schedule/commit crash cuts and cancellation failure | AD-11 calls `wakeup_cancel_all()` on every app launch, then reconciles the last committed record and re-schedules one intended Wakeup if running. It fixes candidate-schedule → matching-record/ID commit → old-ID cancellation, candidate cancellation on persistence failure, stale-generation rejection, and forbids one-alarm assurance until cleanup completes. AD-13 fixes the failure UI/reliability boundary; failed proposals retry only on explicit user invocation. | Contract closed; exercise specified failure and crash cut points operationally |
| 5 | Recovery rebase at one expired deadline | AD-4/AD-12 rebase on every recovery with `now >= deadline`, including one late deadline; an on-time callback advances from the expired deadline. | Closed |
| 6 | Haptics for user-caused phase changes | AD-12 specifies one haptic for Skip and automatic deadlines/overdue recovery (for the current phase only), none for Start/Pause/Resume/Stop/Extend, with persisted at-most-once attempt handling. AD-14 fixes haptic-only mode. | Closed |
| 7 | Launch repair vs retry boundary | AD-11 repairs only the last committed running state on launch; AD-13 does not retry a failed proposed mutation automatically and permits retry on the next user invocation. | Closed |
| 8 | Operational verification coverage | AD-11–AD-14 and the Deferred acceptance gate specify injected schedule, persistence-write, cancellation, and launch-repair failures; crashes after candidate scheduling, state commit, old-Wakeup cancellation, and before/after haptic-attempt persistence; and assertions for session record, `WakeupId`/generation, callback decisions, cleanup/retry, UI reliability/error state, and haptic attempts. | Test gate specified; execution remains outstanding |

## Current adjacent decisions

- AD-15 and PRD FR-6 fix extension choices at +5 and +10 minutes, additive to the active phase and stackable per command.
- AD-2, AD-10, AD-12, and PRD §5.2 fix UTC epoch wall time as authoritative through clock changes. Forward adjustments can make deadlines due; backward adjustments defer them. Start with invalid UTC is rejected/inactive with a visible error; during a running session, state is retained and scheduling/haptics are suppressed until valid UTC returns, then the deadline is reconciled. Invalid-clock behavior is settled; measurable timing tolerance remains open.
- AD-6/AD-15 and PRD FR-6 fix Extend at +5/+10 minutes, additive and stackable. Skip emits one haptic; Start/Pause/Resume/Stop/Extend emit none; automatic deadlines and overdue recovery emit one for the current phase.
- `notify_if_missed = false` is explicit in AD-3; application recovery owns reconciliation and the single current-phase haptic.

The review does not claim physical Pebble Time 2 Wakeup, haptic, foreground, or timing behavior has been validated. Those checks remain required by the spine and PRD.

## Residual guard

Before treating the operational gate as passed, execute the spine's specified injected-failure/crash-cut matrix: schedule, persistence-write, cancellation, and launch-repair failures; crashes after candidate scheduling, committed state, old-Wakeup cancellation, and before/after haptic-attempt persistence. Assert lifecycle/deadline/`WakeupId`/generation, callback acceptance or rejection, launch `wakeup_cancel_all()` cleanup, retry boundary, UI reliability/error state, and haptic-attempt count. Separately record exact firmware and SDK/build identifiers and physical-target results for leave-app Wakeup, haptic, foreground presentation, and measurable timing tolerance. These checks have not passed; no physical validation is asserted here.
