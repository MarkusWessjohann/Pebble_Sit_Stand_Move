# Epic 1 Context: Run and control a sit–stand–move session from the watch

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

Deliver a watch-first Pebble Time 2 app that lets its owner run a repeating Sit/Stand/Move reminder session and control it entirely from the watch. Story 1.1 establishes exact pinned toolchain and reproducible build/install readiness, plus available emulator evidence, before implementation. The finished app's device-dependent Wakeup, haptic, app foregrounding, timing tolerance, operational limits, reliability, and UI acceptance are verified on physical hardware in Story 1.8 before release.

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

- Target Pebble Time 2; watch operation must not require phone interaction during a session. Timing should continue after leaving the app; operation after explicit app termination is not required.
- The cycle is Sit 45 minutes, Stand 10 minutes, Move 5 minutes, starting at the user's actual session start and not aligned to wall-clock boundaries. UTC epoch deadlines are authoritative. Reconcile due deadlines before commands using `now >= deadline`; overdue phase recovery processes elapsed phases in order and rebases the resulting phase to its full default duration from recovery time.
- Support Start/Stop, indefinite Pause/Resume, timed pauses of exactly 5/10/30 minutes, Skip, and additive +5/+10-minute extensions. A timed pause preserves the interrupted phase's remaining time and resumes that same phase. Skip enters the next phase at its default duration; extension affects only the current deadline.
- Automatic phase transitions, timed-pause completion, and Skip request at most one haptic as specified; Start, manual Pause/Resume, Stop, and Extend are silent. Do not replay stale reminders. Physical delivery and foreground behavior are not guaranteed until verified.
- Persisted session state and Wakeup identity must be recoverable and stale callbacks rejected. On clock invalidity, Start remains inactive with a visible error; an active session preserves committed state and suppresses scheduling/haptics until valid UTC returns and reconciliation occurs. Persistence or scheduling failures must retain committed state, be visible, and never imply an uncommitted mutation is active or reliably scheduled.
- Provide only the session view and native watch menu. No sensing, health claims, analytics/history, schedules, cloud/social services, required phone companion, or configurable default phase lengths.
- The session view textually identifies phase/lifecycle, remaining time, current watch time, daily naturally completed phase totals, and errors. Use native Pebble presentation; color/icons are supplementary. Daily totals aggregate across sessions for the local calendar day, persist through app restarts, exclude skipped/in-progress phases, and reset at local midnight.
- Before implementation, Story 1.1 pins exact CLI, Python, and C SDK package/version and records reproducible build/install evidence using the available workflow, with emulator observations kept distinct from physical-device evidence.
- Before release, Story 1.8 records exact device firmware/OS and build/toolchain identifiers and tests finished-app Wakeups after leaving the app, haptic and app foregrounding, normal/overdue behavior, timing tolerance, Wakeup limits, battery/reliability, and finished UI/controls on target hardware.

## Technical Decisions

- Implement one native Pebble C watch app with an event-driven state machine. The Session Coordinator is the only mutation boundary for lifecycle, phase, deadlines, persistence, and Wakeup scheduling. UI renders coordinator state and dispatches commands; it does not own timers or resolve races.
- Use one compact, versioned canonical private session record with lifecycle, generation, active phase, haptic-attempt marker, and timing/Wakeup data valid for that lifecycle. Treat invalid/unsupported records as inactive with a recoverable error; never infer active state from partial data.
- Use Pebble Wakeup API for the next phase or timed-pause deadline, storing one WakeupId and generation cookie. Validate lifecycle, generation/cookie, ID, and due deadline on callback. On launch, clear orphan wakeups, reconcile committed state, and repair scheduling. Follow schedule/commit ordering: schedule candidate, persist matching record/ID, then cancel the old wakeup; cancel candidate and retain committed state if persistence fails.
- Persist the haptic-attempt marker before calling the haptic API. Ignore callbacks made stale by lifecycle/deadline mutation. Retry failed proposals only on specified explicit user commands.
- Pin exact Rebble CLI and Pebble C SDK package versions; the documented CLI baseline is `pebble-tool` 5.0.18 with Python 3.13, but exact compatibility and SDK package must be verified. Architecture lists maximum eight wakeups per app and a one-minute exclusion window; confirm on target firmware. Keep persistent record within Pebble storage limits.

## UX & Interaction Patterns

- Present one foreground session view and the native Pebble menu only. View order: small current time, prominent remaining phase time, monochrome phase icon, readable phase label, then small daily tally.
- Menu shows Start when inactive; while active it shows Pause/Resume, Skip, Stop in that order. Running hardware controls: top +5, middle starts a 30-minute timed break, bottom +10. In timed pause, middle resumes early; top and bottom select the shorter and longer alternative and show available durations. Label indefinite pause `PAUSED` and timed pause `BREAK`.
- Use concise, neutral text for clock, persistence, and Wakeup/scheduling errors. Verify legibility, safe layout, controls, and native assets on Pebble Time 2 hardware.

## Cross-Story Dependencies

- Story 1.1 is the pre-implementation toolchain/readiness gate; it does not establish physical-device capability. Stories 1.2–1.7 depend on its completion and then implement the finished app. Story 1.8 depends on Stories 1.2–1.7 and is the final Epic 1, pre-release physical-device acceptance gate. If a required physical capability cannot be verified or fails, document the limitation and block release pending a product-owner decision.
- Story 1.2 establishes the coordinator, canonical session record, lifecycle, view, and launch-repair foundation used by subsequent phase and pause stories.
- Story 1.3 supplies phase progression, overdue recovery, and natural-completion tally behavior; Stories 1.4–1.7 add lifecycle controls and must reconcile due deadlines through the same coordinator before applying commands.
