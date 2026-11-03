---
title: 'Set the length of Sit, Stand, and Move in Settings'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '81e7dccf9610a706fe0b3b4c06b32ac0788aefa6'
context:
  - _bmad-output/implementation-artifacts/epic-4-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The phase lengths are fixed at 45/10/5 minutes in `prv_duration()`. The owner wants to set each length in Settings.

**Approach:** Add a small settings module that stores one length per phase under its own persist key. The coordinator's `prv_duration()` reads it whenever a phase starts. The Settings window gets three rows, `Sit`, `Stand`, `Move`, below the four haptic rows. Selecting one adds 5 minutes and wraps from 60 to 5. Reset is Story 4.2.

## Boundaries & Constraints

**Always:** Defaults 45/10/5; allowed values 5..60 in steps of 5. Lengths are read only when a phase starts: automatic transition, Skip, Start, and the overdue-recovery rebase. Own persist key; the session record schema and `session_store.c` stay unchanged. The UI writes the setting and never computes deadlines. Keep the phase name `Move`. Haptic rows behave exactly as in Stories 3.1/3.2. Stored values are validated per phase; an invalid one falls back to that phase's default with no error.

**Never:** Touch a running phase's deadline, a paused phase's remaining time, Wakeups, timed-pause choices (5/10/30), Extend amounts, or the tally rules. No haptic preview on length rows. No new window. No Reset row (Story 4.2).

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Defaults | No stored value | Sit 45, Stand 10, Move 5; Start sets deadline now+45 min | N/A |
| Step | Sit 45 | Select: Sit 50, saved | N/A |
| Wrap | Stand 60 | Select: Stand 5, saved | N/A |
| Applied later | Running Sit, Stand set to 20 | Sit deadline unchanged; next Stand ends 20 min after it starts | N/A |
| Skip / transition | Move set to 15 | Phase after Stand ends gets now/deadline + 15 min | N/A |
| Invalid stored | Wrong size, 0, 65, or 7 for one phase | That phase uses its default; the others keep valid values | No error shown |
| Save fails | `fake_fail_write` on step | Row and stored value stay unchanged | Step returns false |
| Overdue day | Sit 30, Stand 5, Move 5, deadline before today | Fast-forward in whole cycles of 40 min (sum), phase preserved | N/A |

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c` -- `prv_duration()` (`:677`) hardcodes 45/10/5 and feeds cycle advance (`:94`), rebase (`:100`), Skip (`:426`). `prv_start()` (`:176`, `:188`) uses `SESSION_DURATION_SECONDS`; switch both to `prv_duration(SESSION_PHASE_SIT)` and drop the unused enum entry. `prv_reconcile_phase` (`:86-89`) fast-forwards in whole `3600` s cycles; use the sum of the three configured lengths instead.
- `src/c/haptic_settings.c/.h` -- the pattern to copy: own key (`0x5354`), `get` with validation, `set` returning bool.
- `src/c/sit_stand_move.c` -- Settings window `prv_settings_*` (`:238-280`): `PATTERN_COUNT` rows, `prv_settings_select` pops on success. Needs 7 rows, formatted length rows, and a no-pop step path that reloads the menu.
- `tests/session_coordinator_test.c` -- persist stubs (`:60-105`, `HAPTIC_KEY`, `fake_reset`) need the new key; `prv_expected_duration` (`:806`) and tables at `:817`, `:847` assume 45/10/5 and stay valid for defaults.
- `scripts/test.sh` -- compile line lists source files; add the new `.c`.
- `session_store.c` -- not changed.

## Tasks & Acceptance

**Execution:**
- [ ] `src/c/phase_settings.h`, `src/c/phase_settings.c` -- new: `phase_settings_get_minutes(SessionPhase)`, `phase_settings_step(SessionPhase)` (add 5, wrap 60->5, return false on write failure), one 3-byte key `0x5355`, per-phase validation with default fallback -- own key, atomic record for 4.2
- [ ] `src/c/session_coordinator.c` -- `prv_duration()` returns configured minutes * 60; Start uses it; cycle fast-forward uses the configured sum -- single lookup
- [ ] `src/c/sit_stand_move.c` -- 7 settings rows; length rows show `Sit 45 min`; select steps and reloads without popping; haptic rows unchanged
- [ ] `tests/session_coordinator_test.c`, `scripts/test.sh` -- stub for the new key; tests for every matrix row, per-phase fallback, persistence across Stop/Start and relaunch, unchanged running phase, tally, and unchanged existing tests
- [ ] `_bmad-output/planning-artifacts/ux-designs/ux-sit_stand_go-2026-10-23/DESIGN.md`, `EXPERIENCE.md` -- add the length rows to the Settings window entries

**Acceptance Criteria:**
- Given no stored lengths, when a session starts and cycles, then timing equals today's 45/10/5 and all existing tests pass.
- Given a changed length, when the next phase starts, then it uses the new length and the running phase is unchanged.
- Given Settings opens, then it shows four haptic rows then `Sit`, `Stand`, `Move` with values as text.

## Implementation Notes

- New `src/c/phase_settings.c/.h`: one 3-byte record (Sit, Stand, Move minutes) under key `0x5355`, validated per byte (5..60, multiple of 5); `phase_settings_step()` adds 5, wraps 60 to 5, returns false on a failed write. A record of the wrong size is untrustworthy as a whole, so all three phases read their defaults.
- `session_coordinator.c`: `prv_duration()` reads the configured minutes; `prv_start()` uses it and the `SESSION_DURATION_SECONDS` entry is gone; the overdue fast-forward jumps in whole cycles of the configured sum instead of a fixed 3600 s. `session_store.c` and the record schema are unchanged.
- `sit_stand_move.c`: Settings has seven rows; length rows read `Sit 45 min`, step in place via `menu_layer_reload_data`, no preview, window stays open.
- Tests: nine `test_phase_lengths_*` tests plus the new key in the persist stub and `scripts/test.sh`. Two deliberate breaks (fixed 3600 back in the fast-forward; hardcoded 45/10/5 in `prv_duration()`) were each caught. `DESIGN.md` and `EXPERIENCE.md` describe the length rows.
- Matrix row "Invalid stored: wrong size ... for one phase" cannot hold per phase with one record. Value errors (0, 65, 7) are isolated per phase; a wrong-sized record resets all three to defaults. Both are tested.
- Not built, not seen on a display: the Pebble SDK is not available in this sandbox. `sit_stand_move.c` is never compiled by the host harness.

## Spec Change Log

## Review Triage Log

- Overdue fast-forward uses the current cycle sum after a mid-session length change (Blind, Edge): false. The jump starts at the end of the current phase, and every phase after that starts later, so the new lengths are the ones FR14 says apply.
- Mixed haptic and length rows with no separator (Blind): low, not fixed. The row order is the owner-approved UX-DR21; a separator or header would be a new design decision.
- Scroll position and clipping of `Stand 60 min` after `menu_layer_reload_data` (Blind): maybe-false, deferred. Needs the emulator or Pebble Time 2.
- Lengths can only step upward (Blind): false. The step-and-wrap gesture was confirmed by the owner (UX-DR22).
- Failed save gives no cue (Blind, Edge, Verification Gap): low, not fixed. The spec and UX-DR22 require only that the old value stays shown, same as the haptic rows in Story 3.1.
- No automated test for the Settings window code (Blind, Verification Gap): deferred. The host harness has no MenuLayer stubs; same gap as Stories 1.6, 1.7, 2.1, 3.1, 3.2.
- `get_minutes` of an invalid phase returns Move's value (Blind, Edge): low, not fixed. `prv_duration()` only receives Sit, Stand, or Move, and the old code returned the Move length in the same case.
- Storage read on every call, three times in the fast-forward (Blind): low, not fixed. Single-threaded, the loop is bounded to one day, and the read is a few bytes.
- Transient read failure during a step would overwrite the other phases with defaults (Edge): low, not fixed. A failing read of an existing record of the right size is not shown to occur, and the fix adds a guard.
- Wrong-sized record resets all three phases, not one (Edge): false as a defect. One record cannot be partly wrong-sized; the matrix wording is noted in Implementation Notes. Tested for 2 and 4 bytes.
- Test calls `prv_set_minutes(45, 10, 15)` twice (Blind): medium-low, fixed. The second call was a no-op and is removed.
- Hardcoded 2400/300/600 in the overdue test, bare `assert` (Blind): false. An independent model should not derive its expectations from the code under test, and the harness is built without `NDEBUG`.
- No test for Extend at the 60-minute maximum (Blind): false. Extend amounts are fixed (+5/-5) and unrelated to phase lengths; `test_phase_lengths_do_not_change_pause_choices_or_extend` covers them.
- Change not traced to the PRD or architecture (Blind): deferred. PRD section 7.2 and AD-6 still say lengths are fixed.

## Design Notes

One key, three bytes (minutes for Sit, Stand, Move) keeps Reset in Story 4.2 a single write, so no half-reset state exists. Validation is per byte so one corrupt phase does not reset the others.

## Verification

**Commands:**
- `sh scripts/test.sh` -- expected: all host tests pass (builds with `-Wall -Wextra -Werror -fshort-enums`)

**Manual checks (if no CLI):**
- The Pebble SDK is not available in this sandbox. The UI file and the 7-row Settings window cannot be compiled or seen here; state that as unverified.
