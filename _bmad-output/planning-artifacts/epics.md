---
status: final
updated: 2026-11-03
epic3Pass: step-04-final-validation  # Epic 3 (settings): validated and complete
epic4Pass: step-04-final-validation  # Epic 4 (phase lengths): validated and complete
stepsCompleted:
  - step-01-validate-prerequisites
  - step-02-design-epics
  - step-03-create-stories
  - step-04-final-validation
inputDocuments:
  - _bmad-output/planning-artifacts/prds/prd-sit_stand_go-2026-10-21/prd.md
  - _bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md
  - _bmad-output/planning-artifacts/ux-designs/ux-sit_stand_go-2026-10-23/DESIGN.md
  - _bmad-output/planning-artifacts/ux-designs/ux-sit_stand_go-2026-10-23/EXPERIENCE.md
  - _bmad-output/planning-artifacts/briefs/brief-sit_stand_go-2026-10-21/brief.md
---

# sit_stand_go - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for sit_stand_go, decomposing the requirements from the PRD, UX Design, and Architecture requirements into implementable stories.

## Requirements Inventory

### Functional Requirements

FR1: The user can manually start and stop a session on Pebble Time 2. With valid UTC, Start begins Sit immediately for 45 minutes, measured from the actual start time; invalid UTC leaves the session inactive and displays a clock-time error. Stopping makes the session inactive and prevents later reminders; a later Start begins a new session at Sit.

FR2: During an active, unpaused session, phases repeat in order Sit (45 minutes), Stand (10 minutes), Move (5 minutes), then Sit. Phase deadlines are absolute UTC epoch times and are relative to actual session start, not aligned to wall-clock hour boundaries. On a normal deadline, advance to the next phase at its default duration. When recovery finds a deadline due (`now >= deadline`), process elapsed phases in order and rebase the resulting phase to its full default duration from recovery time.

FR3: At an on-time automatic phase transition, request one haptic and open/foreground the app UI to show the new phase and remaining time, subject to Pebble Time 2 firmware capability. On overdue recovery, request at most one haptic for the reconciled current phase and do not replay stale reminders. Stopped sessions produce no subsequent reminders.

FR4: The user can manually pause and resume an active session. Manual Pause freezes the active phase and preserves its remaining duration, has no automatic resume deadline, and produces no haptic. Resume continues the same phase with the saved remaining duration and produces no haptic.

FR5: The user can start a timed pause for exactly 5, 10, or 30 minutes. The phase clock is suspended and its remaining duration is preserved. At pause end, resume the same phase with the saved remaining duration, do not advance the cycle, request at most one haptic, and request app open/foreground. If the user resumes early, cancel the pause Wakeup; a stale callback must neither request a haptic nor reopen the app. Overdue timed-pause recovery resumes the preserved phase once without cycle advancement.

FR6: The user can skip the active phase. Skip advances immediately to the next phase in the cycle at that phase's default duration, without carrying over remaining time; it requests one haptic. A skipped phase is not treated as naturally completed for the daily tally.

FR7: The user can extend the active phase by +5 or +10 minutes. Each extension adds the selected duration to the current deadline, repeated extensions stack, and defaults remain unchanged. Extend is silent; the phase does not advance until the extended deadline unless skipped or stopped.

FR8: Before release, the project owner verifies the finished app on Pebble Time 2, including a full 45/10/5 cycle, background reminder behavior, haptics, app foregrounding, timing, recovery, device limits, reliability, and UI/control usability.

**Epic 3 additions (2026-11-03, owner-requested settings menu):**

FR9: The app provides a Settings entry in the native watch menu, visible in every lifecycle state (inactive, running, paused, timed-paused). Settings lets the owner choose the haptic notification pattern: **Short**, **Long**, **Double**, or **Until Select**. The choice is persisted across app restarts and independently of the session record, so Stop, Start, and session-record recovery never reset it. The default is **Short**, which preserves the current single short pulse.

FR10: The chosen pattern applies to every haptic request: automatic phase transition, Skip, and timed-pause end. Events that do not request a haptic (Start, manual Pause, Resume, Stop, Extend) remain silent regardless of the setting.

FR11: With **Until Select**, the haptic repeats until the owner presses Select, then stops. The Select press that dismisses the alert is consumed and does not trigger the middle-control action (it must not start a 30-minute timed pause or resume). The repeat also stops on a safety cap (2 minutes; owner decision 2026-11-03), on Stop, when a newer haptic-requesting transition supersedes it, and when the app exits. Stopping is always silent.

**Epic 4 additions (2026-11-03, owner-requested configurable phase lengths):**

FR12: Settings lets the owner set the length of each phase: **Sit**, **Stand**, and **Move**. Each length is chosen from 5 to 60 minutes in steps of 5 minutes. The defaults stay 45 (Sit), 10 (Stand), and 5 (Move), so an owner who never opens the setting sees today's behavior. The phase keeps the name `Move` (owner decision: not renamed to `Walk`). Where FR1, FR2, FR6, and FR8 state 45/10/5 minutes, those are the default lengths; with changed lengths the same rules apply to the configured values.

FR13: The three lengths persist across app restarts and independently of the session record, so Stop, Start, and session-record recovery never reset them. A missing, corrupt, or out-of-range stored value falls back to that phase's default without a visible error.

FR14: A changed length applies only to phases that start after the change: an automatic phase transition, Skip, a new Start, and the full-length rebase on overdue recovery. The running phase keeps its deadline; a paused or timed-paused phase keeps its saved remaining time; no Wakeup is rescheduled because of a settings change. Timed-pause choices (5, 10, 30 minutes) and extension amounts are unchanged. The daily tally keeps counting naturally completed phases regardless of their configured length (owner decision); skipped and in-progress phases remain excluded.

FR15: Settings offers a **Reset** entry that restores the three lengths to 45/10/5 in one action. It does not change the haptic pattern.

### NonFunctional Requirements

NFR1: The application targets Pebble Time 2 and must be operable from the watch without phone interaction during a session. Timing must continue after the app leaves the foreground; operation after explicit app termination is not required.

NFR2: Phase and timed-pause deadlines use UTC epoch wall time. Forward clock corrections can make a deadline due; backward corrections defer it. When UTC is invalid, a new session remains inactive with a visible error; an active session preserves saved state and suppresses scheduling/haptics until valid time returns, then reconciles.

NFR3: Before implementation, establish pinned toolchain and reproducible build/install readiness with available workflow and emulator evidence. Before release, verify Pebble Time 2 firmware/OS, Wakeup behavior after leaving the finished app, haptic delivery, app foregrounding, measurable timing tolerance, operational limits, reliability, and finished UI on physical hardware in Story 1.8. Firmware-specific delivery is not guaranteed until those acceptance checks pass.

NFR4: Serialize user commands, Wakeup callbacks, launches, and recovery through one FIFO session coordinator. Reconcile a due deadline before applying a competing command using `now >= deadline`; UI must not independently resolve session timing or races.

NFR5: Persist session state and Wakeup identity/generation so stale callbacks are rejected. Haptic attempts are at-most-once for a committed transition; do not replay stale haptics after recovery. On persistence or scheduling failure, retain the last committed state as specified, surface a visible error, and do not claim an uncommitted session mutation is active or reliably scheduled.

NFR6: Haptic is the only v1 notification mode. Skip, automatic phase deadlines, and timed-pause completion request a haptic; Start, manual Pause, Resume, Stop, and Extend do not. Physical haptic delivery is best-effort and must be verified on the target device.

NFR7: Preserve the reminder-only MVP scope: no posture/step/movement sensing, health claims, history/adherence analytics or telemetry, automatic schedules, cloud/social services, required phone companion, or configurable default phase lengths.

NFR8: The watch interface must remain legible and textually identify phase, lifecycle state, remaining time, daily totals, and errors using native Pebble presentation. Do not rely on color alone; verify legibility and layout on Pebble Time 2 hardware.

NFR9: Before implementation, pin the exact Pebble CLI, Python, and C SDK package/version and record reproducible build/install evidence using the available workflow. Record physical-device verification results in Story 1.8 before release; emulator results do not establish physical-device behavior.

**Epic 3 amendments (2026-11-03):**

NFR6 (amended): Haptic is the only notification mode, but its pattern is owner-selectable (FR9). The set of events that request a haptic is unchanged. Physical delivery of each pattern remains best-effort and must be verified on the device.

NFR10: A haptic request is still a single committed attempt per transition. The persisted haptic-attempt marker is written before the first vibration and is never replayed after recovery. Repeating (Until Select) is a presentation of that one attempt, not a retry. A relaunch after the app exits does not resume repeating.

NFR11: The haptic setting lives in its own small persistent-storage key. It must not change the versioned session record schema, and a missing, corrupt, or unsupported value falls back to **Short** without surfacing an error.

**Epic 4 amendments (2026-11-03):**

NFR7 (amended): The exclusion of "configurable default phase lengths" no longer applies; the three phase lengths are owner-configurable (FR12). Every other NFR7 exclusion stands: no sensing, analytics, schedules, cloud, or required phone companion.

NFR12: The phase lengths live in their own small persistent-storage key, separate from the haptic setting key and from `SESSION_STORE_KEY`. They must not change the versioned session record schema. A missing, wrong-size, or out-of-range value falls back to the default for that phase without surfacing an error.

NFR13: Phase timing stays owned by the coordinator. The coordinator reads the configured length once when a phase starts, through its single duration lookup; the UI only writes the setting and never computes deadlines or touches a running phase. Deadline, Wakeup, generation, haptic-attempt, and recovery rules (NFR2, NFR4, NFR5, NFR10) are unchanged, and the existing 45/10/5 behavior is preserved when no setting is saved.

### Additional Requirements

- The architecture specifies one native Pebble C watch app using an event-driven state machine. The Session Coordinator is the sole owner and mutation boundary for lifecycle state, active phase, deadlines, persistence, and Wakeup scheduling; the UI renders coordinator state and dispatches commands.
- Use a canonical, versioned private session record with lifecycle (`inactive`, `running`, `paused`, `timed_paused`), generation, active phase, haptic-attempt marker, and state-specific timing. Invalid or unsupported stored schema is treated as inactive with a recoverable error; never infer active state from a partial record.
- Use Pebble Wakeup API for the next phase or pause deadline, with one persisted WakeupId and generation cookie. Validate lifecycle, cookie/generation, WakeupId, and deadline on callbacks; cancel stale/orphaned wakeups and repair scheduling on app launch according to the architecture's commit-ordering rules.
- Schedule/commit ordering and failures must follow AD-11 and AD-13: do not report a proposed mutation as committed if persistence or Wakeup scheduling fails; retain last committed state and only retry failed proposals on the specified explicit user commands.
- Haptic-attempt state must be persisted before calling the haptic API. Retry only committed transitions without an attempt marker; never replay a marked attempt.
- Process due phase and timed-pause events through the coordinator before commands; generation increments on lifecycle/deadline mutations. Ignore callbacks made stale by pause, resume, stop, skip, extension, or newer scheduling.
- In pre-release Story 1.8, record exact target firmware/OS and toolchain identifiers; test normal and overdue deadlines, timed pause choices and early resume, stale callbacks, skip/extension scheduling, command/deadline races, clock validity and corrections, persistence/schedule/cancel failures, launch repair, and crash boundaries on target hardware.
- Story 1.1 resolves and pins the exact Pebble C SDK, CLI, and Python 3.13 environment and records reproducible build/install readiness. Story 1.8 verifies Wakeup constraints and battery/reliability on physical hardware before release. Keep the compact session record within Pebble persistent-storage limits.
- No starter template is specified in the Architecture. The suggested `src/` files are a structural seed, not a required layout. No separate migration, cloud integration, or deployment service is specified.
- The product brief confirms watch-first use, the 45/10/5 schedule, manual controls, timed-pause behavior, deterministic recovery, UTC authority, and the out-of-scope items reflected above. Where brief wording differs in detail, the final PRD, Architecture, and UX contract provide the implementation-level requirements.
- Product/UX decisions confirmed: native menu order is Start when inactive and Pause/Resume, Skip, Stop while active; phase icons are monochrome chair/standing-person/moving-person paired with text; timed-pause alternatives use shorter-on-top and longer-on-bottom (30-minute entry shows 5/10); daily tally uses local calendar day, persists across app restarts, aggregates across sessions, and resets at local midnight.
- Epic 3 constraints from the code: `prv_request_haptic()` in `src/c/session_coordinator.c` is the single haptic call site (currently `vibes_short_pulse()`), and all haptic requests flow through it. Select is also the middle control (`session_coordinator_middle_control()`), so the dismiss press must be intercepted before it. The setting needs its own persist key beside `SESSION_STORE_KEY`.
- Epic 4 constraints from the code: `prv_duration()` in `src/c/session_coordinator.c` hardcodes 45/10/5 minutes and is the single lookup used by the cycle advance, Skip, Start, and overdue recovery. The lengths need their own persist key beside `HAPTIC_SETTINGS_KEY` in a settings module like `haptic_settings.c`. The host test build (`scripts/test.sh`) compiles the coordinator against Pebble API stubs, so the new setting must be readable through the stubs. The existing Settings window (`prv_settings_*` in `src/c/sit_stand_move.c`) is a single MenuLayer of four haptic rows and must be extended without breaking Story 3.1/3.2 behavior.
- Physical-device decisions to verify with the finished app before release in Story 1.8: Pebble Wakeup delivery after leaving the app, haptic delivery, app foregrounding, and measurable phase-transition tolerance on the target firmware.

### UX Design Requirements

UX-DR1: Implement a single foreground session view and the native Pebble watch menu only. Do not introduce phone, settings, history, analytics, or additional product surfaces.

UX-DR2: Use native Pebble visual styles, system palette/contrast conventions, geometry, spacing, and display-safe area. Do not invent palette values, fonts, decorative layers, shadows, or custom shapes; verify platform-defined dimensions and metrics on Pebble Time 2.

UX-DR3: Arrange the session view in this order: small current wall-clock time at the top; primary large remaining phase time; phase icon directly below the countdown; readable Sit/Stand/Move label paired with its icon; small bottom-line daily Sit/Stand/Move tally.

UX-DR4: Give the active phase countdown the strongest visual emphasis. Keep current time and daily tally secondary, and use `Move` consistently (never `Walk`) as the phase name.

UX-DR5: Pair a monochrome chair icon with readable Sit text, a standing-person icon with readable Stand text, and a moving-person icon with readable Move text. Text is authoritative; icons are supplementary and color must not be the sole phase or state identifier. Verify suitable native assets and legibility on target hardware.

UX-DR6: Map the right-side top, middle, and bottom hardware buttons to the specified running-session controls: top `+5`, middle pause/timed-break action, bottom `+10`. While timed-paused, top selects the shorter alternative and bottom the longer alternative, with the available duration printed beside each control. When 30 minutes is active, show top 5 and bottom 10.

UX-DR7: Distinguish timed break and indefinite manual pause with readable `BREAK` and `PAUSED` labels. Manual pause shows the frozen phase and remaining phase time; timed pause shows remaining break time. Both remain distinguishable without color.

UX-DR8: Present Start as the inactive-session native menu command. While active, present Pause/Resume, Skip, then Stop in that order, using native Pebble menu rows and the lifecycle-appropriate Pause or Resume label.

UX-DR9: Render coordinator state only; the view must not own or advance timers. Show current watch time, active phase, derived remaining time, and today's naturally completed Sit/Stand/Move counts. Skipped and in-progress phases are excluded from counts; tally aggregates completed phases across sessions today.

UX-DR10: Show visible, concise native-style status text for invalid clock, persistence, and Wakeup/scheduling failures. Never imply that a failed mutation is active or reliably scheduled; errors must not depend on color.

UX-DR11: Use native Pebble text sizing and high-contrast presentation. Verify that countdown, phase identity, PAUSED/BREAK states, button labels, daily totals, and error text fit and remain legible on Pebble Time 2; exact type metrics and accessibility scaling are platform-defined.

UX-DR12: Use concise, neutral, functional labels and avoid promotional or motivational copy. Retain the literal `Sit`, `Stand`, `Move`, `+5`, `Pause`, `Resume`, and `+10` labels where applicable.

UX-DR13: Verify the app's display-safe layout and native control behavior on the target Pebble Time 2. Do not represent firmware-dependent Wakeup, haptic delivery, or app foregrounding as guaranteed before physical-device verification.

**Epic 2 additions (2026-11-03, owner-requested UX change; supersede the conflicting wording in UX-DR3, UX-DR4, and UX-DR6 for the session view):**

UX-DR14: Render the current wall-clock time and the daily Sit/Stand/Move tally in the native Gothic 24 text style (about 1.7x the previous Gothic 14; the nearest native size to the requested 50% increase), replacing the "small" treatment in UX-DR3 and UX-DR4. No custom or scaled font rendering.

UX-DR15: The enlarged daily tally must fit the Pebble Time 2 display-safe area without clipping or truncation in every lifecycle state (inactive, running, `PAUSED`, `BREAK`, and error shown). The owner chose one compact row, `S3 T2 M1` (S = Sit, T = Stand, M = Move; 2026-11-03), which supersedes the earlier requirement that the tally labels stay literal; counts stay textually identifiable and the phase name stays literal.

UX-DR16: Show each hardware-button label at the right screen edge, vertically level with its physical button, replacing the single centered `TOP … BOTTOM …` line. Top: `+5` while running, or the shorter alternative duration while timed-paused. Middle: `II` while running, with `BREAK` while timed-paused and `PAUSED` while manually paused. Bottom: `+10` while running, or the longer alternative duration while timed-paused. This refines UX-DR6 and does not change button functions.

UX-DR17: The active phase countdown remains the strongest visual element (UX-DR4) and the phase icon and name stay directly below it (UX-DR3). The countdown, icon, and phase name form one block centered on the screen in both directions, the clock is centered on the screen width, and the labels sit 5 px in from the right edge (2026-11-03 owner changes, superseding the narrowed content column). Countdown, icon, phase name, tally, button labels, and error text do not overlap or clip. Verify on Pebble Time 2 hardware, with emulator evidence recorded separately.

**Epic 3 additions (2026-11-03, owner-requested settings menu; supersede UX-DR1 and the Story 1.2 "no settings" criterion for the Settings entry only):**

UX-DR18: Add a native menu row **Settings** as the last row in every lifecycle state: Start/Settings when inactive; Pause or Resume, Skip, Stop, Settings when active. The existing row order and labels are otherwise unchanged. Settings opens one native menu window; no other new surface is introduced, and UX-DR1's exclusion of phone, history, and analytics surfaces still holds.

UX-DR19: The Settings window lists four rows in this order: `Short`, `Long`, `Double`, `Until Select`. The current choice is marked with readable text (for example a leading `*` or a `Selected` subtitle), not color alone. Selecting a row saves it, gives one preview of that pattern (for Until Select, a single short preview rather than the repeating alert), and returns to the previous screen. Back leaves the setting unchanged.

UX-DR20: While an Until Select alert is repeating, the session view shows a concise text cue (for example `Select to stop`) that does not overlap or clip the countdown, icon, phase name, tally, or button labels. Pressing Select stops the alert and removes the cue. Other buttons do not dismiss it and keep their normal functions.

**Epic 4 additions (2026-11-03, owner-requested configurable phase lengths; extend UX-DR18/UX-DR19, no new surface beyond the Settings window):**

UX-DR21: The Settings window shows the three phase lengths and the Reset entry in addition to the four haptic rows, in the same native menu window and without a new surface. Order: the four haptic rows as today, then `Sit`, `Stand`, `Move`, `Reset`. Each length row shows its current value as readable text (for example `Sit 45 min`), never color alone. Selecting a haptic row behaves as in UX-DR19.

UX-DR22: Selecting a length row steps its value up by 5 minutes and wraps from 60 back to 5 (owner-confirmed edit gesture, 2026-11-03). The new value is saved and the row text updates in place; the window stays open so several lengths can be set in one visit. Back leaves the window with the saved values. Selecting `Reset` restores 45/10/5 and updates the rows. A failed save leaves the previous value shown and stored.

UX-DR23: The longest Settings rows (for example `Stand 60 min`) fit the Pebble Time 2 display-safe area without clipping, and the window scrolls natively. Verify on the emulator and, if available, Pebble Time 2, and record emulator evidence separately.

### FR Coverage Map

FR1: Epic 1 - Start and stop a session from the watch.
FR2: Epic 1 - Run the repeating Sit/Stand/Move cycle with deadline-based recovery.
FR3: Epic 1 - Receive phase reminders and see the current phase in the app.
FR4: Epic 1 - Manually pause and resume a session.
FR5: Epic 1 - Take and end a timed pause, including early resume handling.
FR6: Epic 1 - Skip the active phase.
FR7: Epic 1 - Extend the active phase by +5 or +10 minutes.
FR8: Epic 1 - Verify the finished app on Pebble Time 2 before release.

FR9: Epic 3 - Settings menu with a persisted haptic pattern choice.
FR10: Epic 3 - Chosen pattern applies to phase transition, Skip, and timed-pause end.
FR11: Epic 3 - Until Select repeats until Select (capped); the dismiss press is consumed.

Epic 2 adds no FRs. It refines presentation requirements and covers the new UX design requirements:

UX-DR14: Epic 2 - Story 2.1 - Time and tally at Gothic 24.
UX-DR15: Epic 2 - Story 2.1 - Enlarged tally fits without clipping.
UX-DR16: Epic 2 - Story 2.1 - Button labels at the right edge, level with the buttons.
UX-DR17: Epic 2 - Story 2.1 - Countdown stays dominant; no overlap.

Epic 3 covers the amended and new requirements:

NFR6 (amended), NFR10, NFR11: Epic 3 - Selectable pattern, one haptic attempt per transition, separate setting storage.
UX-DR18: Epic 3 - Settings row in the native menu, all states.
UX-DR19: Epic 3 - Settings window with a text-marked current choice.
UX-DR20: Epic 3 - Select-to-stop cue while an Until Select alert repeats.

Epic 4 covers the new and amended requirements:

FR12: Epic 4 - Story 4.1 - Sit, Stand, and Move lengths (5 to 60 min, step 5) in Settings.
FR13: Epic 4 - Story 4.1 - Lengths persist separately from the session record; invalid values fall back to the defaults.
FR14: Epic 4 - Story 4.1 - Changed lengths apply only to phases that start later; running and paused phases, Wakeups, timed-pause choices, and Extend are unchanged; tally unchanged.
FR15: Epic 4 - Story 4.2 - Reset restores 45/10/5 and leaves the haptic pattern alone.
NFR7 (amended), NFR12, NFR13: Epic 4 - Story 4.1 - Lengths are configurable; own persist key; the coordinator reads the length when a phase starts.
UX-DR21: Epic 4 - Stories 4.1 and 4.2 - Length rows and the Reset row in the Settings window.
UX-DR22: Epic 4 - Stories 4.1 and 4.2 - Select steps a length up by 5 with wrap; Reset restores the defaults.
UX-DR23: Epic 4 - Story 4.1 - Longest rows fit without clipping.

## Epic List

### Epic 1: Run and control a sit–stand–move session from the watch

The owner can start and stop a session on Pebble Time 2, follow the repeating Sit/Stand/Move cycle, and manage it with pause, timed pause, skip, and extension controls. The watch shows the current phase and remaining time, and handles reminders and recovery according to the agreed timing rules. Pinned reproducible toolchain/build readiness is a pre-implementation gate; finished-app physical-device verification of Wakeup, haptic, app-foreground behavior, and UI is a pre-release gate in Story 1.8.

**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6, FR7, FR8.

### Epic 2: Make the session view easier to read at a glance

The owner can read the current time and the daily tally more easily, and can see what each right-side hardware button does because its label sits next to that button. This epic changes layout and presentation only; session behavior, timing, and the coordinator are unchanged. It builds on Epic 1's session view and does not reopen Story 1.8.

**FRs covered:** none new. Refines NFR8 and UX-DR3, UX-DR4, and UX-DR6; adds UX-DR14–UX-DR17.

### Epic 3: Choose how the watch notifies me

The owner can open Settings from the native watch menu and choose the haptic pattern: Short, Long, Double, or Until Select. Phase transitions, Skip, and timed-pause end use the chosen pattern; with Until Select the alert repeats until the owner presses Select, with a safety cap. The choice persists across restarts and Stop. Session timing, scheduling, recovery, and the Epic 1 and 2 behavior are unchanged apart from the haptic call using the chosen pattern. It builds on Epics 1 and 2 and does not reopen Story 1.8.

**FRs covered:** FR9, FR10, FR11. Also covers NFR6 (amended), NFR10, NFR11, UX-DR18–UX-DR20.

### Epic 4: Choose how long each phase lasts

The owner can open Settings and set the length of Sit, Stand, and Move (5 to 60 minutes in steps of 5), or reset them to 45/10/5. A new length applies to phases that start afterwards; the running phase, pauses, Wakeups, timed-pause choices, and Extend are unchanged, and the daily tally keeps counting naturally completed phases. The lengths persist across restarts and Stop. It builds on Epics 1 to 3 and does not reopen Story 1.8.

**FRs covered:** FR12, FR13, FR14, FR15. Also covers NFR7 (amended), NFR12, NFR13, UX-DR21–UX-DR23.

## Epic 1: Run and control a sit–stand–move session from the watch

The owner can start and stop a session on Pebble Time 2, follow the repeating Sit/Stand/Move cycle, and manage it with pause, timed pause, skip, and extension controls. The watch shows the current phase and remaining time, and handles reminders and recovery according to the agreed timing rules. Pinned reproducible toolchain/build readiness is a pre-implementation gate; finished-app physical-device verification of Wakeup, haptic, app-foreground behavior, and UI is a pre-release gate in Story 1.8.

### Story 1.1: Establish Pebble toolchain and emulator readiness

As the project owner,
I want to establish a pinned, reproducible Pebble build/install workflow and record available emulator evidence,
So that implementation can begin on a known toolchain without treating emulator results as physical-device acceptance.

**Requirements:** NFR3, NFR9.

**Acceptance Criteria:**

**Given** the selected project toolchain
**When** readiness is established
**Then** the exact CLI, Python, and C SDK package/version are pinned and recorded, distinguishing manifest SDK API level from package version.

**Given** the pinned toolchain and the app at a recorded commit
**When** documented clean/repeated build and available emulator install procedures are run
**Then** commands, outcomes, and artifact identity are recorded as reproducible; any owner-reported results are identified as such and are not presented as locally verified.

**Given** the app build under test is available in the emulator
**When** phase-deadline and timed-pause Wakeups are observed separately
**Then** available callback, haptic-request, app-open/foreground, and timing evidence is recorded with its source; emulator observations are not physical-device results.

**Given** physical acceptance requires the finished app
**When** Story 1.1 is handed off
**Then** the complete Pebble Time 2 device acceptance scope is assigned to Story 1.8 after Stories 1.2–1.7 and before release, with no physical behavior claimed as verified here.

**Given** any toolchain or emulator readiness evidence remains incomplete
**When** Story 1.1 status is updated
**Then** the missing evidence is stated and the story remains open; it is not marked done.

### Story 1.2: Start and stop a session from the watch

As the watch owner,
I want to start and stop a Sit/Stand/Move session and see its current status,
So that I can control reminders directly from Pebble Time 2.

**Requirements:** FR1, NFR1, NFR4–NFR5, NFR7–NFR8, UX-DR1–UX-DR5, UX-DR8–UX-DR13.

**Dependency:** Story 1.1's pinned toolchain and reproducible build/install readiness must be complete before implementation stories begin. On 2026-10-31, the owner authorized Stories 1.2 and 1.3 to proceed while Story 1.1 remains open; this waiver applies only to those stories, does not satisfy or close Story 1.1, and does not waive readiness for later implementation stories. On 2026-11-03, the owner extended the waiver to Story 1.6 while Story 1.1 remains open; Stories 1.4 and 1.5 were implemented while Story 1.1 was open. On 2026-11-03, the owner extended the waiver to Story 1.7 while Story 1.1 remains open. These extensions do not satisfy or close Story 1.1 and do not cover Story 1.8. Physical-device acceptance is deferred to Story 1.8 and does not block this story.

**Acceptance Criteria:**

**Given** no session is active and UTC time is valid
**When** I select **Start** from the native watch menu
**Then** the session enters **running** with **Sit** active, a 45-minute deadline based on the current UTC time, and a Wakeup scheduled for that deadline, without a haptic.

**Given** UTC time is invalid or unavailable
**When** I select **Start**
**Then** the session remains inactive and the watch displays a visible clock-time error.

**Given** a session is active
**When** I select **Stop** from the native watch menu
**Then** the session becomes inactive, its Wakeup is cancelled, and any stale callback from that session is ignored without a reminder or haptic.

**Given** Start, Stop, a Wakeup callback, or an app-recovery event enters the session coordinator
**When** the coordinator processes these events
**Then** it serializes them through one FIFO queue and reconciles any due deadline before applying a user command.

**Given** the coordinator persists session state
**When** a session lifecycle mutation is committed
**Then** the sole coordinator mutation boundary writes one compact, versioned canonical record containing lifecycle, generation, active phase, haptic-attempt generation, and the timing/Wakeup fields valid for that lifecycle; the record remains within Pebble storage limits.

**Given** the session is inactive or running
**When** I view the app
**Then** the native session view shows the current watch time and, when running, the remaining time, phase icon with readable **Sit**, **Stand**, or **Move** label, and today's completed phase tally in the specified visual order.

**Given** the session view is shown on Pebble Time 2
**When** it renders session information
**Then** it uses native Pebble text styles, palette, geometry, spacing, and display-safe area; the countdown is primary, current time and tally are secondary, and the content follows the order current time, remaining phase time, phase icon, phase name, daily tally.

**Given** the app is used for its v1 purpose
**When** the available product surfaces are reviewed
**Then** the app provides only the session view and native watch menu, with no posture/movement sensing, health claims, history/adherence analytics or telemetry, automatic schedules, cloud/social services, required phone companion, configurable defaults, settings, or other additional surface.

**Given** the session view includes a phase icon or status
**When** phase or lifecycle state is communicated
**Then** readable text identifies Sit, Stand, or Move and any PAUSED/BREAK status; color and icons are not the sole means of identifying state.

**Given** the layout is built for the target Pebble Time 2
**When** the session view is checked on target hardware
**Then** countdown, phase identity, lifecycle state, tally, controls, and visible errors fit and remain legible using native high-contrast presentation.

**Given** the app is inactive or a session is active
**When** I open the native watch menu
**Then** it shows Start when inactive, and Pause/Resume, Skip, then Stop while active, with Pause or Resume appropriate to the current lifecycle state.

**Given** the session view shows a phase
**When** it renders the phase identity
**Then** it pairs a monochrome chair icon with Sit, a standing-person icon with Stand, or a moving-person icon with Move, and readable text remains authoritative.

**Given** the app is opened or relaunched with a committed session record
**When** it restores the session
**Then** the coordinator clears orphaned Wakeups, reconciles the committed record, schedules exactly the intended Wakeup for a running or timed-paused session, and renders the committed state; partial or unsupported stored data is treated as inactive with a recoverable visible error.

**Given** a session mutation requires a new Wakeup
**When** the coordinator commits the proposed mutation
**Then** it schedules a candidate Wakeup, persists the record and candidate WakeupId before reporting success, and cancels a prior Wakeup only after the new record is committed; if persistence fails, it cancels the candidate and retains the last committed record.

**Given** saving session state or scheduling its Wakeup fails
**When** Start or Stop is processed
**Then** the app does not present an uncommitted change as active or reliably scheduled, and displays a visible failure message.

The daily tally counts naturally completed phases across sessions, uses the watch's local calendar day, persists across app restarts, and resets at local midnight. Skipped and in-progress phases do not count.

### Story 1.3: Follow the cycle and recover from missed deadlines

As the watch owner,
I want the session to advance through Sit, Stand, and Move and recover predictably if a deadline is missed,
So that reminders remain useful even when the app is not in the foreground.

**Requirements:** FR2, FR3, NFR1–NFR6, UX-DR3–UX-DR5, UX-DR9–UX-DR11, UX-DR13.

**Dependencies:** Story 1.2 establishes the session coordinator, canonical persisted record, and launch-recovery foundation used by this story.

**Acceptance Criteria:**

**Given** a running phase reaches its deadline while the app is in the background
**When** its scheduled Wakeup is processed on time
**Then** the coordinator advances to the next phase in Sit → Stand → Move → Sit order, assigns that phase its default duration (45/10/5 minutes), sets its deadline from the expired deadline according to on-time cycle semantics, schedules the next deadline, requests one haptic, and requests app open/foreground; the opened session view shows the new phase and its remaining time.

**Given** a phase deadline is overdue when the app recovers
**When** the coordinator reconciles the session using `now >= deadline`
**Then** it processes elapsed phases in order, starts the resulting phase at its full default duration from recovery time, and requests at most one haptic for the reconciled state.

**Given** the device clock is adjusted while a session is running
**When** the coordinator evaluates the UTC deadline
**Then** a forward correction that makes the deadline due triggers reconciliation, while a backward correction defers the transition until the deadline is due.

**Given** a Wakeup callback does not match the committed lifecycle, generation, Wakeup ID, or deadline
**When** the callback is received
**Then** it is ignored without advancing the phase, requesting a haptic, or reopening the app.

**Given** a committed phase transition is due to request a haptic
**When** the coordinator handles the transition, including after app recovery
**Then** it persists the haptic-attempt marker before calling the haptic API and never retries an already-marked attempt.

**Given** persistence or Wakeup scheduling fails while processing a phase transition
**When** the transition is attempted
**Then** the last committed state is retained, the UI displays a visible failure, and the proposed transition is not presented as reliably active or scheduled.

**Given** a valid session deadline is scheduled
**When** the app is not foregrounded
**Then** phase timing continues without phone interaction, subject to pre-release physical-device acceptance in Story 1.8.

**Given** the session view is open during a running phase
**When** time passes without a lifecycle transition
**Then** the displayed remaining time is derived from the coordinator's UTC deadline and refreshes without the UI independently advancing the phase or owning a timer.

**Given** UTC becomes invalid during a running session
**When** the coordinator detects that UTC is unavailable or invalid
**Then** it preserves the last committed session state, suppresses scheduling and haptic requests, and displays a visible clock error.

**Given** UTC becomes valid again after being invalid during a running session
**When** the coordinator resumes time-based processing
**Then** it reconciles the saved deadline before scheduling the next Wakeup, applies the overdue-recovery rules if due, and only then resumes scheduling and haptic behavior.

**Given** a phase completes naturally at its deadline
**When** the completed-phase tally is updated
**Then** that phase's count increases exactly once for the watch's local calendar day, with counts aggregated across sessions.

**Given** a daily tally has been recorded
**When** the app restarts during the same local calendar day
**Then** the tally is restored from local persistence.

**Given** the watch crosses local midnight
**When** the session view displays today's tally
**Then** the new day's Sit, Stand, and Move totals start at zero.

### Story 1.4: Manually pause and resume a session

As the watch owner,
I want to pause a session indefinitely and resume it when I'm ready,
So that a phase does not advance while I'm unavailable.

**Requirements:** FR4, NFR2, NFR4–NFR5, NFR8, UX-DR7–UX-DR10.

**Acceptance Criteria:**

**Given** a session is running with a valid phase deadline
**When** I select **Pause** from the native watch menu
**Then** the session enters manual `paused` state, preserves the active phase and positive remaining duration, has no phase deadline or Wakeup scheduled, and requests no haptic.

**Given** a session is manually paused
**When** the paused session view is shown
**Then** it displays `PAUSED`, the frozen phase, and its saved remaining phase time in readable text, without relying on color.

**Given** a session is manually paused
**When** I select **Resume** from the menu
**Then** the same phase resumes with a new UTC deadline of current time plus the saved remaining duration, and no haptic is requested.

**Given** a session is manually paused and the session view is displayed
**When** I press the middle hardware control
**Then** the same phase resumes with its saved remaining duration, and no haptic is requested.

**Given** a Pause or Resume command coincides with a due deadline
**When** the coordinator processes the command
**Then** it reconciles the due deadline first using `now >= deadline`, then applies the command to the resulting state.

**Given** persistence or Wakeup cancellation/scheduling fails during Pause or Resume
**When** the command is processed
**Then** the last committed state remains authoritative, a visible failure is shown, and an uncommitted change is not presented as active or reliably scheduled.

**Given** a callback from the pre-pause or pre-resume Wakeup arrives later
**When** the coordinator validates its lifecycle, generation, and Wakeup ID
**Then** the stale callback is ignored without advancing a phase, requesting a haptic, or reopening the app.

### Story 1.5: Take and end a timed pause

As the watch owner,
I want to pause a phase for 5, 10, or 30 minutes and resume it early or at the selected time,
So that I can take a planned break without losing my place in the cycle.

**Requirements:** FR5, NFR2–NFR6, NFR8, UX-DR6–UX-DR7, UX-DR10–UX-DR13.

**Acceptance Criteria:**

**Given** a session is running
**When** I start a timed pause for 5, 10, or 30 minutes
**Then** the session enters `timed_paused`, saves the active phase's remaining duration, schedules the selected pause-end deadline, does not let phase time elapse during the break, and requests no haptic.

**Given** a session is running
**When** I press the middle hardware control
**Then** a 30-minute timed pause starts, the phase's remaining duration is saved, and the UI displays `BREAK` with the remaining break time.

**Given** a session is timed-paused
**When** the pause view is shown
**Then** it displays `BREAK` and remaining break time; the top control shows the shorter alternative and the bottom control the longer alternative (for 5 active, top 10/bottom 30; for 10 active, top 5/bottom 30; for 30 active, top 5/bottom 10), without changing the saved phase duration.

**Given** the timed-pause duration is changed using a hardware control
**When** I select an alternative duration
**Then** the pause-end deadline is updated and rescheduled for that duration, and the updated duration is reflected by the control labels.

**Given** a timed pause reaches its deadline, including recovery when overdue
**When** the coordinator processes PauseEnd
**Then** it resumes the same phase with a new deadline of current UTC plus the saved remaining phase duration, does not advance the cycle, requests at most one haptic, and requests app open/foreground.

**Given** a session is timed-paused before its pause-end deadline
**When** I select Resume using the middle hardware control
**Then** the same phase resumes with its saved remaining duration, the pause Wakeup is cancelled, and no haptic is requested.

**Given** early Resume cancels a pause Wakeup and a stale callback later arrives
**When** the coordinator validates the callback's lifecycle, generation, and Wakeup ID
**Then** the callback is ignored without requesting a haptic or reopening the app.

**Given** the pause-end deadline is already due when Resume is processed
**When** the coordinator handles the command
**Then** it reconciles PauseEnd first, and the Resume command does not cause a second transition or haptic.

**Given** persistence or Wakeup scheduling/cancellation fails during timed-pause entry, duration change, or resume
**When** the mutation is attempted
**Then** the last committed state remains authoritative, a visible failure is shown, and the app does not imply that an uncommitted break or resume is active or reliably scheduled.

The top/bottom duration mapping is fixed: shorter alternative on top and longer alternative on bottom. A newly started 30-minute break shows top 5 and bottom 10.

### Story 1.6: Skip the active phase

As the watch owner,
I want to skip the current phase,
So that the session can move on when a phase does not fit the moment.

**Requirements:** FR6, NFR4–NFR6, NFR8, UX-DR8–UX-DR10, UX-DR12.

**Acceptance Criteria:**

**Given** a session is running in Sit, Stand, or Move
**When** I select **Skip** from the native watch menu
**Then** the coordinator advances immediately to the next phase in Sit → Stand → Move → Sit order, assigns that phase its default duration, schedules its deadline, updates the session view, and requests one haptic.

**Given** a phase is skipped
**When** today's tally is displayed
**Then** the skipped phase is not counted as naturally completed.

**Given** Skip is processed when the current phase deadline is due
**When** the coordinator handles the command
**Then** it reconciles the due deadline first using `now >= deadline`, and then applies Skip to the resulting state.

**Given** persistence or Wakeup scheduling fails during Skip
**When** the mutation is attempted
**Then** the last committed state remains authoritative, the app displays a visible failure, and the proposed phase change is not presented as active or reliably scheduled.

**Given** a callback for the phase that was skipped arrives later
**When** the coordinator validates its lifecycle, generation, and Wakeup ID
**Then** it ignores the stale callback without a duplicate phase transition or haptic.

### Story 1.7: Extend the active phase

As the watch owner,
I want to extend the active phase by 5 or 10 minutes,
So that I can stay in the current phase when I need more time.

**Requirements:** FR7, NFR4–NFR5, NFR8, UX-DR6, UX-DR10, UX-DR12.

**Acceptance Criteria:**

**Given** a session is running
**When** I select the top `+5` or bottom `+10` hardware control
**Then** the selected duration is added once to the active phase deadline, the updated remaining time is shown, and the next Wakeup is scheduled for the new deadline without a haptic.

**Given** I apply multiple extensions to a running phase
**When** each extension is committed
**Then** the extensions add cumulatively to the active deadline and do not alter default phase durations.

**Given** an Extend command is processed when the current phase deadline is due
**When** the coordinator handles the command
**Then** it reconciles the due deadline first using `now >= deadline`, then applies Extend to the resulting state if the command is valid.

**Given** persistence or Wakeup scheduling fails during Extend
**When** the mutation is attempted
**Then** the last committed state remains authoritative, a visible failure is displayed, and the app does not imply the extended deadline is active or reliably scheduled.

**Given** a callback for the superseded phase deadline arrives after an extension
**When** the coordinator validates its lifecycle, generation, and Wakeup ID
**Then** it ignores the stale callback without advancing the phase or requesting a haptic.

### Story 1.8: Verify the finished app on Pebble Time 2 before release

As the watch owner,
I want to verify the completed app's reminder behavior and interface on my Pebble Time 2,
So that release is based on actual target-device acceptance rather than emulator or toolchain evidence alone.

**Requirements:** FR1–FR8, NFR1–NFR6, NFR8–NFR9, UX-DR1–UX-DR13.

**Dependencies:** Stories 1.2–1.7 are complete; Story 1.1's pre-implementation toolchain readiness is complete. Story 1.8 is the final Epic 1 story and a pre-release gate.

**Acceptance Criteria:**

**Given** the finished app and target Pebble Time 2
**When** physical-device acceptance begins
**Then** exact device model, firmware/OS, build artifact, and CLI/SDK toolchain identifiers are recorded; the finished app is installed on the target using the pinned workflow; and the exact physical install command and outcome are recorded.

**Given** a session is started at a valid UTC time and the watch remains in normal use through one complete cycle
**When** Sit, Stand, and Move each reach their deadline
**Then** observe the complete 45/10/5 cycle on the target and record each resulting phase, remaining time, requested/observed haptic, app open/foreground behavior, and timing.

**Given** an active session with a phase Wakeup scheduled
**When** an ordinary phase deadline occurs
**Then** leave the app normally so it is no longer foregrounded without force-terminating it, then separately record Wakeup/callback occurrence, requested and observed haptic, app open/foreground behavior, active phase and remaining time, and measured timing against a product-owner-agreed tolerance and repeat count.

**Given** a timed pause is selected for 5, 10, or 30 minutes
**When** the app is left normally so it is no longer foregrounded without force-terminating it until pause end
**Then** record that each pause-end Wakeup resumes the same phase with saved remaining duration, does not advance the cycle, requests at most one haptic, opens/foregrounds the app, and meets the agreed timing tolerance.

**Given** a user resumes before timed-pause end
**When** the pause Wakeup is cancelled and any stale callback is delivered
**Then** verify the same phase resumes with saved remaining time and the stale callback neither requests a haptic nor reopens the app; also verify overdue pause-end recovery resumes once without cycle advancement.

**Given** session controls and phase deadlines are exercised on device
**When** Skip, manual Pause/Resume, Stop, Extend, competing command/deadline, phase Pause/Stop cancellation, and extension rescheduling are tested
**Then** verify the expected phase, Wakeup, stale-callback, and haptic behavior, including silent Start/Pause/Resume/Stop/Extend and one requested Skip/automatic-transition haptic.

**Given** a phase deadline is overdue or UTC is invalid or adjusted
**When** recovery and clock cases are exercised
**Then** verify ordered overdue phase reconciliation and full-duration rebase, inactive Start with visible error for invalid UTC, preserved active state with scheduling/haptics suppressed until UTC is valid, forward corrections making reached deadlines due, and backward corrections deferring them.

**Given** Wakeups are scheduled and the app is used repeatedly
**When** operational limits and sustained use are tested
**Then** verify no more than eight scheduled Wakeups, at least one-minute spacing, `notify_if_missed = false`, and record observed reliability and battery behavior.

**Given** reliability and battery observations have been recorded
**When** the project owner reviews the results against the intended personal-use pattern
**Then** record whether the observed behavior is acceptable for release; any unacceptable or inconclusive result keeps Story 1.8 open pending further testing or a product decision.

**Given** the finished app's session view and controls are used on Pebble Time 2
**When** the display and hardware controls are checked
**Then** verify countdown, phase identity, PAUSED/BREAK state, tally, button labels/behavior, native layout, and error text fit and remain legible without relying on color.

**Given** implementation stories have tested persistence, scheduling, cancellation, and launch-recovery failures and crash boundaries
**When** those implementation-test results are reviewed as part of Story 1.8 acceptance
**Then** record the evidence that committed lifecycle/phase state, Wakeup identity/generation and callback acceptance, candidate cleanup/retry behavior, visible errors, and at-most-once haptic requests follow architecture contracts; physical acceptance itself does not require injecting internal failures into the watch.

**Given** any required physical acceptance check fails or remains unverified
**When** results are reported
**Then** record the limitation and evidence, keep Story 1.8 open, and block release until the product owner resolves the affected scope or acceptance; do not claim physical acceptance passed.

## Epic 2: Make the session view easier to read at a glance

The owner can read the current time and the daily tally more easily, and can see what each right-side hardware button does because its label sits next to that button. This epic changes layout and presentation only; session behavior, timing, and the coordinator are unchanged. It builds on Epic 1's session view and does not reopen Story 1.8.

### Story 2.1: Enlarge the time and tally, and place button labels beside the buttons

As the watch owner,
I want the time and daily tally larger and each button's label next to that button,
So that I can read the session view at a glance and know what each button does.

**Requirements:** NFR8, UX-DR3, UX-DR4, UX-DR6, UX-DR11, UX-DR14–UX-DR17.

**Dependencies:** Builds on the Epic 1 session view. Story 1.8 is closed by owner waiver and is not reopened. Physical-device legibility is an owner check and does not block this story; emulator evidence is recorded as emulator evidence.

**Acceptance Criteria:**

**Given** the session view is shown in any state
**When** it renders the current wall-clock time
**Then** the time uses the native Gothic 24 text style, not a custom or scaled font, and keeps its position at the top of the view.

**Given** the session view is shown
**When** it renders today's Sit/Stand/Move tally
**Then** the tally uses Gothic 24, stays at the bottom of the view, and reads as one compact row `S3 T2 M1` (S = Sit, T = Stand, M = Move; owner decision 2026-11-03), including two-digit counts.

**Given** the enlarged tally and the optional error line
**When** the view is shown while inactive, running, `PAUSED`, or `BREAK`, with and without an error
**Then** nothing is clipped, truncated, or overlapping, and the error text stays visible and readable.

**Given** a session is running
**When** the session view is shown
**Then** `+5` appears at the right edge level with the top button, `II` level with the middle button, and `+10` level with the bottom button, and the centered `TOP … BOTTOM …` line is no longer shown.

**Given** a session is timed-paused
**When** the session view is shown
**Then** the top label shows the shorter alternative duration, the middle label shows `BREAK`, and the bottom label shows the longer alternative, following the Story 1.5 mapping (for 30 active, top 5 and bottom 10).

**Given** a session is manually paused
**When** the session view is shown
**Then** the middle label shows `PAUSED`, the frozen phase and remaining time stay visible, and no `+5` or `+10` label is shown because those buttons do not act in that state.

**Given** no session is active
**When** the session view is shown
**Then** no button labels are shown.

**Given** the label column is at the right edge
**When** the content column is laid out
**Then** the countdown, phase icon, and phase name form one block centered on the screen in both directions, the clock is centered on the screen width, and the labels sit 5 px in from the right edge; none of them overlaps another, the countdown remains the strongest element, and the icon and name remain directly below it.

**Given** the layout changes
**When** the session is used
**Then** hardware button functions, menu behavior, coordinator logic, and persistence are unchanged, and the existing tests still pass.

**Given** the new layout is built
**When** it is checked on the emulator and, if available, on Pebble Time 2
**Then** evidence for each state is recorded with its source; emulator results are not presented as physical-device results, and any unverified state is stated.

`DESIGN.md` and `EXPERIENCE.md` are updated in the same change so the UX contract matches the new layout.

## Epic 3: Choose how the watch notifies me

The owner can open Settings from the native watch menu and choose the haptic pattern: Short, Long, Double, or Until Select. Phase transitions, Skip, and timed-pause end use the chosen pattern; with Until Select the alert repeats until the owner presses Select, with a safety cap. The choice persists across restarts and Stop. Session timing, scheduling, recovery, and the Epic 1 and 2 behavior are unchanged apart from the haptic call using the chosen pattern. It builds on Epics 1 and 2 and does not reopen Story 1.8.

### Story 3.1: Choose a haptic pattern in Settings

As the watch owner,
I want to pick Short, Long, or Double for my haptic notification in a Settings menu,
So that reminders are easy to feel and recognize on my wrist.

**Requirements:** FR9, FR10, NFR6 (amended), NFR11, UX-DR18, UX-DR19.

**Dependencies:** Builds on Epics 1 and 2. Story 1.8 is closed by owner waiver and is not reopened. Physical-device feel of each pattern is an owner check and does not block this story. Emulator evidence is recorded as emulator evidence.

**Acceptance Criteria:**

**Given** the app is in any lifecycle state (inactive, running, `PAUSED`, `BREAK`)
**When** I open the native watch menu
**Then** `Settings` is the last row, after Start when inactive, or after Pause/Resume, Skip, and Stop when active, and the existing row order and labels are unchanged.

**Given** I select `Settings`
**When** the Settings window opens
**Then** it lists `Short`, `Long`, and `Double` in that order, marks the current choice with readable text (not color alone), and introduces no other new surface.

**Given** I select a pattern row
**When** the choice is saved
**Then** the pattern plays once as a preview, the choice persists, and I return to the previous screen; Back leaves the setting unchanged.

**Given** no choice has ever been saved
**When** the app runs
**Then** the pattern is `Short`, matching the current single short pulse, and behavior is unchanged from today.

**Given** a pattern is saved
**When** I stop and restart a session, or relaunch the app
**Then** the choice is still in effect and is not reset by Stop, Start, or session-record recovery.

**Given** the saved value is missing, corrupt, or unsupported
**When** the app loads it
**Then** it falls back to `Short` without a visible error, and the session record schema is unchanged.

**Given** the setting is stored
**When** persistence is reviewed
**Then** it uses its own persist key and does not alter the versioned session record.

**Given** an automatic phase transition, Skip, or timed-pause end requests a haptic
**When** the request is made
**Then** it uses the chosen pattern through the single existing haptic call site, and still makes exactly one attempt with the haptic-attempt marker persisted first.

**Given** Start, manual Pause, Resume, Stop, or Extend is performed
**When** the command is processed
**Then** no haptic is requested, whatever pattern is set.

**Given** Settings is open while a deadline comes due
**When** the transition is processed
**Then** the coordinator handles it as before and the chosen pattern plays.

**Given** the change is built
**When** session behavior, timing, coordinator logic, and persistence of the session record are reviewed
**Then** they are unchanged, and the existing tests still pass.

**Given** the feature is checked on the emulator and, if available, on Pebble Time 2
**When** evidence is recorded
**Then** each pattern's evidence is recorded with its source; emulator results are not presented as physical-device results, and any unverified pattern is stated.

`DESIGN.md` and `EXPERIENCE.md` are updated in the same change to add the Settings row and window.

### Story 3.2: Repeat the alert until Select is pressed

As the watch owner,
I want an `Until Select` haptic option that keeps vibrating until I press Select,
So that I don't miss a phase change when the watch is not on my wrist or I am busy.

**Requirements:** FR11, NFR10, UX-DR19 (fourth row), UX-DR20.

**Dependencies:** Builds on Story 3.1's persisted setting, Settings window, and single haptic call site. Physical-device feel is an owner check and does not block this story. Emulator evidence is recorded as emulator evidence.

**Acceptance Criteria:**

**Given** the Settings window is open
**When** it renders its rows
**Then** `Until Select` appears as the fourth row after `Short`, `Long`, and `Double`, with the current choice marked in text as before.

**Given** I select `Until Select`
**When** the choice is saved
**Then** it persists like the other patterns, I get one short preview (not the repeating alert), and I return to the previous screen.

**Given** `Until Select` is chosen and an automatic phase transition, Skip, or timed-pause end requests a haptic
**When** the request is made
**Then** the haptic repeats at a fixed interval until stopped, and the haptic-attempt marker is persisted once before the first vibration.

**Given** the alert is repeating
**When** the session view is shown
**Then** a concise text cue (`Select to stop`) is visible without overlapping or clipping the countdown, icon, phase name, tally, button labels, or error text, and does not rely on color.

**Given** the alert is repeating
**When** I press Select
**Then** the vibration stops, the cue disappears, and that press is consumed: it does not start a timed pause, resume, or open the menu.

**Given** the alert is repeating
**When** the Select press is handled
**Then** the dismissal is decided before the middle-control dispatch (`session_coordinator_middle_control()`), so no middle-control session event is enqueued by that press, and a test verifies that no timed pause, resume, or other lifecycle change results from it.

**Given** no alert is repeating, including after Stop, a cap expiry, or a dismissed alert
**When** I press Select
**Then** it behaves as before: middle control while a session is running, paused, or timed-paused, and the native menu opens when inactive.

**Given** the alert is repeating
**When** I press Up, Down, or long-press Select
**Then** the alert keeps repeating and the button keeps its normal function.

**Given** the alert is repeating
**When** the safety cap is reached (2 minutes after the alert starts; owner decision 2026-11-03)
**Then** the vibration stops by itself and the cue disappears, with no further reminder.

**Given** the alert is repeating
**When** I Stop the session, or the app exits
**Then** the vibration stops and the cue disappears.

**Given** the alert is repeating
**When** a newer haptic-requesting transition occurs (for example Skip)
**Then** the older alert is superseded, only one repeating alert runs, and it follows the newer transition.

**Given** the alert is repeating
**When** I start a timed pause, pause, resume, or extend using the hardware controls or menu
**Then** those commands work as before.

**Given** the app is relaunched, or recovery finds an overdue deadline, after a previous alert was marked attempted
**When** the coordinator reconciles state
**Then** the alert does not resume repeating, and the marked attempt is not replayed (NFR10).

**Given** recovery requests a haptic for a newly reconciled transition
**When** `Until Select` is chosen
**Then** at most one repeating alert starts, subject to the same cap and Select dismissal.

**Given** `Short`, `Long`, or `Double` is chosen
**When** a haptic is requested
**Then** behavior is exactly as in Story 3.1: no repeat, no cue, and Select keeps its normal function.

**Given** the change is built
**When** timing, scheduling, coordinator logic, and the session record are reviewed
**Then** they are unchanged beyond the haptic call, and the existing tests still pass.

**Given** the feature is checked on the emulator and, if available, on Pebble Time 2
**When** evidence is recorded
**Then** each case (dismiss, cap, supersede, Stop, relaunch) is recorded with its source; emulator results are not presented as physical-device results, and any unverified case is stated.

`DESIGN.md` and `EXPERIENCE.md` are updated in the same change to describe the alert cue and Select behavior.

## Epic 4: Choose how long each phase lasts

The owner can open Settings and set the length of Sit, Stand, and Move (5 to 60 minutes in steps of 5), or reset them to 45/10/5. A new length applies to phases that start afterwards; the running phase, pauses, Wakeups, timed-pause choices, and Extend are unchanged, and the daily tally keeps counting naturally completed phases. The lengths persist across restarts and Stop. It builds on Epics 1 to 3 and does not reopen Story 1.8.

### Story 4.1: Set the length of Sit, Stand, and Move in Settings

As the watch owner,
I want to set how long Sit, Stand, and Move last in the Settings menu,
So that the cycle fits my working day instead of the fixed 45/10/5.

**Requirements:** FR12, FR13, FR14, NFR7 (amended), NFR12, NFR13, UX-DR21 (length rows), UX-DR22 (step and wrap), UX-DR23.

**Dependencies:** Builds on Epics 1 to 3. Story 1.8 is closed by owner waiver and is not reopened. Physical-device checks on Pebble Time 2 are owner checks and do not block this story. Emulator evidence is recorded as emulator evidence.

**Acceptance Criteria:**

**Given** I open Settings in any lifecycle state (inactive, running, `PAUSED`, `BREAK`)
**When** the window opens
**Then** it lists the four haptic rows as before, followed by `Sit`, `Stand`, and `Move`, each showing its current value as text (for example `Sit 45 min`), and introduces no other new surface.

**Given** no length has ever been saved
**When** the app runs
**Then** the lengths are 45, 10, and 5 minutes, and behavior is unchanged from today.

**Given** I select a length row
**When** I press Select
**Then** its value rises by 5 minutes, wraps from 60 back to 5, is saved, and the row text shows the new value. The window stays open, no haptic preview plays, and Back leaves the window with the saved values.

**Given** saving a value fails
**When** I press Select on a length row
**Then** the row keeps showing the previous value and the stored value is unchanged.

**Given** I select a haptic row
**When** I press Select
**Then** it behaves as in Stories 3.1 and 3.2 (save, one preview, return), and the haptic rows are otherwise unchanged.

**Given** the lengths are saved
**When** I stop and start a session, or relaunch the app
**Then** the lengths are still in effect and are not reset by Stop, Start, or session-record recovery.

**Given** a stored value is missing, has the wrong size, is outside 5 to 60, or is not a multiple of 5
**When** the app reads it
**Then** that phase uses its default without a visible error, and the other phases keep their valid values.

**Given** the lengths are stored
**When** persistence is reviewed
**Then** they use their own persist key, separate from the haptic key and from `SESSION_STORE_KEY`, and the versioned session record schema is unchanged.

**Given** a phase starts (automatic transition, Skip, Start, or the rebase on overdue recovery)
**When** the coordinator sets its deadline
**Then** it reads that phase's configured length through `prv_duration()`, and the UI computes no deadlines.

**Given** a phase is running and I change its length or another phase's length
**When** the change is saved
**Then** the running phase keeps its deadline and its Wakeup, nothing is rescheduled, and the new length applies the next time that phase starts.

**Given** the session is `PAUSED` or `BREAK`
**When** I change lengths and then resume
**Then** the phase continues with its saved remaining time, and the timed-pause choices (5, 10, 30) and Extend are unchanged.

**Given** a phase ends naturally, with a changed or the default length
**When** the tally updates
**Then** it counts as completed as before, and skipped or in-progress phases stay excluded.

**Given** Settings is open while a deadline comes due
**When** the transition is processed
**Then** the coordinator handles it as before, with the chosen haptic pattern and the configured length of the new phase.

**Given** the longest row (for example `Stand 60 min`) and all rows of the Settings window
**When** the window is shown on the emulator
**Then** no row is clipped or overlapped and the window scrolls natively.

**Given** the change is built
**When** the host tests run (`scripts/test.sh`)
**Then** they cover the defaults, valid and invalid stored values, the length used at each phase transition and at Skip, the wrap from 60 to 5, and the unchanged running phase, and all existing tests still pass.

**Given** the feature is checked on the emulator and, if available, on Pebble Time 2
**When** evidence is recorded
**Then** each result is recorded with its source, emulator results are not presented as physical-device results, and anything unverified is stated.

`DESIGN.md` and `EXPERIENCE.md` are updated in the same change to add the length rows.

### Story 4.2: Reset the phase lengths to 45/10/5

As the watch owner,
I want to reset all three phase lengths to their defaults in one action,
So that I can get back to the standard 45/10/5 cycle without stepping each value around.

**Requirements:** FR15, UX-DR21 (Reset row), UX-DR22 (Reset).

**Dependencies:** Builds on Story 4.1 (the length rows and their own persist key). Story 1.8 is closed by owner waiver and is not reopened. Physical-device checks are owner checks. Emulator evidence is recorded as emulator evidence.

**Acceptance Criteria:**

**Given** I open Settings in any lifecycle state
**When** the window opens
**Then** `Reset` is the last row, after `Sit`, `Stand`, and `Move`, and the other rows are unchanged.

**Given** the lengths are not 45/10/5
**When** I select `Reset` and press Select
**Then** Sit, Stand, and Move are saved as 45, 10, and 5 minutes, the three rows show the default values, and the window stays open.

**Given** the lengths are already 45/10/5
**When** I select `Reset`
**Then** nothing changes and no error is shown.

**Given** saving the default values fails
**When** I select `Reset`
**Then** the rows and the stored values still show the previous values, and no value is left half reset.

**Given** I select `Reset`
**When** the values are reset
**Then** the chosen haptic pattern is unchanged and no haptic preview plays.

**Given** a phase is running, or the session is `PAUSED` or `BREAK`
**When** I select `Reset`
**Then** the running phase keeps its deadline and its Wakeup, the pause keeps its remaining time, and the default lengths apply the next time each phase starts (as in Story 4.1).

**Given** I selected `Reset` and then restart the app
**When** it reads the lengths
**Then** they are still 45/10/5, and Stop, Start, and recovery change nothing.

**Given** the change is built
**When** the host tests run (`scripts/test.sh`)
**Then** they cover reset from changed values, reset at the defaults, a failed write leaving no half-reset state, and the unchanged haptic pattern, and all existing tests still pass.

**Given** the feature is checked on the emulator and, if available, on Pebble Time 2
**When** evidence is recorded
**Then** each result is recorded with its source, emulator results are not presented as physical-device results, and anything unverified is stated.

`DESIGN.md` and `EXPERIENCE.md` are updated in the same change to add the Reset row.
