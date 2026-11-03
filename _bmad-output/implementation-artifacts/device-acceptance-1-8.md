# Story 1.8 Device Acceptance Protocol and Evidence Record

**Status: not run. No physical result is recorded as passed.** The owner runs this protocol on the Pebble Time 2 and fills in the tables. Story 1.8 stays open, and release stays blocked, until the owner records a release decision in section 12.

Single source for the acceptance criteria: Story 1.8 in `_bmad-output/planning-artifacts/epics.md`. This document does not copy them. It cites each by position: **A1** is the first `Given` block under Story 1.8's Acceptance Criteria, **A2** the second, and so on to **A12** (the last `Given` block, on failed or unverified checks). Open the story beside this file.

Keep emulator and Story 1.1 evidence in `platform-verification.md`. Nothing recorded there counts as a physical result here.

Result column values: `Not run`, `Pass`, `Fail`, `Inconclusive`, `Not observed` (the row could not be observed; the owner gives the reason in the row). Every cell starts as `Not run`. Any row that is not `Pass`, including `Not observed` and `Not run` with a reason, goes to section 12 for owner resolution. This document does not exempt any row.

## 1. Owner-fill fields

Timing-gated rows are A2 to A5 (section 4) and the A6 competing-command row (section 5). They cannot be run until the tolerance and repeat count are filled. Nothing here is a proposed value.

| Field | Value |
|---|---|
| Timing tolerance (`now` minus `dl` on a callback-driven `WAKEUP` line, and other timing rows) | _unset - owner to fill_ |
| Repeat count per timing check | _unset - owner to fill_ |
| Definition of "acceptable" battery and reliability (A9) | _unset - owner to fill; may be left to judgment at review_ |

## 2. Preconditions and identifiers (A1)

Install the finished app from a clean tree at a recorded commit. The build under acceptance must be the build that contains the `APP_LOG` lines (the commit that added this file). Do not accept a different build than the one logged.

Commands, from the project root:

```sh
git rev-parse HEAD
git status --short                                                    # must be clean
uv run --locked python scripts/pebble.py clean
uv run --locked python scripts/pebble.py build 2>&1 | tee build.log; echo "exit=${PIPESTATUS[0]}"
shasum -a 256 build/sit_stand_walk.pbw
uv run --locked python scripts/pebble.py install --phone <ip>         # the physical install
uv run --locked python scripts/pebble.py logs --phone <ip> | tee acceptance-run-1.log   # keep running; one file per run
```

| Item | Value | Result |
|---|---|---|
| Device model | | Not run |
| Firmware / OS version (watch settings) | | Not run |
| Phone model and Pebble app / OS version | | Not run |
| Commit (`git rev-parse HEAD`) and clean-tree confirmation | | Not run |
| Build exit status and compiler warnings (`build.log`) | | Not run |
| Build artifact and SHA-256 (`build/sit_stand_walk.pbw`) | | Not run |
| Pebble Tool version (`uv run --locked pebble --version`) | | Not run |
| Active C SDK (`uv run --locked pebble sdk list`) | | Not run |
| Python (`uv run --locked python --version`) | | Not run |
| Exact install command as run, and its full output and exit status | | Not run |
| Install outcome (app present and launches on the watch) | | Not run |
| Installed build has the log lines: the saved log contains a `WAKEUP_DELIVERED` or `HAPTIC_REQUEST` line from the first deadline or haptic of the run | | Not run |
| Date, time zone and watch local time at start | | Not run |

If the saved log has no such line after the first deadline or haptic of the run, the installed build is not the build under acceptance. Stop and reinstall.

Note: the `.pbw` hash is not known to be stable across builds (see `platform-verification.md`). Record the hash of the file actually installed.

## 3. Log lines and timing arithmetic

The build logs these lines (level INFO), visible with `pebble logs --phone`. They add no behavior, timing or persistence change.

```text
WAKEUP_DELIVERED id=<n> gen=<n>
WAKEUP result=<accepted|early> lc=<n> gen=<n> id=<n> eid=<n> now=<utc> dl=<utc>
WAKEUP result=accepted clock=invalid lc=<n> gen=<n> id=<n> eid=<n> now=<utc> dl=<utc>
WAKEUP result=stale lc=<n> gen=<n> egen=<n> id=<n> eid=<n>
HAPTIC_REQUEST lc=<n> phase=<n> gen=<n>
```

Keys: `lc` lifecycle, `gen` committed generation, `egen` generation carried by the callback, `id` committed Wakeup ID, `eid` Wakeup ID carried by the callback or event, `now` watch UTC, `dl` active deadline. Lifecycle `0` inactive, `1` running, `2` paused, `3` timed pause. Phase `0` none, `1` Sit, `2` Stand, `3` Move. `now` and `dl` are UTC epoch seconds from the watch.

Use of the lines:

- **`WAKEUP_DELIVERED` is the only line that shows a platform callback.** It is logged when the platform hands a Wakeup to the app, before any validation. In `WAKEUP_DELIVERED`, `gen` is the callback's cookie.
- `WAKEUP result=...` lines can also come from app-driven reconciliation (before a command, or the foreground clock check) with no preceding `WAKEUP_DELIVERED`. A `WAKEUP result` line alone does not prove a callback occurred. No `WAKEUP_DELIVERED` line means no callback was logged, which must be read together with whether the log session was connected when the app launched.
- `accepted` matched lifecycle, generation and Wakeup ID. `stale` is rejected before any state change. `early` is accepted but arrived before the deadline, so the coordinator reschedules the same deadline.
- `clock=invalid` lines: exclude them from delta computation. Their `now` is not a valid UTC.
- A `HAPTIC_REQUEST` line means the app called the haptic API. It is not evidence the watch vibrated. Record **requested** (from the log) and **observed** (felt by the owner) in separate columns. A mismatch is a finding, not a rounding error.
- The log does not measure haptic latency. `HAPTIC_REQUEST` has no watch time and no cause. Infer the cause from line order (for example, the preceding `WAKEUP result=accepted`, or none for a command).
- Foreground behavior is observed by eye. It is not logged. Record what was on screen when the deadline passed.
- Timing delta, by hand: `delta = now - dl`, both read from the same `WAKEUP result` line, excluding `clock=invalid`. Positive is late. A negative value appears only with `result=early`. Use only lines preceded by a `WAKEUP_DELIVERED` for the same `id` when the check is about the platform callback. For the phase chain, also compare consecutive `dl` values against the expected 45 min (2700 s), 10 min (600 s) and 5 min (300 s), adjusted for any Extend, Skip, pause or recovery in that run.
- The log does not carry a Start line. The first `dl` after Start is the reference. Note the wall time at which Start was pressed.
- Log timestamps come from the logging host, not the watch. Do not mix them with `now`/`dl` epoch values in one delta.

Save the full log output for each run and name the file in the Evidence column.

## 4. Cycle and Wakeup timing (A2, A3, A4, A5)

The timing-gated rows listed in section 1 (A2 to A5, and the A6 competing-command row) need section 1 filled. Until then they stay `Not run` and are not runnable.

### A2 - Complete 45/10/5 cycle

Start a session at valid UTC and use the watch normally until the Move deadline has passed (60 minutes after Start plus delay). Run it the number of times set in section 1.

| Run | Phase reached | `dl` (log) | `now` (log) | delta | Remaining time shown | Haptic requested (log) | Haptic observed (felt) | App open / foreground | Result | Evidence |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Sit to Stand | | | | | | | | Not run | |
| 1 | Stand to Move | | | | | | | | Not run | |
| 1 | Move to Sit | | | | | | | | Not run | |

Add rows for each repeat.

### A3 - Ordinary phase deadline with the app left

Leave the app normally (Back to the watchface) so it is no longer foregrounded. Do not force-terminate it. Record separately for each phase deadline.

| Phase deadline | `WAKEUP_DELIVERED` logged | `now` / `dl` / delta | Haptic requested (log) | Haptic observed (felt) | App opened / foregrounded | Phase and remaining time on open | Result | Evidence |
|---|---|---|---|---|---|---|---|---|
| Sit end | | | | | | | Not run | |
| Stand end | | | | | | | Not run | |
| Move end | | | | | | | Not run | |

Timing is compared with the section 1 tolerance and repeat count. Record the tolerance used with the result.

### A4 - Timed pause end, left in the background

From a running session, press the middle (Select) button. That starts a 30-minute timed pause. While timed paused, Top and Bottom change the length. The on-screen `TOP %dm BOTTOM %dm` line shows the two alternatives for the current length (see section 9). Reach each length this way, then confirm the shown `BREAK` countdown matches it:

- 30 minutes: Select once from running.
- 5 minutes: Select, then Top.
- 10 minutes: Select, then Bottom.

Note the remaining phase time shown, leave the app normally (do not force-terminate it), and wait for pause end. Record the log `dl` after the length change as the pause deadline.

| Pause | Remaining before pause | `WAKEUP_DELIVERED` logged / delta | Same phase resumed | Remaining time resumed | Cycle advanced? | Haptic requests (log count) | Haptic observed (felt) | App opened / foregrounded | Result | Evidence |
|---|---|---|---|---|---|---|---|---|---|---|
| 5 min | | | | | | | | | Not run | |
| 10 min | | | | | | | | | Not run | |
| 30 min | | | | | | | | | Not run | |

"Meets the agreed tolerance" is judged against section 1.

### A5 - Early resume and overdue pause-end recovery

| Case | Steps | Record | Result | Evidence |
|---|---|---|---|---|
| Resume before pause end | Start a timed pause (Select), resume from the middle control or menu before it ends. | Same phase resumed with saved remaining time. `HAPTIC_REQUEST` count (expected 0). Whether the app reopened later. | Not run | |
| Stale callback after early resume | Wait past the original pause deadline. | `WAKEUP_DELIVERED` and `WAKEUP result=stale` lines present or absent. `HAPTIC_REQUEST` count (expected 0). No app reopen. | Not run | |
| Overdue pause end | Start a timed pause, exit the app, leave it until after the pause deadline without opening it, then open it. (If the Wakeup already reopened the app, record that.) | Resumes once. Phase not advanced. `HAPTIC_REQUEST` count (expected at most 1). | Not run | |

Expected `HAPTIC_REQUEST` counts for A4 and A5: timed-pause end 1 (at most 1), early resume 0, overdue pause-end recovery at most 1.

Record whether a `stale` line appeared. Absence is recorded as observed, with no conclusion drawn beyond what the log and the screen show.

## 5. Controls and competing events (A6)

Record phase, remaining time, `HAPTIC_REQUEST` count and `WAKEUP` lines for each. The expected contract: Start, Pause, Resume, Stop and Extend are silent. Skip and an automatic transition each request one haptic. Expected `HAPTIC_REQUEST` counts: Start 0, Pause 0, Resume 0, Stop 0, Extend 0, Skip 1, automatic transition 1.

| Case | Steps | Expected phase / Wakeup / stale / haptic behavior to check | Result | Evidence |
|---|---|---|---|---|
| Start | Start from inactive. | Sit, 45:00. No `HAPTIC_REQUEST`. | Not run | |
| Manual Pause / Resume | Pause, then Resume. | Remaining time frozen then continued. No `HAPTIC_REQUEST`. Old callback shows `stale`, if delivered. | Not run | |
| Stop | Stop while running, while paused, and from a timed pause. | Returns to inactive. No later `WAKEUP` accepted, no later haptic. | Not run | |
| Extend +5 and +10 | Press top, then bottom, in running. | Deadline moves by the amount and stacks. No `HAPTIC_REQUEST`. `WAKEUP` for the old deadline shows `stale`. New deadline in later `WAKEUP` lines. | Not run | |
| Skip | Skip from running, paused and timed pause. | Next phase at its default duration. Exactly one `HAPTIC_REQUEST`. Old callback `stale`. | Not run | |
| Competing command and deadline | Press a control close to a phase deadline. | Deadline reconciles first, then the command applies to the resulting phase. Record phase shown and `HAPTIC_REQUEST` count. | Not run | |
| Phase Pause / Stop cancellation | Pause or Stop shortly before a deadline, then wait past it. | No transition, no haptic after the original deadline. | Not run | |
| Extension rescheduling | Extend, then wait for the extended deadline. | Transition occurs at the extended deadline only. | Not run | |

## 6. Recovery and clock cases (A7)

Clock changes need a way to move watch time. Record the method used. If the owner cannot change time, mark those rows `Not run` with the reason.

| Case | Steps | Record | Result | Evidence |
|---|---|---|---|---|
| Overdue phase recovery | Start, exit, leave the app until at least one phase deadline passes, then reopen. | Phases reconciled in order. Resulting phase shown with full default duration from recovery time. One `HAPTIC_REQUEST` at most. Tally counts. | Not run | |
| Invalid UTC at Start | Start with invalid time, if the watch can be placed in that state. | Stays inactive. Clock error text visible. | Not run | |
| Invalid UTC while active | Make UTC invalid during a session, if possible. | Active state preserved. No scheduling or haptic until valid time returns. Reconciles after. | Not run | |
| Forward clock correction | Move time forward past the deadline. | Deadline treated as due. | Not run | |
| Backward clock correction | Move time backward. | Deadline deferred. | Not run | |

## 7. Operational limits and sustained use (A8)

| Item | How observed | Record | Result | Evidence |
|---|---|---|---|---|
| At most eight scheduled Wakeups | Code keeps one persisted `wakeup_id`. During repeated use, read `id=` and `eid=` in `WAKEUP` lines; use `WAKEUP_DELIVERED` for callback occurrence. | Distinct live IDs seen. Any scheduling error text. | Not run | |
| At least one-minute spacing | Press Skip or Extend repeatedly in quick succession and across a due deadline. | Any "Reminder scheduling failed" error, with the action that caused it. | Not run | |
| `notify_if_missed = false` | Source: `wakeup_schedule(..., false)` in `prv_schedule`, `src/c/session_coordinator.c`. | Source line reference at the tested commit. On device: whether any notice appeared for a deadline missed while the watch was off or out of range, if tested. | Not run | |
| Reliability over sustained use | Use the app per the intended personal-use pattern for the period the owner chooses. | Period, deadlines expected, `WAKEUP_DELIVERED` lines logged, misses. | Not run | |
| Battery behavior | Same period. | Start and end battery level, period, other watch use. | Not run | |

## 8. Owner review of reliability and battery (A9)

| Item | Entry |
|---|---|
| Observations reviewed (sections 4, 7) | |
| Acceptable for release per the intended personal-use pattern? (`Yes` / `No` / `Inconclusive`) | _not recorded_ |
| Owner, date | |

Any `No` or `Inconclusive` keeps Story 1.8 open pending more testing or a product decision.

## 9. Finished UI and hardware controls (A10)

Judge fit and legibility on the watch without relying on color. Layout under test is in `src/c/sit_stand_move.c` (`prv_window_load`, `prv_render`).

| Element | What to check | Result | Evidence |
|---|---|---|---|
| Current time (14pt) | Legible, no clipping | Not run | |
| Countdown (30pt, `MM:SS`) | Fits, legible, no wrapping | Not run | |
| Phase icon and label (Sit / Stand / Move) | Phase readable from text | Not run | |
| `PAUSED: <phase>` label | Longest case (`PAUSED: Stand`) fits | Not run | |
| `BREAK` label | Fits, distinct from `PAUSED` | Not run | |
| Running controls line `TOP +5 BOTTOM +10` | Fits on one line, buttons match | Not run | |
| Timed-pause controls line `TOP %dm BOTTOM %dm` | Per `prv_render`: 5-minute entry shows `TOP 10m BOTTOM 30m`, 10-minute `TOP 5m BOTTOM 30m`, 30-minute `TOP 5m BOTTOM 10m`. Record what is shown and whether buttons match | Not run | |
| Daily tally (`Sit n  Stand n  Move n`) | Fits with larger counts | Not run | |
| Error text | Each string fits the fixed layer: `Clock time unavailable`, `Session save failed`, `Reminder scheduling failed`, `Session data invalid`, `Please retry` (some may need a forced condition; record which were seen) | Not run | |
| Native menu | Rows Start / Pause or Resume / Skip / Stop; native callbacks respond | Not run | |
| Hardware buttons | Up, Select, Down in running, paused and timed pause; long Select opens the menu | Not run | |

## 10. Implementation-test evidence for failure injection (A11)

A11 does not require injecting failures on the watch. The evidence is the host test suite. Run `sh scripts/test.sh` at the commit under acceptance and record the result:

| Item | Entry |
|---|---|
| Commit | |
| `sh scripts/test.sh` result | Not run |
| Reviewer confirms mapping below against the test source | Not run |

All names are `static void` test functions in `tests/session_coordinator_test.c`. "Result" is for the owner's review of the evidence at the commit under acceptance.

| Contract in A11 | Host tests | Result |
|---|---|---|
| Committed lifecycle/phase state retained on persistence or scheduling failure | `test_clock_schedule_and_storage_failures`, `test_stop_storage_failure_keeps_committed_session`, `test_stop_failure_keeps_persisted_session`, `test_skip_failures_retain_committed_state`, `test_pause_resume_failures_retain_committed_state`, `test_timed_pause_failure_retains_committed_state`, `test_extend_failures_retain_committed_state`, `test_failed_transition_has_no_tally_or_haptic` | Not run |
| Wakeup identity, generation and callback acceptance | `test_stale_callback_is_ignored`, `test_schedule_error_codes_and_cookie`, `test_early_callback_reschedules_same_deadline`, `test_skip_cycles_phases_without_counting_and_ignores_stale_callback`, `test_pause_resume_freezes_persists_and_rejects_stale_callback`, `test_extend_ignores_stale_callback_and_persists_across_restart`, `test_timed_pause_entry_change_resume_and_stale_callback` | Not run |
| Candidate cleanup and retry | `test_recovery_failure_exits_report_error_and_retry`, `test_clock_check_retries_failed_due_transition`, `test_pause_does_not_commit_when_due_reconciliation_fails`, `test_overdue_pause_end_recovery_does_not_cancel_invalid_id` | Not run |
| Launch recovery and crash boundaries (persisted record, restart) | `test_launch_repair_and_invalid_record`, `test_wakeup_launch_event_is_queued_behind_recovery_and_stale`, `test_paused_record_recovers_without_wakeup`, `test_store_round_trip`, `test_store_rejects_invalid_records`, `test_timed_pause_store_validation`, `test_invalid_record_is_persisted_as_error_until_replaced` | Not run |
| Visible errors | `test_recovery_failure_is_not_presented_as_scheduled`, `test_control_enqueue_failure_is_reported`, `test_skip_invalid_utc_preserves_committed_state`, `test_extend_invalid_or_overflowing_utc_preserves_committed_state`, `test_timed_pause_invalid_utc_preserves_committed_state`, `test_pause_resume_reject_invalid_or_overflowing_clock` | Not run |
| At-most-once haptic request | `test_skip_restart_does_not_replay_haptic`, `test_pause_end_recovery_failures_and_restart_haptic_marker`, `test_natural_cycle_tally_and_haptics_persist`, `test_recovery_counts_each_elapsed_phase_once`, `test_wakeup_catch_up_table` | Not run |

Known gap: the host harness does not compile `sit_stand_move.c`, so the native `MenuLayer` callbacks, click handlers and layout are only covered by sections 5 and 9.

## 11. Carried-over device checks from `deferred-work.md`

| Source (story) | Check | Result | Evidence |
|---|---|---|---|
| Story 1.7 | Extend labels (`TOP +5 BOTTOM +10`) fit and legible; running top/bottom Extend presses work; Extend Wakeup fires at the new deadline. Covered by sections 5 and 9, record here the cross-reference. | Not run | |
| Story 1.6 | Skip menu row dispatches from PAUSED, BREAK and running; the resulting view and the Skip haptic. | Not run | |
| Story 1.6 | Tap Skip at a due phase deadline. Two haptic requests occur back to back (the reconciled transition, then Skip). Record the `HAPTIC_REQUEST` count and what is felt: played, merged, or dropped. The owner already chose to keep this behavior. | Not run | |
| Story 1.5 / 1.3 | A PauseEnd Wakeup raises the app from the menu window, and from the background. The SDK has no separate foreground call. | Not run | |
| Story 1.1 / 1.2 | Error text fits the fixed-height layer (see section 9). | Not run | |
| Story 1.2 | Native `MenuLayer` selection callbacks work on the device. | Not run | |
| Story 1.3 | Stop blocked while a due transition keeps failing to schedule. Only if real scheduling failures appear in testing; otherwise record `Not observed`. The owner accepted the current behavior. | Not run | |

## 12. Release decision (A12)

Fill in only after sections 2 to 11 are complete.

| Item | Entry |
|---|---|
| Rows not `Pass` (list them, with limitation and evidence) | _not recorded_ |
| Owner resolution of affected scope or acceptance | _not recorded_ |
| Release decision (`Release` / `Do not release`) | _not recorded_ |
| Owner, date | |

**Rule:** If any required row is not `Pass`, record the limitation and evidence. Story 1.8 stays open and release is blocked until the product owner resolves the affected scope or acceptance. Do not claim physical acceptance passed. Story 1.1 also stays open. This record does not close it.
