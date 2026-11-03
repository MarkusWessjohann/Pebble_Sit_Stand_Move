---
title: 'Follow the cycle and recover from missed deadlines'
type: 'feature'
created: '2026-11-01'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '9541cbad17cc63f71d500ac020dcb93f39d5b333'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The session coordinator can advance phases and repair missed deadlines, but transitions do not yet provide the complete reminder behavior or naturally completed phase tally. The session must continue to communicate useful state after background wakeups and recovery.

**Approach:** Complete coordinator-owned cycle progression, ordered overdue recovery, at-most-once haptic requests, and daily tally updates. Preserve UTC deadlines, committed-state/Wakeup ordering, stale callback rejection, and the existing session view; physical Pebble Time 2 behavior remains for Story 1.8.

## Boundaries & Constraints

**Always:** Sit, Stand, and Move durations are 45, 10, and 5 minutes. An on-time transition advances from the expired deadline; recovery at `now >= deadline` processes elapsed phases in order and rebases the resulting phase to its full default duration from recovery time. Reconcile before competing commands. Count naturally completed phases once for the watch's local calendar day, across sessions; persist totals across app restarts and reset them at local midnight. Persist a transition's haptic-attempt marker before requesting haptic; never repeat a marked attempt. Recovery requests at most one haptic for the reconciled state. Failed persistence/scheduling retains the last committed state, reports an error, and does not claim the proposed phase is reliably scheduled. Invalid UTC preserves committed state and suppresses scheduling/haptics until valid time returns.

**Never:** Add pause, skip, extend, sensing, extra product surfaces, phone-dependent behavior, or claims of physical-device acceptance. UI must not own timers or advance session state. Do not replay stale haptics.

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| On-time deadline | Valid running phase; matching Wakeup at deadline | Advance in Sit → Stand → Move order from the expired deadline, schedule next deadline, count completed phase, persist haptic-attempt marker, request one haptic; Wakeup launch presents updated view | Retain committed state and show visible failure if scheduling or persistence fails |
| Overdue recovery | Running deadline due at recovery | Process elapsed phases in order, count natural completions on their local completion day, rebase final phase to its full duration from recovery time, schedule next deadline, request at most one haptic | Do not signal an uncommitted transition; retain committed state and show visible error on failure |
| Invalid UTC | Running session while UTC unavailable/invalid | Preserve committed record and suppress schedule/haptic; reconcile before scheduling when valid UTC returns | Show clock error |
| Stale/early callback | Callback has wrong lifecycle, generation, Wakeup ID, or is before deadline | Ignore stale callback; an early matching callback repairs scheduling without phase advance, tally increment, or haptic | Surface repair failure |

</frozen-after-approval>

## Code Map

- `_bmad-output/implementation-artifacts/epic-1-context.md` -- current Epic 1 rules and session-wide invariants.
- `_bmad-output/planning-artifacts/epics.md` -- Story 1.3 acceptance criteria, including cycle semantics, invalid UTC, tally, and reminder behavior.
- `_bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` -- AD-2–AD-4, AD-7, AD-9–AD-13 govern deadlines, recovery, effects, persistence, and failures.
- `src/c/session_coordinator.c` -- `prv_reconcile_phase`, `prv_recover`, `prv_wakeup`, and `prv_rollover_tally` already implement basic phase timing/recovery and rollover; extend these coordinator paths for completion counts, effects, and invalid-clock recovery. Preserve schedule/commit/cancel order and FIFO queue.
- `src/c/session_store.h` and `src/c/session_store.c` -- persisted 33-byte versioned record already includes daily counts and `haptic_generation`; retain record compatibility unless implementation evidence requires a documented schema change.
- `tests/session_coordinator_test.c` and `tests/pebble-stubs/pebble.h` -- deterministic time, storage, Wakeup, and queue stubs; extend with haptic/effect observability and tally/recovery cases.
- `src/c/sit_stand_move.c` -- renders coordinator view every second; retain render-only behavior and visible coordinator errors. Pebble Wakeup launch path is initialized in `session_coordinator_init`.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_coordinator.c` -- commit each natural phase transition with generation-bound haptic marker and update local-day tally exactly once; preserve on-time deadline cadence and ordered overdue rebase, including recovery and time-validity paths.
- [x] `tests/pebble-stubs/pebble.h` and `tests/session_coordinator_test.c` -- model haptic requests and verify normal cycle, exact-deadline boundary, multi-phase catch-up, local-day tally persistence/rollover, invalid and corrected UTC, duplicate/stale callbacks, and schedule/storage failures.
- [x] `src/c/sit_stand_move.c` -- ensure normal Wakeup launch renders committed current phase/remaining time and tally without adding UI-owned transition logic; only change if tests/source investigation shows it is needed.

**Acceptance Criteria:**
- Given a running phase reaches its deadline with a matching Wakeup, when the coordinator processes it on time, then it advances to the next phase, assigns the next default duration from the expired deadline, schedules the following deadline, increments the naturally completed phase tally once, and requests one haptic after its attempt marker is persisted.
- Given a running phase deadline is due when recovery occurs, when the coordinator reconciles it, then it processes elapsed phases in order, rebases the resulting phase to its full default duration from recovery time, updates today's tally only for completions on the current local day, and requests at most one haptic for the reconciled state.
- Given a callback has mismatched lifecycle, generation, or Wakeup ID, when it is delivered, then it causes no phase/tally mutation, haptic, or app reopen.
- Given a matching Wakeup arrives before its committed deadline, when processed, then the phase and tally remain unchanged and the intended deadline is rescheduled.
- Given UTC is invalid during a running session, when time-based processing occurs, then committed session state is preserved, scheduling and haptics are suppressed, and a visible clock error is shown; when UTC becomes valid, the saved deadline is reconciled before scheduling resumes.
- Given a phase completes naturally and a tally is persisted, when the app restarts within the local day or crosses local midnight, then today's totals restore or reset respectively; skipped/in-progress phases are not counted.
- Given transition persistence or Wakeup scheduling fails, when the transition is attempted, then the last committed state remains authoritative, the UI shows a visible failure, and no haptic is requested for an uncommitted transition.

## Implementation Notes

Implemented coordinator-owned natural-completion tally updates, on-time cycle progression, overdue deadline reconciliation, persisted haptic attempts, and valid/invalid UTC recovery behavior. Added deterministic tests for cycle transitions, recovery/day attribution, launch repair, duplicate/stale callbacks, clock adjustments, queue pressure, and scheduling/storage failures. The existing UI renders coordinator state and required no code change.

## Verification

**Commands:**
- `scripts/test.sh` -- expected: C coordinator and Python wrapper tests pass.
- `git diff --check` -- expected: no whitespace errors.

**Manual checks (if no CLI):** Confirm physical wakeup, haptic delivery, and app foreground behavior remain unclaimed and assigned to Story 1.8.

**Review result:** Three review passes found and resolved coordinator and verification gaps. Final behavior and edge-case review found no remaining actionable issues; physical Pebble Time 2 behavior remains assigned to Story 1.8.

### Review Findings

Code review of `9541cba..3088da3` (commit 3088da3), 2026-11-02, verified against HEAD. Layers: blind-hunter, edge-case-hunter, verification-gap, acceptance-auditor.

- [x] [Review][Defer] Stop cannot commit while a due transition keeps failing to schedule [src/c/session_coordinator.c:176-190, 745-780] — deferred: accepted by owner decision (option 2); Stop stays blocked until Wakeup scheduling works again. Reproduced at HEAD with a probe test (running, `now >= deadline`, `wakeup_schedule` failing: Stop leaves RUNNING with `SESSION_ERROR_WAKEUP`). Revisit if Story 1.8 device testing shows scheduling failures in practice; conflicts with Story 1.2 AC3 "Stop commits inactive" only in this failure state.
- [x] [Review][Patch] `prv_reconcile_phase` computes local midnight with the wrong DST flag — it copies `*localtime(&now)` and calls `mktime` with `tm_isdst` unchanged, so on a DST-change day the midnight is off by one hour. Repro (Europe/Berlin, 2026-10-25 fall-back, overdue 12 h): current code gives sit=13 stand=12 move=12, with `tm_isdst = -1` gives 13/13/13. Set `midnight.tm_isdst = -1` and add a DST-day regression test. [src/c/session_coordinator.c:66-80] (blind-hunter+edge-case-hunter; low)
- [x] [Review][Patch] Tally tests depend on the host timezone — `fake_now = 1800000000` is exactly local midnight in America/Los_Angeles and 23:00 the previous day in America/Anchorage. Verified: `test_recovery_counts_each_elapsed_phase_once` fails under `TZ=America/Los_Angeles` and `test_natural_cycle_tally_and_haptics_persist` under `TZ=America/Anchorage`; both pass under UTC and Europe/Berlin. Pin `TZ` for the C test run (in `scripts/test.sh` and `fake_reset`), and keep the `midnight + N` tests as the only local-day coverage. [scripts/test.sh; tests/session_coordinator_test.c: fake_reset] (verification-gap+blind-hunter; medium)
- [x] [Review][Patch] `test_long_recovery_tallies_only_current_local_day` casts `int64_t *` to `time_t *` and indexes `expected[phase - 1]` — undefined on a 32-bit `time_t`, and it relies on `SESSION_PHASE_SIT == 1`. Copy into a `time_t` local and index through an explicit phase-to-slot mapping. [tests/session_coordinator_test.c:904-906] (blind-hunter+edge-case-hunter; low)
- [x] [Review][Patch] `test_failed_transition_has_no_tally_or_haptic` ends on `assert(fake_schedule_count >= 4)` — a lower bound that passes if retries schedule extra Wakeups. Assert the exact count. [tests/session_coordinator_test.c:564] (blind-hunter+acceptance-auditor; low)

#### Rejected

- false — Failed transition or recovery leaves wakeup_id -1 with no repair path until relaunch (edge-case-hunter, acceptance-auditor, blind-hunter): fixed at HEAD by the Story 1.2 review patch. `prv_wakeup_lost()` makes the clock check and every command reload the committed record and re-run recovery; `test_clock_check_retries_failed_due_transition` and `test_recovery_failure_exits_report_error_and_retry` cover it.
- false — A callback or synthetic event with `wakeup_id == -1` passes the identity check: the OS only delivers non-negative ids, and `prv_reconcile_before_command` and the clock check both require `wakeup_id >= 0` before building a synthetic due event.
- false — A transition that completes just before midnight and is processed after it is not counted: the spec counts completions "on their local completion day", so yesterday's completion correctly does not enter today's tally.
- false — The 999 count cap is an unspecified magic number: `session_store_validate` rejects any count above 999, so saturating keeps the record writable.
- false — Overdue recovery buzzes on any launch, not only a Wakeup launch: the spec's recovery row says "request at most one haptic" and the marker is persisted before the call, so there is no replay.
- false — Clock check skips a due deadline when `wakeup_id` is -1 with no error: `prv_wakeup_lost()` routes that state to recovery at HEAD.
- low — `get_view` enqueues a clock check, so UI rendering drives time processing: the coordinator is still the only mutator and it matches the existing 1 s render refresh; moving it to a coordinator-owned timer adds complexity with no named harm.
- low — A full queue overwrites a visible CLOCK/STORAGE/WAKEUP error with QUEUE, and QUEUE is not cleared: requires 8 pending events within one tick; already deferred in Story 1.2's triage log and `deferred-work.md`.
- low — `localtime()` returning NULL makes the catch-up loop walk every phase: not reachable on the watch, and the guard adds a branch.
- low — A backward clock change past a day boundary resets the tally (`day != tally_day` instead of ordering): rare, arguably a different day, and the fix adds a branch.
- low — A transition across local midnight leaves `tally_day` stale until the async rollover: it self-corrects on the next render and `prv_count_completion` resets on the next completion.
- low — Invalid-clock branch of the clock check keeps a live Wakeup when `s_error` was already CLOCK: the callback then cancels itself in `prv_wakeup`, so there is no harm.
- low — Redundant `wakeup_id = -1` assignments in `prv_recover`, 3600 s cycle-skip assumption, no midnight-boundary on-time test: cosmetic or already covered by the DST patch and the existing `midnight + N` tests.
- rejected (spec edits) — `status: 'done'` vs sprint `review`, `review_loop_iteration: 0`, Verification section without run output, and the `epic-1-context.md` rewrite dropping the Story 1.8 checklist: every fix edits the spec or epic context, and the Story 1.8 checklist is still in full in `epics.md` (Story 1.8 AC).
