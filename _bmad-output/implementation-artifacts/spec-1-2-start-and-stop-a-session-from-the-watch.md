---
title: 'Start and stop a session from the watch'
type: 'feature'
created: '2026-11-01'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: 'ef33c2f08aa067d7d4fc1a0150c55ebf7cabbb4f'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The Pebble app is currently a Wakeup diagnostic harness, so its owner cannot start and stop an actual reminder session or see authoritative session status.

**Approach:** Replace the diagnostic interactions with a native session view/menu backed by one serialized coordinator and a compact persisted session record. Starting begins Sit from current valid UTC and commits its deadline and Wakeup; stopping invalidates the session and cancels its Wakeup. The view includes today's Sit/Stand/Move tally; natural-completion tally updates are implemented with phase progression in Story 1.3. Pause/skip/extend behavior and physical-device acceptance remain in their designated later stories.

**Decision:** Proceed with Story 1.2 while Story 1.1 remains in progress. The exact C SDK pin and reproducible build/install evidence remain open in Story 1.1; this authorization does not mark that readiness gate complete.

## Boundaries & Constraints

**Always:** The coordinator alone mutates lifecycle, record, and Wakeup state; serialize commands, callbacks, and recovery through FIFO processing and reconcile due deadlines before commands using `now >= deadline`. Persist a versioned canonical record and generation-bound Wakeup identity; reject stale callbacks. Start and Stop are silent. Invalid UTC keeps Start inactive with visible error. Failed proposals must not appear committed or reliably scheduled. Render native Pebble text/menu surfaces and keep physical Pebble Time 2 acceptance in Story 1.8.

**Never:** Add phone-dependent operation, extra product surfaces, sensing, analytics/history, configurable phase defaults, or claim physical firmware behavior is verified. Do not implement later-story user controls or natural-completion tally updates in this story.

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Start | Inactive, valid UTC | Commit running Sit with deadline `now + 45 minutes`, schedule its Wakeup, display session | Remain inactive and show an error if scheduling or persistence cannot commit |
| Invalid clock | Inactive, unavailable/invalid UTC | Start is rejected; remain inactive | Display concise clock error |
| Stop | Running session | Commit inactive state, cancel Wakeup; stale callback has no effect | Retain last committed state and show failure if mutation cannot commit |
| Relaunch | Valid or invalid stored record | Clear orphan Wakeups, restore only a valid committed record, repair intended scheduling | Treat invalid/unsupported record as inactive with visible recoverable error |

</frozen-after-approval>

## Code Map

- `_bmad-output/implementation-artifacts/epic-1-context.md` -- authoritative Epic 1 invariants, scope, coordinator and UI patterns.
- `_bmad-output/planning-artifacts/epics.md` -- Story 1.2 behavior and acceptance criteria, plus its explicit Story 1.1 dependency.
- `_bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` -- AD-9 through AD-13 define canonical state, FIFO reconciliation, Wakeup commit ordering, and failure semantics.
- `src/c/sit_stand_move.c` -- temporary Wakeup diagnostic harness; replace diagnostic controls/status with the session experience, reusing Pebble window, text, click, and Wakeup API patterns.
- `package.json`, `wscript`, `pyproject.toml`, `.python-version` -- Pebble targets/build rules and current CLI/Python pins; SDK API level 3 is not an exact SDK package pin.
- `_bmad-output/implementation-artifacts/platform-verification.md` -- readiness evidence and local/owner-reported toolchain limits; local Pebble CLI/emulator are unavailable.
- `_bmad-output/implementation-artifacts/sprint-status.yaml` -- Story 1.1 remains in progress and Story 1.2 is now in review.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_coordinator.c` and `src/c/session_coordinator.h` -- own serialized Start, Stop, recovery, deadline reconciliation, committed state, and callback validation.
- [x] `src/c/session_store.c` and `src/c/session_store.h` -- validate and persist one compact versioned lifecycle record within Pebble storage limits.
- [x] `src/c/sit_stand_move.c` -- render current time, running countdown, textual phase and today's Sit/Stand/Move tally, plus native Start/Pause/Resume/Skip/Stop menu labels as applicable; dispatch lifecycle commands only through the coordinator.
- [x] `tests/session_coordinator_test.c` -- exercise invalid clock/record, Start/Stop, Wakeup schedule/persist failures, launch repair, overdue reconciliation, and stale callback behavior with deterministic Pebble API stubs.

**Acceptance Criteria:**
- Given an inactive session and valid UTC, when Start is selected, then committed state is running Sit with a 45-minute deadline from current UTC and its Wakeup is scheduled without haptic.
- Given UTC is invalid or unavailable, when Start is selected, then the session remains inactive and a visible clock-time error is shown.
- Given an active session, when Stop is selected, then inactive state is committed and its Wakeup cancelled; any stale callback is ignored without reminder or haptic.
- Given commands, Wakeup callbacks, or recovery events arrive, when processed, then one FIFO coordinator reconciles any due deadline before applying a command and rejects callbacks whose lifecycle, generation, identity, or deadline is invalid.
- Given the app is relaunched, when persisted state is restored, then orphan Wakeups are cleared, only a complete supported record is trusted, intended running scheduling is repaired, and invalid data is shown as inactive with recoverable error.
- Given session state or Wakeup scheduling fails, when Start or Stop is attempted, then no uncommitted mutation is presented as active/reliable and a visible failure is shown.
- Given the session view/menu is rendered, when inactive or running, then native Pebble surfaces show current watch time and today's Sit/Stand/Move tally, plus remaining phase time and readable phase identity while running; Start appears inactive and Pause/Resume, Skip, Stop appear in that order while active.

## Implementation Notes

Implemented the event-queued session coordinator, 33-byte versioned persistent record, session view and native menu. Start commits Sit with its 45-minute UTC deadline and scheduled Wakeup; Stop commits inactive state before cancelling the Wakeup. Launch recovery clears orphan Wakeups and repairs a valid active record. The coordinator advances phases on a valid due callback/recovery; natural completion tally increments and physical-device verification remain for later stories. Story 1.1's SDK pin and reproducible build/install gaps remain open as authorized. Tests use deterministic Pebble API stubs because the Pebble CLI/SDK is unavailable in this environment.

## Verification

**Commands:**
- `cc -std=c99 -Wall -Wextra -Werror -Itests/pebble-stubs -Isrc/c src/c/session_coordinator.c src/c/session_store.c tests/session_coordinator_test.c -o /tmp/session_coordinator_test && /tmp/session_coordinator_test` -- expected: `session coordinator tests passed`.
- `git diff --check` -- expected: no whitespace errors.

**Manual checks (if no CLI):**
- Verify session record shape and storage bounds, schedule/commit/cancel ordering, all menu/view text and failure states, and any unverified hardware behavior remains assigned to Story 1.8.
- Pebble SDK target build and Pebble Time 2 UI/firmware behavior remain unverified; project CLI/SDK and physical watch are unavailable here.

## Review Triage Log

- deferred — Pause/Resume and Skip menu labels are present, but their lifecycle actions belong to Stories 1.4 and 1.6; this story establishes the menu surface and only implements Start/Stop.
- deferred — Natural phase tally increments are not implemented; phase completion and tally updates belong to Story 1.3. This story displays persisted daily tally fields.
- deferred — Automatic phase-transition haptics belong to Story 1.3; Start and Stop remain silent as required here.
- false — A failed transition schedule can leave the committed running record without its next Wakeup, but it sets a visible error and retains the previous committed record as required by the failure contract; retries are not automatic.
- patch — A callback delivered before its committed deadline previously returned and lost the only alarm after backward clock correction. It now schedules a replacement for the committed deadline and persists its new identity.
- patch — Overdue recovery previously looped once per missed phase; it now skips whole 60-minute cycles arithmetically before processing the remaining phases.
- false — Launch repair can fail after orphan cleanup, but it surfaces a scheduling/storage error and leaves only the committed state authoritative, consistent with launch repair's no-automatic-retry rule.
- deferred — Direct Pebble MenuLayer/button integration is not exercised by the coordinator tests; they verify Start/Stop menu-row dispatch at the coordinator entry point, and source inspection confirms the native callback forwards its selected row there. Exercising the actual UI callback requires the unavailable Pebble SDK/runtime.

### Review round 2 (adversarial, edge-case, verification-gap)

- patch — The menu replaced the main window's click config and was destroyed inside its own select callback. It now lives in its own `s_menu_window` that is pushed and popped, and BACK is handled by the window stack. Not compiled or run: the Pebble SDK is unavailable here.
- patch — `prv_schedule` treated only `-1` as a failed `wakeup_schedule`. Any negative status now fails, and a test covers `E_RANGE`.
- patch — `get_view` re-enqueued a day rollover on every call when the write failed or `localtime()` returned NULL. It is now requested at most once per day, and never for day 0. A test counts the write attempts.
- patch — A failed re-schedule during launch repair left the record running with a Wakeup id that `wakeup_cancel_all` had just removed. The id is now invalidated in memory.
- patch — `session_store_clear` compared the `persist_delete` status as a bool. It now checks `S_SUCCESS`, and the stub uses `status_t`-style codes.
- patch — Test setup used `assert()` around side effects. It now uses `prv_save` and `prv_check`.
- patch — Verification gaps closed with tests: Wakeup cookie, rollover, multi-cycle catch-up and overdue recovery tables, early callback, persisted state after a failed Stop, menu row contract, store round trip, and store reject cases. `scripts/test.sh` runs the C and Python tests.
- deferred — A full event queue drops the new event, which may be a Wakeup or Stop. Reserving slots or coalescing needs a queue design decision.
- deferred — A failed due transition or launch repair does not retry, which is consistent with the earlier `false` entries. The edge-case and adversarial lenses both flag the frozen "00:00" outcome, so a retry policy needs a decision before release.
- deferred — `s_error` is cleared only by a successful Start, Stop, or Wakeup commit, so stale errors can stay visible.
- deferred — A backward clock correction can leave the countdown above the phase duration. `remaining_seconds` is not clamped.
- deferred — Any `now > 0` counts as valid UTC. A minimum plausible time is not enforced.
- deferred — An invalid stored record is reset only in memory, so it is reported again on each launch until a Start overwrites it. The generation also restarts at 1.
- deferred — `session_coordinator_deinit` does not flush the queue or unsubscribe the Wakeup handler.
- deferred — The layout uses fixed 144x168 offsets, and the 1 s refresh timer runs while inactive. Both belong with the physical-device pass in Story 1.8.
- deferred — Pause and Skip stay selectable no-ops until Stories 1.4 and 1.6. Pausing and skipping, and phase haptics, stay with Stories 1.4, 1.6, and 1.3.
- deferred — The multi-cycle arithmetic in `prv_reconcile_phase` is an optimization. Replacing it with a single-step loop gives the same result, so no behavioral test can pin it.
- false — `tests/test_pebble_wrapper.py` belongs to the Story 1.1 SDK-pin commit (`7fa97ee`), not to this story's code.

### Review Findings

Code review of `ef33c2f..faa0104` (be85b56, 3cab0b6, 7fa97ee, faa0104), 2026-11-02. Layers: blind-hunter, edge-case-hunter, verification-gap, acceptance-auditor.

- [x] [Review][Patch] A running session can be left with no effective Wakeup and no retry — resolved by decision (option 1): on every `prv_recover` failure exit and failed `prv_wakeup` reschedule, represent the lost Wakeup uniformly (`wakeup_id = -1`, in a form `session_store_validate` and later saves accept), keep the visible error, and add a reconcile/retry trigger that reschedules and reconciles any due deadline once UTC is valid again (e.g. from `get_view` or the coordinator's FIFO). Cover each exit with a test asserting error code, `wakeup_id == -1` and the later retry. [src/c/session_coordinator.c:176-217, 251-300] (sources: blind-hunter+edge-case-hunter+verification-gap+acceptance-auditor; medium)
- [x] [Review][Patch] Tests never check which Wakeup id is cancelled — the `wakeup_cancel` stub discards the id, so Stop or a phase transition cancelling `-1` or the new id still passes on `fake_cancel_count`. Record the last cancelled id and assert it in Start-failure, Stop and transition tests. [tests/session_coordinator_test.c:83-86, 201, 296] (verification-gap; medium)
- [x] [Review][Patch] Menu-row Stop and Start with a due deadline are untested — the reconcile-before-command branches in `EVENT_MENU_SELECT` row 2 and `EVENT_START` are exercised only through `session_coordinator_stop()`. Add menu-row 2 and Start tests with `fake_now >= deadline`. [src/c/session_coordinator.c:312-338] (verification-gap+acceptance-auditor; medium)
- [x] [Review][Patch] Launch-from-Wakeup branch in `session_coordinator_init` is never executed — `launch_reason()` and `wakeup_get_launch_event()` stubs are hard-wired to false. Make them configurable and test that the launch event is queued behind recovery and rejected as stale after `wakeup_cancel_all` and reschedule. [src/c/session_coordinator.c:388-394; tests/session_coordinator_test.c:96-104] (verification-gap; medium)
- [x] [Review][Patch] Recovery failure exits have no test for the surfaced error — only the not-due schedule failure is covered. Add cases for invalid clock, overdue schedule failure and overdue save failure asserting `SESSION_ERROR_CLOCK`, `_WAKEUP` and `_STORAGE`. Also assert `wakeup_id == -1` (per the resolved recovery decision). [src/c/session_coordinator.c:176-198] (verification-gap+edge-case-hunter; low)

#### Rejected

- false — Story 1.1 SDK-pin files (`.pebble-sdk-version`, `scripts/pebble.py`, wrapper tests, README) violate the "pin remains open" decision: they come from `7fa97ee`, a Story 1.1 commit the chosen range includes. The Decision authorises 1.2 to proceed while 1.1 stays in progress; it does not forbid 1.1 work.
- false — Phase progression and overdue recovery belong to Story 1.3: the spec's own tests task lists "overdue reconciliation", AC4 requires due-deadline reconciliation, and the Implementation Notes record that tally and haptics are not included.
- false — Diagnostic harness removal breaks Story 1.1 evidence: the spec's Approach says to replace the harness, and the revision-specific evidence gap is already in `deferred-work.md`.
- false — Wrapper "refuses build/install" claim contradicted by behavior: the guard applies to a superset of commands, which still satisfies the claim.
- false — Generation reaching INT32_MAX blocks Stop: about 2 billion transitions at 60-minute cycles is unreachable.
- low — Spec `status: 'done'` vs sprint `review` and Code Map: the fix edits the spec under review; the status is updated by this workflow's final step anyway.
- low — `pebble.py` guards `sdk`/`clean` commands, runs the SDK check before the usage check, no `uv` or missing pin file handling, regex-style `(active)` matching: cosmetic in a Story 1.1 helper; fix adds branches.
- low — `s_rollover_requested_day` set before enqueue succeeds, and `get_view` enqueues a rollover from draw procs: needs a full queue to bite; fix changes `prv_enqueue`'s signature.
- low — Menu row numbers hard-coded in UI and coordinator, three copies of the due-reconcile block: no current divergence; the test patches above pin the copies.
- low — Host tests depend on `localtime()` timezone: maybe-false, would only be low (settle by running the suite under `TZ=Pacific/Kiritimati` and `TZ=Pacific/Pago_Pago`).
- low — Stale Story 1.1 narrative and rewritten Spec Change Log entry, tally string width, `haptic_generation` unused, `notify_if_missed=false`, no CI for `scripts/test.sh`, no ASan/UBSan: no named harm for 1.2, or owned by Stories 1.3 and 1.8.
- duplicate — Queue overflow drops Stop/Wakeup, minimum plausible UTC, countdown clamp, no-op Pause/Skip, fixed layout offsets, always-on refresh timer, invalid-record reset, deinit not unsubscribing, native MenuLayer untested: already deferred in Review Triage Log rounds 1 and 2 and in `deferred-work.md`.
