---
title: Sit Stand Move — Watch Experience
status: final
created: 2026-10-23
updated: 2026-11-04
sources:
  - ../../briefs/brief-sit_stand_go-2026-10-21/brief.md
  - ../../prds/prd-sit_stand_go-2026-10-21/prd.md
  - ../../architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md
---

# Sit Stand Move — Experience Spine

## Foundation

Single-surface, watch-first experience for Pebble Time 2. No phone interaction is required during a session. Use native Pebble UI and physical controls. `DESIGN.md` owns visual identity and component placement; this spine defines behavior. The architecture’s single Session Coordinator owns the session lifecycle, phase and deadline state, and serialized event handling; the UI only renders its state and dispatches commands.

## Information Architecture

| Surface | Reached from | Purpose |
|---|---|---|
| Session view | App start/open; phase or timed-pause event requests open/foreground | Show current time, active phase and remaining time, phase icon, and today's completed Sit / Stand / Move counts. Provide the three right-side hardware controls. |
| Native watch menu | Opened through Pebble menu navigation | When inactive, show Start, then Settings. While active, show Pause/Resume, Skip, Stop, then Settings, with Pause or Resume selected according to lifecycle state. |
| Settings window | Native watch menu → Settings | Native menu window listing the haptic patterns Short, Long, Double, and Until Select in that order, then the phase lengths Sit, Stand, and Move, then Reset. Mark the current haptic choice in text. Selecting a haptic row saves it, plays one preview, and returns to the previous screen; Back leaves it unchanged. Each length row shows its value as text (`Sit 45 min`); selecting it adds 5 minutes (5 to 60, wrapping 60 to 5), saves, updates the row, and keeps the window open with no preview. A failed save leaves the old value shown. A new length applies to phases that start afterwards; the running phase is unchanged. Selecting `Reset` restores the three lengths to 45, 10, and 5 in one save, updates the rows, keeps the window open, plays no preview, and leaves the haptic pattern unchanged; a failed save leaves the previous values shown. |

There is one foreground session view, one native menu, and one Settings window for the haptic pattern and phase lengths; no phone, history, analytics, or other additional product surface is in scope. The session view's ordered layout and control placement are in `DESIGN.md`.

Verify finished-app controls, legibility, and firmware-dependent behavior on Pebble Time 2 in Story 1.8 before release.

## Voice and Tone

Use concise, neutral, functional labels. Names and control labels stay literal; the phase is always `Move`.

| Use | Avoid |
|---|---|
| `Sit`, `Stand`, `Move` | A different phase name than the name in the watch UI |
| `+5`, `Pause`, `Resume`, `+10` | Unspecified promotional or motivational copy |
| Clear clock, persistence, or wakeup failure text | Silently implying that a failed session change is active or scheduled |

Exact error sentence wording is [ASSUMPTION] and should be concise, actionable where possible, and consistent with Pebble's native conventions.

## Component Patterns

Behavioral rules; visual placement lives in `DESIGN.md` → Components.

| Component | Use | Behavioral rules |
|---|---|---|
| Session view | All lifecycle states | Render coordinator state; never own or advance the timer. Visual specification: `DESIGN.md` → Session view. |
| Session time | Session view | Displays current watch time at top, at secondary size (Gothic 24). Exact time format is [ASSUMPTION] native/local watch format. Visual specification: `DESIGN.md` → Session time. |
| Phase remaining | Session view | Display the derived remaining value for the current lifecycle state. Visual specification: `DESIGN.md` → Phase remaining. |
| Phase icon and name | Session view | Identify active `Sit`, `Stand`, or `Move`; pair a monochrome chair, standing-person, or moving-person icon with readable phase text directly below the countdown. Text is authoritative. Visual specification: `DESIGN.md` → Phase icon and name. |
| Day tally | Session view | Bottom tally at secondary size (Gothic 24) gives naturally completed phase totals for Sit, Stand, and Move (written `S3 T2 M1`) across sessions during the watch's local calendar day. A skipped phase is excluded. In-progress phases are not counted. Persist today's totals locally across app restarts and reset them at local midnight. Visual specification: `DESIGN.md` → Day tally. |
| Top hardware button | Session view | See the running and timed-pause control rules in Interaction Primitives. Visual specification: `DESIGN.md` → Top hardware button. |
| Middle hardware button | Session view | See the running, manual-pause, and timed-pause control rules in Interaction Primitives. Visual specification: `DESIGN.md` → Middle hardware button. |
| Bottom hardware button | Session view | See the running and timed-pause control rules in Interaction Primitives. Visual specification: `DESIGN.md` → Bottom hardware button. |
| Watch menu | Session view | When inactive, Start is the available session command, then Settings. While active, commands appear in this order: Pause/Resume, Skip, Stop, Settings. See Interaction Primitives. Visual specification: `DESIGN.md` → Watch menu. |
| Status message | Session view | Surface invalid clock, persistence, and wakeup/scheduling failures; do not represent a failed proposal as active or reliably scheduled. Visual specification: `DESIGN.md` → Status message. |
| Alert cue | Session view | While an Until Select alert repeats, show the text `Select to stop`. It must not overlap the countdown, icon, phase name, tally, button labels, or error text, and does not rely on color. Visual specification: `DESIGN.md` → Alert cue. |

The timed-pause control mapping is defined once under Interaction Primitives. The visual labels `BREAK` and `PAUSED` distinguish timed break from indefinite manual pause; see `DESIGN.md` → Middle hardware button.

## State Patterns

Session lifecycle follows architecture states: `inactive`, `running`, `paused` (manual pause), and `timed_paused`.

| State | Session view behavior | Rules |
|---|---|---|
| Inactive | Start is available in the menu. | Successful Start enters Sit with 45 minutes. Invalid UTC leaves the session inactive and presents a clock error. |
| Running | Display active phase and its remaining time. | Cycle and physical controls follow Interaction Primitives. |
| Manual `paused` | Show `PAUSED`, frozen phase, and saved remaining phase time. | No deadline or automatic resume. Resume explicitly from menu or middle control. |
| `timed_paused` | Show `BREAK`, remaining break time, and alternative duration labels. | Timed-pause controls and completion follow Interaction Primitives. Resume the same phase without cycle advancement. |
| Automatic phase transition | Show the newly active phase and remaining time if firmware opens/foregrounds the app. | Advance at due deadline; request one haptic. Physical delivery and foregrounding require device verification. |
| Skip transition | Show the next phase and its default remaining time. | Skip requests one haptic; exclude the skipped phase from day totals. |
| Until Select alert | Show the `Select to stop` cue. | One committed haptic attempt repeats until Select, a 2-minute cap, Stop, or supersession by a newer transition (AD-18). Select stops it and starts no timed pause or resume. Not resumed after relaunch. |
| Stopped | Session is inactive; Start is available in the menu. | A new session begins with Sit. |
| Invalid clock | Show a visible time error. | New session remains inactive. For an active session, preserve saved state and suppress scheduling/haptics until valid time returns, then reconcile the deadline. |
| Persistence or wakeup failure | Show a visible failure message. | Do not claim the new mutation is active or background-scheduled if commit/scheduling failed; retain the last committed state per architecture. |
| Overdue deadline recovery | Show the reconciled resulting phase and remaining time. | Phase and timed-pause recovery follow their respective Interaction Primitives rules; request at most one haptic. |

**Daily tally semantics:** count only naturally completed phases and aggregate across all sessions today; Skip never increments a count. “Today” means the watch's local calendar day, totals persist locally across app restarts, and counts reset at local midnight.

## Interaction Primitives

- **Start:** Menu command. With valid UTC, begins a new session immediately at Sit for 45 minutes. Silent. With invalid UTC, remain inactive and show an error.
- **Automatic phase deadline:** Process before applying a competing command using `now >= deadline`; advance as specified by the phase cycle. Request one haptic and request UI open/foreground, subject to Pebble firmware feasibility. Actual delivery remains unverified.
- **Right-side controls while running:** top `+5` and bottom `+10` extend the active phase by the labeled duration, additively and silently. Middle immediately starts a 30-minute timed break.
- **Right-side controls while timed-paused:** middle resumes the same phase with saved remaining phase duration and cancels the pause deadline. Top and bottom select the alternative duration printed beside that control and update remaining break time. Top always selects the shorter available alternative and bottom the longer: when 5 is active, labels show top 10 and bottom 30; when 10 is active, labels show top 5 and bottom 30; on entry with 30 active, labels show top 5 and bottom 10. Changing break duration does not change saved phase duration.
- **Timed break end:** Resume the same phase with saved remaining duration; do not advance the cycle. Request one haptic and app open/foreground. Manual early Resume cancels the pause Wakeup; stale callbacks neither request haptic nor reopen the app.
- **Manual Pause/Resume:** Menu command for indefinite pause. Freeze the phase and preserve remaining duration; show `PAUSED`. No automatic deadline; Resume continues the same phase. Both are silent.
- **Skip:** Menu command advances immediately to next phase at its default duration, requests one haptic, and excludes the skipped phase from tally.
- **Stop:** Menu command makes session inactive and prevents later reminders for that session. Silent. Next Start begins at Sit.
- **Settings:** Menu command, last row in every state. Opens the Settings window. Silent apart from the single preview when a pattern is selected.
- **Select while an alert repeats:** Stops the alert and is consumed; it enqueues no session event. With no alert repeating, Select is unchanged: middle control while a session is running, paused, or timed-paused, and the native menu when inactive.
- **Competing input and deadline:** Coordinator reconciles a due deadline before applying the command, using `now >= deadline`; UI must not independently resolve races.
- **Day boundary:** `[ASSUMPTION]` local watch calendar date defines the tally day; behavior at midnight follows this boundary unless revised.

## Accessibility Floor

- Target-device acceptance: verify countdown, phase identity, `PAUSED`/`BREAK` states, daily totals, button labels, and errors remain legible on Pebble Time 2; use the native high-contrast presentation and do not rely on color alone.
- Ensure `Sit`, `Stand`, and `Move`, active/manual/timed pause status, remaining time, and daily totals are textually identifiable; icons are supplementary.
- Use native Pebble text sizing and layout behavior. Exact accessibility features and scaling limits are [ASSUMPTION] platform-defined and require target-device verification.
- Hardware actions and menu commands must remain distinguishable by their function; never communicate +5/+10 or pause state by color only.
- Surface clock, persistence, and wakeup/scheduling errors in visible text.

## Key Flows

`[ASSUMPTION]` A named persona is not supplied; use “the owner” rather than inventing person identity.

### Flow 1 — Start and follow a session (PRD UJ-1)

1. The owner selects Start from the watch menu; valid UTC begins a new Sit phase.
2. The session view presents the agreed time, countdown, phase, and daily tally hierarchy.
3. The owner leaves the app; the watch continues timing without phone interaction.
4. The owner adjusts or pauses the phase using the controls defined in Interaction Primitives.
5. **Climax:** At the phase deadline, the watch requests one haptic and foregrounds the new phase if Pebble firmware supports it.

Failure path: invalid UTC leaves the session inactive with a visible error. Persistence or wakeup scheduling failure is surfaced. Overdue recovery and platform limitations follow Interaction Primitives and State Patterns.

### Flow 2 — Take and end a timed break (PRD UJ-2)

1. The owner presses the middle control during a running phase; the 30-minute timed break starts.
2. The owner can select another duration using the displayed top/bottom labels, or press middle to resume early.
3. **Climax:** At break end, the same phase resumes with its saved remaining time, without cycle advancement.

Failure path: wakeup or persistence failure is surfaced and must not be presented as a reliably scheduled break. Manual Pause remains distinct: it has no auto-resume deadline and waits for explicit Resume.

### Flow 3 — Skip a phase and check today's tally

1. The owner selects Skip from the native menu during a running phase.
2. The next phase begins at its default duration and the owner returns to the session view.
3. **Climax:** The new phase is clear, and the skipped phase has not increased today's tally.

Failure path: if Skip cannot be committed or its Wakeup cannot be scheduled, show a visible error and retain the last committed state.

## Open Items and Assumptions

- `[ASSUMPTION]` No named persona was supplied; journeys use “the owner.”
- Native menu command order is Start, Settings when inactive; Pause/Resume, Skip, Stop, Settings while active. Physical navigation mechanics remain native Pebble behavior.
- Timed-pause alternatives map by duration: shorter on top and longer on bottom; a new 30-minute break shows 5 on top and 10 on bottom.
- Today's tally uses the watch's local calendar day, persists locally across app restarts, aggregates across sessions, and resets at local midnight.
- Use monochrome phase icons for a chair (Sit), standing person (Stand), and moving person (Move), always paired with readable text; verify suitable native assets and legibility on Pebble Time 2.
- Exact native Pebble palette values, text metrics, and layout constraints are platform-defined; verify on Pebble Time 2.
- Phase-deadline/timed-pause Wakeup delivery, haptic invocation, and app foregrounding after leaving the app require target-device verification; none is guaranteed here.
