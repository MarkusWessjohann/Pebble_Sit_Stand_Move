---
title: 'Manually pause and resume a session'
type: 'feature'
created: '2026-11-01'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '0491edf3fc01dc6fabb27b2fde020becfc4c844d'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** A running session currently cannot be paused indefinitely; the visible Pause/Resume menu row is not wired to lifecycle behavior. A phase must remain unchanged while its owner is unavailable.

**Approach:** Add coordinator-owned manual pause and resume transitions with persisted positive remaining phase time, no phase deadline or Wakeup while paused, and a new UTC deadline on resume. Render the frozen countdown with a readable `PAUSED` label and keep both actions silent.

**Decision:** Pause commits its paused state and attempts Wakeup cancellation; any callback that still arrives is rejected using lifecycle, generation, and Wakeup-ID validation because Pebble's cancellation API provides no success result.

## Boundaries & Constraints

**Always:** Reconcile a due running deadline using `now >= deadline` before Pause. Increment generation on committed lifecycle changes. Pause preserves the phase and positive remaining duration; paused records have no deadline or Wakeup. Resume uses current valid UTC plus the saved remainder and schedules the new deadline. Persist before reporting success; leave the last committed state authoritative on write/schedule failure. Ignore stale callbacks by lifecycle, generation, and Wakeup ID. UI dispatches through the coordinator and renders its state. Start, Pause, Resume, Stop remain haptic-free.

**Never:** Add an automatic resume deadline, change phase defaults, advance the cycle during pause, or add UI-owned timing/state mutation. Do not claim physical-device behavior verified; Story 1.8 owns it.

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Pause | Running phase, valid UTC, deadline in future | Commit paused phase and positive `deadline - now` remainder; clear deadline/Wakeup; no haptic | On persistence failure retain running record and Wakeup; show visible error |
| Due Pause | Running phase, `now >= deadline` | Reconcile deadline first, then pause the resulting phase with its positive remainder | Failed reconciliation/mutation must not present an uncommitted state |
| Resume | Paused phase, valid UTC | Commit same phase as running with deadline `now + saved remainder` and scheduled Wakeup; no haptic | On schedule/persistence failure retain last committed paused state and show visible error |
| Stale callback | Callback from before Pause or Resume | Ignore without phase change, haptic, or app reopen | No error required |

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c` -- owns the FIFO event queue, pre-command due-deadline reconciliation, callback validation, persistence ordering, and `SessionView`; add queued Pause/Resume behavior here.
- `src/c/session_coordinator.h` -- public coordinator API and view type; lifecycle is carried through `SessionRecord`.
- `src/c/session_store.h` and `src/c/session_store.c` -- currently version-1 33-byte record supports only inactive/running. Add paused lifecycle and saved remaining seconds with strict validation; unsupported/invalid records recover inactive with visible error per AD-9.
- `src/c/sit_stand_move.c` -- existing native menu draws Pause at row 0 and currently treats it as an unimplemented action; render lifecycle-specific Pause/Resume and `PAUSED`, frozen phase and remainder.
- `tests/session_coordinator_test.c` -- deterministic time/storage/Wakeup/haptic fixtures; extend menu, store, failure, due-deadline, persistence/recovery, and stale-callback coverage.
- `_bmad-output/planning-artifacts/epics.md` Story 1.4 and `architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` AD-2, AD-9–AD-13 define acceptance and record/ordering invariants.
- The completed Story 1.3 coordinator and test suite establish cycle/tally and callback behavior; preserve those paths and extend the shared coordinator rather than adding UI timing.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_store.h` and `src/c/session_store.c` -- represent and validate paused remainder and lifecycle-specific absence of deadline/Wakeup, retaining compact versioned record semantics.
- [x] `src/c/session_coordinator.c` and `src/c/session_coordinator.h` -- serialize Pause/Resume, reconcile due deadlines first, commit state with correct generation and schedule ordering, expose frozen remaining time, and reject stale callbacks.
- [x] `src/c/sit_stand_move.c` -- wire native menu Pause/Resume and render readable `PAUSED`, phase, and saved countdown.
- [x] `tests/session_coordinator_test.c` -- verify pause/resume, exact due boundary, persistence/scheduling failures, restart restoration, no haptic, and stale callback rejection.

**Acceptance Criteria:**
- Given a running phase with a future deadline, when Pause is selected, then the same phase and positive remaining duration are committed as paused with no deadline/Wakeup and no haptic.
- Given a manually paused session, when viewed, then readable `PAUSED`, the frozen phase, and saved remaining time are shown.
- Given a manually paused session and valid UTC, when Resume is selected, then the same phase runs with a deadline at current UTC plus saved remainder and no haptic.
- Given a paused session is shown, when the middle hardware control is pressed, then it resumes identically to the menu command.
- Given Pause or Resume coincides with a due deadline, when the coordinator processes it, then it reconciles first using `now >= deadline` and applies the command to the resulting state.
- Given a pre-mutation callback arrives after Pause or Resume, when validated, then it cannot advance phase, request haptic, or reopen the app.
- Given a persistence or Wakeup scheduling operation fails, when Pause or Resume is attempted, then committed state remains authoritative and a visible failure is shown; no uncommitted state is presented as active or reliably scheduled.

## Implementation Notes

Implemented persisted manual pause using the existing 33-byte record, storing the paused remaining duration in the state-specific timing field. Pause commits before best-effort Wakeup cancellation; stale callbacks are rejected by existing lifecycle, generation, and ID validation. Resume schedules and commits a fresh phase Wakeup before reporting running state. The active menu now toggles Pause/Resume, the Select/middle control resumes while paused, and the view renders `PAUSED`, phase, and frozen remaining time. Tests cover paused record validation/round-trip/recovery, due-boundary reconciliation, failures, silent controls, and stale callbacks.

## Verification

**Commands:**
- `scripts/test.sh` -- passed: coordinator tests and all 3 Python wrapper tests. The Pebble SDK 4.33.1 subprocess check reports unavailable because the SDK is not installed in this environment.
- `git diff --check` -- passed with no whitespace errors.

**Manual checks (if no CLI):**
- Inspect paused-record encoding/validation, no-Wakeup paused recovery, menu and `PAUSED` rendering, silent haptic behavior, and visible errors. Pebble SDK/device behavior remains for Story 1.8.

## Review Triage Log

- false — Paused remainders may exceed the default phase duration after Story 1.7 extensions; restricting them to default duration would break resuming a valid extended phase.
- false — The record stays within the existing version-1 schema and 33-byte size; older software rejects the new lifecycle as invalid/inactive, and cross-version active-session compatibility is not a Story 1.4 requirement.
- false — Persistence-failure tests now load the stored record and confirm failed Pause retains running state and failed Resume retains paused state.
- false — Tests now cover both scheduling and persistence failure during due-deadline reconciliation before Pause; neither commits paused state or requests a haptic.
- false — A dedicated test verifies Stop from paused clears the saved remainder in memory and persisted state.
- defer — The middle-control decision is factored into and tested through `session_coordinator_resume_if_paused()`, and source inspection confirms the Select click handler calls it. The repository's host test harness does not compile the Pebble UI layer; actual hardware button behavior is explicitly verified in Story 1.8.
- false — A due Pause first processes the automatic phase deadline; any haptic belongs to that due automatic transition, not to the manual Pause command. This preserves Story 1.3's transition reminder contract and Story 1.4's silent manual action.
- patch — Factored paused middle-control behavior into `session_coordinator_resume_if_paused()` and added coverage for resumed-when-paused and no-op-when-running; `scripts/test.sh` passes.
