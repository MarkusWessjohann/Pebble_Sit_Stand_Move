---
title: 'Verify the finished app on Pebble Time 2 before release'
type: 'chore'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '062e537513db1d0a28366424a6ca4d75206501d3'
context:
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Stories 1.2–1.7 are tested only on a host harness and, for 1.1, an emulator. Nothing shows that Wakeup, haptic, foregrounding, timing, the finished UI or battery behave on a physical Pebble Time 2. No procedure or record exists for collecting that evidence, and the agent cannot touch the watch.

**Approach:** Write an operator-run acceptance protocol with an evidence template covering every Story 1.8 acceptance criterion, plus the carried-over device checks from `deferred-work.md`. Add minimal `APP_LOG` lines so `pebble logs --phone` shows Wakeup callbacks and haptic requests. The owner runs the protocol on the watch and records results. The story stays open until the owner records a release decision.

## Boundaries & Constraints

**Always:** Cover every Story 1.8 criterion in `epics.md` and cite it by position. Record device model, firmware/OS, commit, `.pbw` hash and CLI/SDK identifiers, and install through `uv run --locked python scripts/pebble.py install --phone <ip>`. Keep emulator/Story 1.1 evidence separate from physical results. Every result cell starts as `Not run`. Map the failure-injection criterion to existing host tests by name, since it needs no on-device injection.

**Decision (owner):** The protocol has two owner-fill fields, timing tolerance and repeat count, unset until the owner fills them. Timing rows are not runnable before that.

**Decision (owner):** Add `APP_LOG` lines for Wakeup callback (accepted, stale, early) and haptic request. Logs only: no behavior, timing or persistence change. The logged build is the build under acceptance. A requested haptic (log) is recorded separately from an observed one (felt). Callback occurrence comes from the log.

**Decision (owner):** Deliverable is the protocol document plus the log lines. No log-timing helper script; deltas are computed by hand in the template.

**Never:** Mark any physical check passed, invent a tolerance, repeat count or pass criterion, claim Story 1.8 or 1.1 done, change coordinator behavior or timing beyond the log lines, add product surfaces, or copy the acceptance criteria wholesale (`platform-verification.md` points to `epics.md` as their single source).

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Check passes | Observation within agreed tolerance | Row records value, timestamp, evidence source | N/A |
| Check fails or unrun | Any required row not `Pass` | Limitation recorded; story stays open; release blocked | Owner resolves scope |
| Inconclusive battery/reliability | Observations unclear | Gate stays open; no acceptance claimed | Owner decides |

</frozen-after-approval>

## Code Map

- `_bmad-output/planning-artifacts/epics.md` (Story 1.8, `:443`) -- the 13 acceptance criteria; sole source. NFR3/NFR9 `:49,:61` and UX-DR6/7/10/11 define legibility and button checks.
- `_bmad-output/implementation-artifacts/platform-verification.md` -- "Pre-release physical-device acceptance" and Results tables already name Story 1.8 and open items. Link to the protocol from here; do not duplicate.
- `_bmad-output/implementation-artifacts/deferred-work.md` -- device checks to fold in: Extend label fit and presses (1.7); Skip row dispatch and double haptic at a due deadline (1.6); PauseEnd foreground from menu window and background (1.5); error-text fit; native MenuLayer callbacks; Stop blocked while due transition keeps failing (1.3).
- `src/c/sit_stand_move.c` -- UI under test: layers at `:240-273` (time 14pt, countdown 30pt, phase 24pt, controls 14pt, tally 14pt, error 14pt), `TOP +5 BOTTOM +10`, `TOP %dm BOTTOM %dm`, `BREAK`, `PAUSED: <phase>`. Has no `APP_LOG`.
- `src/c/session_coordinator.c` -- `prv_request_haptic` (`:100`, `vibes_short_pulse`): log here, so all five call sites are covered. `prv_wakeup` (`:707`): log accepted, stale (first `return`), and early (`now < deadline`) callbacks with lifecycle, generation, `now` and deadline. `wakeup_schedule(..., false)` (`:126`, `notify_if_missed = false`); one persisted `wakeup_id`. No `APP_LOG` exists yet.
- `tests/pebble-stubs/pebble.h` -- host stub; has no `APP_LOG`. Add a no-op macro so `scripts/test.sh` still compiles with `-Wall -Wextra -Werror` (unused-argument safe).
- `tests/session_coordinator_test.c` -- 71 host tests (all pass via `scripts/test.sh`); source for the failure-injection criterion mapping (schedule/persist/cancel/launch-repair/haptic-marker cases).
- `README.md` -- pinned build/install commands; `.pebble-sdk-version` 4.33.1, Tool 5.0.40.

## Tasks & Acceptance

**Execution:**
- [x] `src/c/session_coordinator.c`, `tests/pebble-stubs/pebble.h` -- add `APP_LOG` lines (haptic request; Wakeup accepted/stale/early) and a no-op host stub -- makes callback and haptic request observable on the watch
- [x] `_bmad-output/implementation-artifacts/device-acceptance-1-8.md` -- protocol and evidence template: preconditions, one section per criterion with steps, what to record (incl. log line format and hand-computed timing delta), `Not run` result column, owner release-decision block -- the owner executes it on the watch
- [x] `_bmad-output/implementation-artifacts/device-acceptance-1-8.md` -- failure-injection mapping table (criterion to host test names) and deferred device checks -- covers the review-evidence criterion without on-device injection
- [x] `_bmad-output/implementation-artifacts/platform-verification.md` -- add a pointer from the Story 1.8 section to the protocol -- single place to find it

**Acceptance Criteria:**
- Given the log lines, when `scripts/test.sh` runs, then all existing tests still pass and coordinator state, persistence, Wakeup scheduling and haptic count are unchanged.
- Given the protocol, when compared with Story 1.8, then each of its 13 criteria has at least one section and no physical result is marked passed.
- Given a section that needs a tolerance or repeat count, when it is read, then it states the owner-agreed value or an unset field, never an invented one.
- Given the mapped host tests, when `scripts/test.sh` runs, then every test name in the mapping table exists and passes.
- Given any row not `Pass`, when the decision block is read, then it states that Story 1.8 stays open and release is blocked.

## Implementation Notes

- `prv_request_haptic` logs `HAPTIC_REQUEST` (covers all five call sites). `prv_wakeup` logs `WAKEUP result=stale|early|accepted`, plus `accepted clock=invalid` for the invalid-UTC branch. No behavior, timing or persistence change. The host stub `APP_LOG` is never evaluated but keeps arguments format-checked under `-Werror`.
- `device-acceptance-1-8.md` has sections A1-A12. `epics.md` has 12 `Given` blocks, not 13 as the spec said; nothing is missing against the source.
- Owner-fill fields (tolerance, repeat count, battery/reliability definition) are unset. Every result cell is `Not run`; no `Pass` appears.
- A missing `stale` line is recorded as an observation, not a failure: the platform may never deliver a callback for a cancelled Wakeup. Log timestamps come from the logging host, not the watch.
- The new `APP_LOG` lines were not compiled with the real Pebble SDK (CLI unavailable in the sandbox, wrapper blocks on inactive SDK 4.33.1). Build on the host before the device run.
- `sprint-status.yaml` set to `in-progress` at step 3 start.

## Spec Change Log

## Review Triage Log

- medium (patch) — `prv_wakeup` is also called synthetically from the pre-command reconcile (`session_coordinator.c:206`) and the foreground clock check (`:856`); both log `WAKEUP result=accepted`, so a log line is not proof of a platform callback and A3/A4 could be marked from app-driven reconciliation. Log the platform delivery separately in `session_coordinator_wakeup` and say so in section 3. (edge-case-hunter, blind-hunter)
- medium (patch) — Only `result=stale` carries `id=`/`event_id=`; section 7 tells the owner to read `id=` from `WAKEUP` lines for the Wakeup-limit check. Add ids to the accepted/early/invalid-clock lines, keeping each line short. (edge-case-hunter)
- medium (patch) — No test reads the new log lines; inverting the early/accepted ternary or deleting the haptic log leaves all tests green. Add a capturing `APP_LOG` stub and a test for stale, early, accepted, delivery and haptic lines. (verification-gap)
- medium (patch) — A4 says "select a timed pause from the menu" for 5/10/30 minutes; the menu has no such row. Select starts the 30-minute pause, Top/Bottom change the length. (edge-case-hunter)
- medium (patch) — Section 12 gate omits section 10 (A11 evidence), so a release decision could be recorded without reviewing it. (blind-hunter, edge-case-hunter)
- medium (patch) — Result values lack a state for rows that cannot be observed (`Not observed` is used in section 11); say any non-`Pass` row goes to section 12 for owner resolution. (blind-hunter, edge-case-hunter)
- low (patch, doc) — A2 "about 60 minutes" cannot reach the Move deadline; A6 "within about a second" is an unapproved parameter; timing-gated rows are listed inconsistently; `clock=invalid` `now` is not a usable delta input; expected haptic counts for early resume, pause end and overdue recovery are unstated; Stop is not tested from timed pause; no log capture command or build-identification check. (blind-hunter, edge-case-hunter, verification-gap)
- low (rejected) — Failures after an accepted callback are not logged. The on-screen error is the specified surface (UX-DR10); one log per failure path adds many sites for a transient state. (blind-hunter, edge-case-hunter)
- low (rejected) — `HAPTIC_REQUEST` lacks cause and watch `now`. Cause follows from line order; the doc will state that haptic latency is not measured by the log. (blind-hunter)
- low (rejected) — A-numbers are fragile; add titles. Cosmetic; the document is a one-release record. (blind-hunter)
- low (rejected) — No steps for phone developer connection or finding `<ip>`. Owner already installs to the watch this way. (blind-hunter)
- low (rejected) — Stale line near the 120-byte log limit. Resolved by the shortening patch above, not a separate change. (blind-hunter)
- rejected (edits the spec) — Spec says 13 criteria, `epics.md` has 12. Recorded in Implementation Notes; the protocol covers all 12. (blind-hunter, verification-gap, edge-case-hunter)

## Design Notes

Story 1.8 cannot reach `done` from this build. The build delivers the runnable protocol and leaves `sprint-status` at `in-progress`; `done` needs the owner's recorded results and release decision.

## Verification

**Commands:**
- `sh scripts/test.sh` -- expected: host C and Python tests pass.
- `rg -c "^static void test_" tests/session_coordinator_test.c` plus a name lookup -- expected: every mapped test name exists.
- `git diff --check` -- expected: no whitespace errors.

**Manual checks (if no CLI):**
- Read the protocol against `epics.md` Story 1.8 and confirm every criterion is covered and every result cell is `Not run`.
