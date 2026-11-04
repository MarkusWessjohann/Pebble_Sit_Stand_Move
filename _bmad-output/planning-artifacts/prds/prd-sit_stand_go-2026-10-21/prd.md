---
title: Sit Stand Move — Pebble Reminder App
status: final
created: 2026-10-21
updated: 2026-11-04
---

# PRD: Sit Stand Move

## 0. Document Purpose

This PRD defines the first version of Sit Stand Move, a personal Pebble Time 2 watch app. It guides UX, architecture, and implementation planning, building on `_bmad-output/planning-artifacts/briefs/brief-sit_stand_go-2026-10-21/brief.md`. **Build readiness:** before implementation, pin the exact toolchain and demonstrate reproducible build/install readiness with the available workflow and emulator. Verify the finished app's physical Pebble Time 2 behavior as the pre-release gate in Story 1.8, after Stories 1.2–1.7.

## 1. Vision

Sit Stand Move helps its owner interrupt long periods of sitting with a repeating 45-minute Sit, 10-minute Stand, and 5-minute Move schedule. The owner can start a session at any wall-clock time, such as 09:10. The first 45-minute Sit phase begins immediately; subsequent phases are relative to actual deadlines, not aligned to full-hour boundaries. The owner can pause, skip, or extend an active phase by 5 or 10 minutes. The three phase lengths are defaults; the owner can change each one in Settings (FR-12).

The product succeeds when phase deadlines reliably produce a haptic signal and open the app to show phase and remaining time, with no phone interaction during a session. After any overdue deadline, the app processes elapsed phases in order, restarts the resulting phase at full default duration, and emits only one haptic. The watch firmware must be verified before this behavior is treated as deliverable.

## 2. Target User

### 2.1 Jobs To Be Done

- When focused on work or another seated activity, remind me to stand and move without requiring me to watch the clock.
- When a phase does not fit the moment, let me skip it or extend it without abandoning the session.
- Let me control a reminder session directly from my watch.

### 2.2 Key User Journey

- **UJ-1. The owner runs a Sit–Stand–Move session.** The owner starts a session on Pebble Time 2 and returns to other activities. At an on-time phase deadline, the watch vibrates and opens the app to show the active phase and remaining time. The owner may pause/resume, skip, or extend by 5 or 10 minutes. Stop ends the session; starting again begins with Sit. If a deadline is overdue on recovery, elapsed phases are processed in order, the resulting phase restarts with its full default duration, and one haptic is emitted. **[ASSUMPTION: Pebble Time 2 firmware foregrounds the app on Wakeup; verify on device.]**
- **UJ-2. The owner pauses a phase for a set interval.** The owner chooses 5, 10, or 30 minutes. The current phase is suspended with its remaining duration saved. At the timer's end, the watch opens the app, resumes that same phase, shows its remaining duration, and requests one haptic. It does not advance to the next phase at pause end. **[ASSUMPTION: Pebble Time 2 can foreground the app on the timed-pause Wakeup; verify on device.]** If the owner resumes early, the pause Wakeup is cancelled; a stale callback neither requests a haptic nor reopens the app.

## 3. Glossary

- **Session** — One user-started run of the repeating movement schedule.
- **Phase** — One timed activity within a Session: Sit, Stand, or Move.
- **Cycle** — The ordered sequence of Sit, Stand, and Move phases; the default Cycle lasts 60 minutes, and the configured Cycle lasts the sum of the three configured Phase lengths.
- **Sit** — The seated phase, 45 minutes by default.
- **Stand** — The standing phase, 10 minutes by default.
- **Move** — The movement phase, 5 minutes by default.
- **Configured length** — The length the owner set for a Phase in Settings (5 to 60 minutes in steps of 5); it equals the default until the owner changes it.
- **Timed Pause** — A user-selected 5-, 10-, or 30-minute suspension of the active Phase; at its end the same Phase resumes with its saved remaining duration.

## 4. Features

### 4.1 Session and phase timing

Throughout §4 and §8, the figures 45, 10, and 5 minutes and the words "default duration" are the default Phase lengths. When the owner has set other lengths (FR-12), every rule that names a default duration applies to the configured length of that Phase instead, read when the Phase starts.

The user starts and stops a Session on Pebble Time 2 at any wall-clock time. A Session begins with Sit and repeats the Cycle in order from the actual Start time, not at a full-hour boundary. At each on-time Phase deadline, the app advances, emits one haptic, and opens the UI to show the new Phase and remaining time. After recovery when `now >= deadline`, it processes elapsed phases in order and restarts the resulting Phase with its full default duration from recovery time. Move advances to Sit for the next Cycle.

#### FR-1: Start and stop a Session **[ASSUMPTION: Sessions are started and stopped manually.]**

The user can start and stop a Session from Pebble Time 2 at any wall-clock time. A newly started Session begins with Sit only when UTC time is valid; its 45-minute deadline is measured from actual Start time. Otherwise the Session remains inactive and the app shows a clock-time error. Stopping ends the active Session.

**Consequences (testable):**
- Starting at any time with valid UTC time makes Sit the active Phase with 45 minutes remaining and sets its deadline to Start time + 45 minutes. Starting with invalid/unavailable UTC time leaves the Session inactive and displays an error.
- Stopping an active Session makes the Session inactive and prevents subsequent phase-change reminders for that Session.
- Starting a Session after stopping begins at Sit, not at the previously active Phase. **[ASSUMPTION: Stopped Sessions are not resumable.]**

#### FR-2: Advance through the Cycle

During an active, unpaused Session, the app advances from Sit to Stand to Move to Sit in that order using the default Phase durations. The session deadline is an absolute UTC epoch time; a forward device-clock correction may make it due, while a backward correction delays it.

**Consequences (testable):**
- Sit lasts 45 minutes, Stand lasts 10 minutes, and Move lasts 5 minutes unless the user skips or extends the active Phase.
- When a Phase ends, the next Phase becomes active and its remaining time reflects its default duration.
- After Move ends, the next Phase is Sit and the Cycle repeats.

#### FR-3: Notify at phase changes

On an on-time automatic phase transition, the watch emits one haptic and opens the app UI to show the active Phase and remaining time. A haptic is not a readable text notification. When recovering with `now >= deadline`, the app processes elapsed phases in order, rebases the resulting Phase to its full default duration from recovery time, and emits at most one haptic. **[ASSUMPTION: Pebble Time 2 foregrounds the app on a scheduled Wakeup; verify on device.]**

**Consequences (testable):**
- Each on-time automatic Phase transition emits one haptic signal and opens the app UI to show the new Phase and its remaining time on Pebble Time 2.
- After any overdue deadline is reconciled, including one late deadline, elapsed phases are processed in order, the resulting current Phase starts at its full default duration from recovery time, and no more than one haptic is emitted.
- Notifications for this Session cease after the user stops it.

### 4.2 Session controls

The user can adjust the active Session on Pebble Time 2. Manual Pause freezes the phase with remaining duration preserved; Resume continues that phase. Timed Pause suspends the phase for 5, 10, or 30 minutes and resumes that same phase at pause end. Skip advances immediately and vibrates once. Extend adds +5 or +10 minutes to the active Phase deadline and is silent. Start, manual Pause, Resume, Stop, and Extend do not emit haptics.

#### FR-4: Pause and resume

The user can manually pause an active Session and resume it from Pebble Time 2. Manual Pause preserves the active Phase and its remaining duration without an automatic resume deadline.

**Consequences (testable):**
- While manually paused, the active Phase does not advance and no phase haptic is produced until the user resumes.
- Resuming continues the same Phase with the time remaining when the Session was paused.
- Pause and resume do not change the Phase order or configured default durations.

#### FR-5: Timed pause

The user can suspend the active Phase for 5, 10, or 30 minutes. The phase deadline is suspended and remaining duration preserved. At pause end, Pebble Time 2 should foreground/open the app, show the resumed Phase and remaining time, and request one haptic. The same Phase resumes; pause end is not phase completion and does not advance the Cycle. Physical haptic delivery is best-effort, at most one attempt.

**Consequences (testable):**
- The timed-pause choices are exactly 5, 10, and 30 minutes.
- No phase time elapses during the timed pause; at pause end the same Phase resumes with the saved remaining duration.
- At most one haptic is requested and an app-open/foreground is requested at timed-pause end; no phase transition is emitted.
- If the user manually resumes before pause end, the pause-end Wakeup is cancelled; any already delivered stale callback is ignored without haptic or UI reopen.
- If the timed-pause wakeup is overdue on recovery, process it once, resume the preserved Phase, and emit at most one haptic.

**Platform gate:** Timed-pause Wakeup must pass the finished-app Pebble Time 2 physical-device acceptance test in §5.1 and Story 1.8 before release.

If the user resumes manually before pause end, cancel the pause-end Wakeup; any already delivered stale callback must request neither a haptic nor an app reopen.

#### FR-6: Skip a Phase

The user can skip the active Phase. The Session immediately advances to the next Phase in the Cycle and uses that Phase's default duration.

**Consequences (testable):**
- Skipping Sit advances to Stand; skipping Stand advances to Move; skipping Move advances to Sit.
- A skip does not add the skipped Phase's remaining time to another Phase.
- Skip updates the app UI to the newly active Phase and remaining time and emits one haptic signal.
- Start, Pause, Resume, and Stop do not emit haptics.

#### FR-7: Extend a Phase

The user can extend the active Phase by +5 minutes or +10 minutes. The selected amount is added to the current deadline; repeated extensions stack, without changing default phase durations. Extend is silent; the later automatic phase transition emits its normal haptic.

**Consequences (testable):**
- The user can select +5 minutes or +10 minutes from the watch interface.
- Selecting an extension does not emit a haptic; the resulting automatic phase transition still emits one.
- The active Phase does not advance before its remaining time plus the selected extension elapses, unless the user skips or stops it.
- After the extension, the Session advances to the next Phase in the Cycle.

## 5. Cross-Cutting Requirements

### 5.1 Platform and background behavior

- The app is intended to be controlled from the Pebble watch without phone interaction during a Session.
- The target device is Pebble Time 2.
- Phase timing must continue after the user leaves the app so it is no longer in the foreground, without phone interaction during a Session. Continued operation after explicit app termination is not required.
- Rebble's developer site documents SDK 4 and includes guides for background workers and wakeups ([SDK 4](https://developer.rebble.io/sdk4), [Developer Guides](https://developer.rebble.io/guides/)). These general SDK references do not establish which mechanisms are supported by the Pebble Time 2 firmware or guarantee the required phase-notification behavior.
- **Pre-implementation readiness:** pin the exact CLI, Python, and C SDK package/version and demonstrate a reproducible build/install using the available workflow; record available emulator results separately. Emulator results do not establish physical-device behavior.
- **Pre-release acceptance:** Story 1.8 verifies the finished app on Pebble Time 2, including phase-deadline and timed-pause-end Wakeups after leaving the app, haptic and app foreground behavior, timing, operational limits, reliability, and finished-UI acceptance. If a required capability is unsupported, revisit the affected feature with the user before release.
- **[OPEN]** Pebble Time 2 firmware/OS version and whether Wakeup can both emit a haptic and foreground the finished app after the user leaves it; operation after explicit app termination is not required. Resolve through Story 1.8 before release.

### 5.2 Timing and reliability

- Phase transitions use UTC epoch wall time even when the device clock is manually/system adjusted: a forward correction that reaches a deadline makes it due; a backward correction delays it. Invalid UTC at Start leaves the Session inactive with a visible clock-time error. If UTC becomes invalid during a running Session, preserve state and suppress scheduling/haptics until valid time returns, then reconcile the deadline.
- Before physical acceptance, agree a measurable phase-transition tolerance and verify it on Pebble Time 2 using the finished app; test deadline/control races using `now >= deadline`.

#### FR-9: Choose the haptic pattern

The user can open Settings from the native watch menu, in every session state, and choose the haptic pattern: Short, Long, Double, or Until Select. The choice persists across app restarts, Stop, and Start, and defaults to Short (the single short pulse). The haptic pattern is the only notification setting; there is no alternate channel. The Phase lengths (FR-12) are not a notification setting.

**Consequences (testable):**
- Settings is the last native menu row in every lifecycle state, and its window lists the four patterns in that order with the current choice marked in text.
- Selecting a pattern saves it and gives a single preview; Back leaves it unchanged.
- A missing or corrupt stored value falls back to Short without an error.

#### FR-10: Apply the pattern to every haptic request

The chosen pattern applies to every haptic request: automatic phase transition, timed-pause end, and Skip. Start, manual Pause, Resume, Stop, and Extend stay silent whatever the pattern. A haptic is still one attempt per committed transition.

#### FR-11: Repeat the alert until Select

With Until Select, the haptic repeats until the user presses Select, then stops. That press is consumed and does not trigger the middle-control action. The repeat also stops after 2 minutes, on Stop, when a newer haptic transition supersedes it, and when the app exits. A relaunch never resumes it.

**Consequences (testable):**
- While the alert repeats, the session view shows `Select to stop`.
- Pressing Select while it repeats starts no timed pause and changes no session state.
- With no alert repeating, Select behaves as before.

### 5.3 Phase lengths

#### FR-12: Set the length of each Phase

The user can open Settings and set the length of Sit, Stand, and Move. Each length is chosen from 5 to 60 minutes in steps of 5. The defaults are 45, 10, and 5 minutes, so a user who never opens the setting sees the original behavior. Move keeps its name.

**Consequences (testable):**
- The Settings window lists the three lengths after the haptic patterns, each with its current value as text, for example `Sit 45 min`. Selecting a length row adds 5 minutes, wraps from 60 back to 5, saves, and keeps the window open with no preview.
- A failed save leaves the previous value stored and shown.

#### FR-13: Keep the lengths across restarts

The three lengths persist across app restarts, Stop, and Start, independently of the Session record. A missing, wrong-sized, out-of-range, or not-a-multiple-of-5 stored value falls back to the default of that Phase without an error; the other Phases keep their valid values.

#### FR-14: Apply a changed length to later Phases only

A changed length applies to Phases that start after the change: an automatic phase transition, Skip, a new Start, and the full-length rebase after an overdue deadline. The running Phase keeps its deadline, a paused or timed-paused Phase keeps its saved remaining time, and no Wakeup is rescheduled because of the change. Timed Pause choices (5, 10, 30 minutes) and Extend amounts do not change. The daily tally keeps counting naturally completed Phases whatever their length; skipped and in-progress Phases stay excluded.

**Consequences (testable):**
- With Sit set to 30 and Stand to 5, a Session that is overdue by several days recovers to the Phase that a repeating 30/5/5 Cycle would be in, not the Phase a 60-minute Cycle would give.

#### FR-15: Reset the lengths

Settings offers a `Reset` entry after the length rows. Selecting it restores 45, 10, and 5 minutes in one save, updates the rows, keeps the window open, plays no preview, and leaves the haptic pattern unchanged. A failed save leaves the previous values stored and shown.

## 6. Non-Goals

- Detecting posture, steps, or movement automatically.
- Health advice, health outcome claims, activity history, or adherence analytics.
- Social features, cloud services, or a required phone companion.
- Scheduled working hours or automatic overnight start/stop.
- Any notification customization other than the haptic pattern (FR-9), in the first version.
- Customizing timed-pause choices (5, 10, 30 minutes) or extension amounts (+5, +10), and Phase lengths outside 5 to 60 minutes or in steps other than 5.

## 7. MVP Scope

### 7.1 In Scope

- Pebble Time 2 app with on-watch Session start and stop.
- Repeating 45-minute Sit, 10-minute Stand, and 5-minute Move Cycle.
- On-time phase deadlines emit haptics and open the app UI to show the new Phase and remaining time.
- Manual pause/resume; timed pause for 5, 10, or 30 minutes; skip; and +5/+10-minute Phase extension.
- Background reminders, subject to the pre-release physical-device acceptance gate in §5.1 and Story 1.8.
- Owner-selectable haptic pattern (Short, Long, Double, Until Select) in a Settings menu (FR-9 to FR-11).
- Owner-set length for each of Sit, Stand, and Move (5 to 60 minutes, steps of 5) and a Reset to 45/10/5, in the same Settings menu (FR-12 to FR-15).

### 7.2 Out of Scope for MVP

- Activity sensing and analytics, to keep this a reminder utility.
- Automatic schedules, to keep operation user-started and controlled.
- Cloud, social, or required phone-companion features, because a watch-first personal tool does not need them.

## 8. Success Metrics

- **SM-1:** After Story 1.8 pre-release acceptance, the owner can observe a full 45/10/5 Cycle, the app requests haptics for automatic phase deadlines, timed-pause end, and Skip, and shows the active/resumed Phase and remaining time when opened on Pebble Time 2. Validates FR-1–FR-8.
- **SM-2:** The owner can manually pause/resume, choose a 5/10/30-minute timed pause, skip, and extend without losing the active Session. Skip requests one haptic; timed-pause end requests at most one without advancing; Extend is silent. Early manual resume makes stale pause-end callbacks silent and prevents app reopen. Validates FR-4–FR-7.
- **SM-C1 (counter-metric):** Avoid repeated or stale haptics after a Session is stopped or paused; reminder volume should not increase at the expense of control. Counterbalances SM-1. The Until Select pattern (FR-11) is a deliberate, owner-chosen exception: it repeats one committed attempt, stops on Select or after 2 minutes, and never resumes after relaunch.

- **SM-3:** If the app recovers when a phase deadline is due/overdue, it processes elapsed phases in order and rebases the current Phase with its full default duration; if a timed-pause deadline is due/overdue, it resumes the interrupted Phase with saved remaining duration and no Cycle advancement. Haptic invocation is at most once. Validates FR-2, FR-3, FR-5.
- **SM-4:** Skip advances and requests one haptic; timed-pause end resumes the same Phase and requests one; early manual resume prevents stale haptic/app reopen; Start, manual Pause, Resume, Stop, and Extend request none. Validates FR-1, FR-4–FR-7.

For this personal tool, success is judged through direct use and timing checks rather than analytics or telemetry.

## 9. Open Questions

1. Which Pebble Time 2 firmware/OS is the support target, and does Wakeup after leaving the finished app emit a haptic and foreground the UI? **Owner: project owner; resolve at the Story 1.8 pre-release gate.**
2. What measurable phase-transition tolerance is acceptable on Pebble Time 2? **Owner: project owner; agree before Story 1.8 timing acceptance.**
3. A separate Pebble system missed-wakeup alert is disabled in v1; revisit only if target testing shows user value. **Owner: product; reconsider after device testing.**

## 10. Assumptions Index

- §4.1 FR-1: Sessions start and stop manually.
- §2.2 and §4.1 FR-1: A stopped Session is not resumable, and a newly started Session begins with Sit.
- §2.2 and §4.2 FR-4: Pause and resume are included in v1.
- §4.2 FR-5: Timed Pause freezes remaining phase duration and resumes the same Phase on pause-end Wakeup.
- §4.1 FR-3: Phase changes use a haptic signal; phase name and remaining time are shown in the opened app UI.
- §2.2 UJ-1 and §4.1 FR-3: Pebble Time 2 foregrounds the app on scheduled Wakeup; verify on device.
- §5.2: UTC time must be valid to start; otherwise the Session remains inactive with a visible clock-time error.
- §5.3 FR-14: A changed Phase length applies from the next start of a Phase; the owner accepted that a running Phase keeps its current deadline.
