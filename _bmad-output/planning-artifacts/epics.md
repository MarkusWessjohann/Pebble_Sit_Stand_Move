---
status: final
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

### NonFunctional Requirements

NFR1: The application targets Pebble Time 2 and must be operable from the watch without phone interaction during a session. Timing must continue after the app leaves the foreground; operation after explicit app termination is not required.

NFR2: Phase and timed-pause deadlines use UTC epoch wall time. Forward clock corrections can make a deadline due; backward corrections defer it. When UTC is invalid, a new session remains inactive with a visible error; an active session preserves saved state and suppresses scheduling/haptics until valid time returns, then reconciles.

NFR3: Define and verify a measurable phase-transition tolerance on the target firmware before implementation. Verify Pebble Time 2 firmware/OS, CLI/SDK versions, Wakeup behavior after leaving the app, haptic delivery, and app foregrounding on physical hardware. Firmware-specific delivery is not guaranteed until these acceptance checks pass.

NFR4: Serialize user commands, Wakeup callbacks, launches, and recovery through one FIFO session coordinator. Reconcile a due deadline before applying a competing command using `now >= deadline`; UI must not independently resolve session timing or races.

NFR5: Persist session state and Wakeup identity/generation so stale callbacks are rejected. Haptic attempts are at-most-once for a committed transition; do not replay stale haptics after recovery. On persistence or scheduling failure, retain the last committed state as specified, surface a visible error, and do not claim an uncommitted session mutation is active or reliably scheduled.

NFR6: Haptic is the only v1 notification mode. Skip, automatic phase deadlines, and timed-pause completion request a haptic; Start, manual Pause, Resume, Stop, and Extend do not. Physical haptic delivery is best-effort and must be verified on the target device.

NFR7: Preserve the reminder-only MVP scope: no posture/step/movement sensing, health claims, history/adherence analytics or telemetry, automatic schedules, cloud/social services, required phone companion, or configurable default phase lengths.

NFR8: The watch interface must remain legible and textually identify phase, lifecycle state, remaining time, daily totals, and errors using native Pebble presentation. Do not rely on color alone; verify legibility and layout on Pebble Time 2 hardware.

NFR9: Build and installation must be reproducible using pinned, verified Pebble CLI/SDK versions. Resolve the SDK `latest` reference to an exact package version and record build/install and physical-device verification results before implementation is considered ready.

### Additional Requirements

- The architecture specifies one native Pebble C watch app using an event-driven state machine. The Session Coordinator is the sole owner and mutation boundary for lifecycle state, active phase, deadlines, persistence, and Wakeup scheduling; the UI renders coordinator state and dispatches commands.
- Use a canonical, versioned private session record with lifecycle (`inactive`, `running`, `paused`, `timed_paused`), generation, active phase, haptic-attempt marker, and state-specific timing. Invalid or unsupported stored schema is treated as inactive with a recoverable error; never infer active state from a partial record.
- Use Pebble Wakeup API for the next phase or pause deadline, with one persisted WakeupId and generation cookie. Validate lifecycle, cookie/generation, WakeupId, and deadline on callbacks; cancel stale/orphaned wakeups and repair scheduling on app launch according to the architecture's commit-ordering rules.
- Schedule/commit ordering and failures must follow AD-11 and AD-13: do not report a proposed mutation as committed if persistence or Wakeup scheduling fails; retain last committed state and only retry failed proposals on the specified explicit user commands.
- Haptic-attempt state must be persisted before calling the haptic API. Retry only committed transitions without an attempt marker; never replay a marked attempt.
- Process due phase and timed-pause events through the coordinator before commands; generation increments on lifecycle/deadline mutations. Ignore callbacks made stale by pause, resume, stop, skip, extension, or newer scheduling.
- Record exact target firmware/OS and toolchain identifiers; test normal and overdue deadlines, timed pause choices and early resume, stale callbacks, skip/extension scheduling, command/deadline races, clock validity and corrections, persistence/schedule/cancel failures, launch repair, and crash boundaries on target hardware.
- Resolve and pin the Pebble C SDK and CLI; use the documented `pebble-tool` 5.0.18 / Python 3.13 baseline only after verifying exact compatibility. Verify Wakeup constraints and battery/reliability on device. Keep the compact session record within Pebble persistent-storage limits.
- No starter template is specified in the Architecture. The suggested `src/` files are a structural seed, not a required layout. No separate migration, cloud integration, or deployment service is specified.
- The product brief confirms watch-first use, the 45/10/5 schedule, manual controls, timed-pause behavior, deterministic recovery, UTC authority, and the out-of-scope items reflected above. Where brief wording differs in detail, the final PRD, Architecture, and UX contract provide the implementation-level requirements.
- Product/UX decisions confirmed: native menu order is Start when inactive and Pause/Resume, Skip, Stop while active; phase icons are monochrome chair/standing-person/moving-person paired with text; timed-pause alternatives use shorter-on-top and longer-on-bottom (30-minute entry shows 5/10); daily tally uses local calendar day, persists across app restarts, aggregates across sessions, and resets at local midnight.
- Physical-device decisions still to verify before implementation: Pebble Wakeup delivery after leaving the app, haptic delivery, app foregrounding, and measurable phase-transition tolerance on the target firmware.

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

### FR Coverage Map

FR1: Epic 1 - Start and stop a session from the watch.
FR2: Epic 1 - Run the repeating Sit/Stand/Move cycle with deadline-based recovery.
FR3: Epic 1 - Receive phase reminders and see the current phase in the app.
FR4: Epic 1 - Manually pause and resume a session.
FR5: Epic 1 - Take and end a timed pause, including early resume handling.
FR6: Epic 1 - Skip the active phase.
FR7: Epic 1 - Extend the active phase by +5 or +10 minutes.

## Epic List

### Epic 1: Run and control a sit–stand–move session from the watch

The owner can start and stop a session on Pebble Time 2, follow the repeating Sit/Stand/Move cycle, and manage it with pause, timed pause, skip, and extension controls. The watch shows the current phase and remaining time, and handles reminders and recovery according to the agreed timing rules. Target-device verification of Wakeup, haptic, and app-foreground behavior is a readiness gate for implementation.

**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6, FR7.

## Epic 1: Run and control a sit–stand–move session from the watch

The owner can start and stop a session on Pebble Time 2, follow the repeating Sit/Stand/Move cycle, and manage it with pause, timed pause, skip, and extension controls. The watch shows the current phase and remaining time, and handles reminders and recovery according to the agreed timing rules. Target-device verification of Wakeup, haptic, and app-foreground behavior is a readiness gate for implementation.

### Story 1.1: Verify Pebble Time 2 reminder capabilities

As the project owner,
I want to verify the target watch's reminder and build capabilities,
So that I know whether the planned session behavior is feasible before implementation proceeds.

**Requirements:** NFR1, NFR3, NFR9, UX-DR13.

**Acceptance Criteria:**

**Given** a Pebble Time 2 and the proposed development toolchain
**When** the verification is performed
**Then** the exact device firmware/OS, CLI version, and SDK version are recorded, and the CLI and SDK dependencies are pinned to exact versions.

**Given** the pinned toolchain and a minimal test app
**When** the documented build and install procedure is run
**Then** it produces a reproducible build/install on the target device, and the exact commands and results are recorded.

**Given** a minimal test app installed on the target watch
**When** phase-deadline and timed-pause Wakeups are tested after the app leaves the foreground
**Then** each test records whether the Wakeup occurs, requests a haptic, and opens or foregrounds the app UI.

**Given** the target-device acceptance test plan
**When** it is prepared for implementation
**Then** it defines observable checks for normal and overdue Wakeups, invalid UTC and clock corrections, timing tolerance, and Wakeup limits; app-level Pause/Resume, timed-pause, Skip, Extend, race, persistence, cancellation, launch-repair, and haptic-count scenarios are assigned to their implementing stories and do not block this platform-capability story.

**Given** the target firmware and app configuration
**When** Wakeup limits and sustained reminder use are tested
**Then** the documented Wakeup count and spacing constraints are respected and battery/reliability observations are recorded.

**Given** the target-device timing tests
**When** normal phase-deadline behavior is measured
**Then** the observed transition timing and an agreed measurable tolerance are documented.

**Given** any required Wakeup, haptic, or foreground behavior cannot be verified or does not work
**When** the verification results are reported
**Then** the unmet capability is clearly documented, and implementation of dependent reminder behavior remains gated for a product decision.

### Story 1.2: Start and stop a session from the watch

As the watch owner,
I want to start and stop a Sit/Stand/Move session and see its current status,
So that I can control reminders directly from Pebble Time 2.

**Requirements:** FR1, NFR1, NFR4–NFR5, NFR7–NFR8, UX-DR1–UX-DR5, UX-DR8–UX-DR13.

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
**Then** phase timing continues without phone interaction, subject to the target-device capability gate in Story 1.1.

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
