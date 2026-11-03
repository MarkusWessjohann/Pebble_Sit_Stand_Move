# Epic 1 Context: Run and control a sit–stand–move session from the watch

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

Deliver a watch-first Pebble Time 2 app for running and controlling a repeating Sit/Stand/Move reminder session without phone interaction. Users can start at any time, follow the cycle, and pause, take a timed break, skip, extend, or stop; missed deadlines recover deterministically. Reproducible toolchain/build readiness is required before implementation, while Wakeup, haptic, foreground, timing, reliability, and finished-UI behavior must be verified on the physical watch before release.

## Stories

- Story 1.1: Establish Pebble toolchain and emulator readiness
- Story 1.2: Start and stop a session from the watch
- Story 1.3: Follow the cycle and recover from missed deadlines
- Story 1.4: Manually pause and resume a session
- Story 1.5: Take and end a timed pause
- Story 1.6: Skip the active phase
- Story 1.7: Extend the active phase
- Story 1.8: Verify the finished app on Pebble Time 2 before release

## Requirements & Constraints

- Target Pebble Time 2. Sessions are controlled on-watch; timing continues after leaving the app, but need not continue after explicit termination.
- The fixed cycle is Sit 45 minutes, Stand 10 minutes, Move 5 minutes. Start begins Sit immediately; phases are relative to actual UTC epoch deadlines, not wall-clock boundaries. At on-time transitions, advance sequentially from the expired deadline. On recovery at `now >= deadline`, process elapsed phases in order and give the resulting phase its full default duration from recovery time.
- Start requires valid UTC; otherwise remain inactive and show a clock error. If UTC becomes invalid during an active session, preserve committed state and suppress scheduling/haptics until valid time returns, then reconcile. Forward clock changes may make deadlines due; backward changes defer them.
- Manual pause preserves remaining phase duration without an automatic deadline. Timed pauses are exactly 5, 10, or 30 minutes, suspend the phase clock, and resume the same phase with saved time. Skip advances immediately to the next phase at its default duration without counting the skipped phase as naturally completed. +5/+10 extensions add to the current deadline and stack without changing defaults.
- Automatic phase transitions, timed-pause completion, and Skip request a haptic; Start, manual Pause, Resume, Stop, and Extend are silent. Recovery requests at most one haptic for the reconciled state. Stopped sessions and stale callbacks must not produce later reminders. Physical delivery/foregrounding is subject to device verification.
- Only the session view and native watch menu are in scope. Exclude sensing, health claims, analytics/history, automatic schedules, cloud/social services, required phone companion, and configurable default durations.
- Show phase and lifecycle state, remaining time, watch time, daily naturally completed phase totals, and errors as readable text. Totals aggregate across sessions for the watch's local calendar day, persist across restarts, exclude skipped/in-progress phases, and reset at local midnight. Do not rely on color or icons alone.
- Story 1.1 pins Pebble Tool CLI 5.0.40, Python 3.13.5, and C SDK 4.33.1 and records reproducible build/install readiness; the SDK/toolchain path selected by a locked build remains to be captured. Emulator evidence is not physical-device acceptance. Story 1.8 records target firmware/OS and toolchain identifiers and verifies finished-app behavior, timing tolerance, operational limits, reliability, and UI on hardware before release.

## Technical Decisions

- Use one native Pebble C app with an event-driven state machine. The Session Coordinator is the sole owner and mutation boundary for lifecycle, phase, deadlines, persistence, and Wakeup scheduling. UI renders coordinator state and dispatches commands; it owns no timer and resolves no races.
- Use a compact, versioned private session record with lifecycle (`inactive`, `running`, `paused`, `timed_paused`), generation, active phase, haptic-attempt marker, and lifecycle-valid timing/Wakeup data. Invalid or unsupported records recover as inactive with a visible error; never infer active state from partial data.
- Serialize commands, Wakeup callbacks, launches, and recovery through one FIFO coordinator. Reconcile due deadlines before commands using `now >= deadline`. Use the Pebble Wakeup API for the next phase or pause deadline, with one persisted WakeupId and generation cookie; accept callbacks only when lifecycle, generation/cookie, ID, and deadline match. Clear orphan Wakeups and repair scheduling on launch.
- Follow schedule/commit ordering: schedule candidate, persist its matching record and WakeupId, then cancel the prior Wakeup. On persistence failure cancel the candidate and retain committed state; on scheduling failure do not present the proposed mutation as active/reliably scheduled. Retry failed proposals only on specified explicit user commands.
- Persist the haptic-attempt marker before calling the haptic API; never replay a marked attempt. Architecture notes at most eight scheduled Wakeups per app and a one-minute exclusion window; verify target behavior and keep the record within Pebble storage limits.

## UX & Interaction Patterns

- Provide one foreground session view and the native menu. View order: small current time, prominent remaining phase time, monochrome phase icon with readable Sit/Stand/Move label, then small daily tally. Use native Pebble styles, geometry, and safe area; verify fit and legibility on target hardware.
- Menu order: Start when inactive; Pause/Resume, Skip, Stop while active. While running, top is +5, middle starts a 30-minute timed break, and bottom is +10. During timed pause, middle resumes early; top and bottom select the shorter and longer available alternatives and display their durations (30-minute entry shows 5/10). Label indefinite pause `PAUSED` and timed pause `BREAK`.
- Use concise, neutral text for clock, persistence, and Wakeup/scheduling errors. Text—not color or icons—identifies phase and lifecycle state.

## Cross-Story Dependencies

- Story 1.1 is the pre-implementation toolchain/readiness gate. Stories 1.2–1.7 depend on it; the owner has waived this for Stories 1.2–1.6 while 1.1 stays open (the waiver does not close 1.1 or cover 1.7–1.8); Story 1.8 depends on Stories 1.2–1.7 and is the final pre-release physical-device acceptance gate. Unverified or failed required physical behavior blocks release pending a product decision.
- Story 1.2 establishes the coordinator, canonical record, lifecycle, view, and launch-repair foundation. Story 1.3 adds cycle progression, overdue recovery, and natural-completion tally behavior; Stories 1.4–1.7 add controls through the same coordinator and reconcile due deadlines before applying commands.
