---
title: Sit Stand Go — Pebble Reminder App
status: draft
created: 2026-10-21
updated: 2026-10-31
---

# Product Brief: Sit Stand Go

## Executive Summary

Sit Stand Go is a simple, watch-first Pebble Time 2 app for people who want a nudge to break up long periods of sitting. During a manually started session, it runs a repeating 60-minute rhythm: sit for 45 minutes, stand for 10 minutes, then walk or move for 5 minutes. At each on-time phase transition, the watch vibrates and opens the app to show the active phase and remaining time; Pebble Time 2 foreground behavior remains to be verified on-device.

The app is intended to continue timing after the user leaves it, without phone interaction during a session. Users can manually pause/resume, pause the active phase for 5, 10, or 30 minutes, skip a phase, or extend it by 5 or 10 minutes. During timed pause the phase clock is frozen; at pause end the same phase resumes with saved remaining duration, the app opens, and the watch vibrates once. If a deadline is missed during downtime, elapsed phases are processed in order; the resulting phase restarts with its full default duration and produces one haptic. Pin and verify a reproducible toolchain/build workflow before implementation; verify physical Wakeup and foreground behavior with the finished app before release in Story 1.8.

## The Problem

When focused on work or other activities, a person may sit for long stretches and lose track of time. A phone reminder can be easy to miss or can require reaching for another device. The user wants a wrist-based prompt to change posture and move, while retaining control when a phase does not fit the moment.

## The Solution

Provide a Pebble Time 2 app that starts a repeating sit–stand–move session on demand. Each cycle has a default schedule of 45 minutes sitting, 10 minutes standing, and 5 minutes walking or moving. At a phase deadline, the watch emits a haptic and opens the app to show the current phase and remaining time. The user can pause, resume, stop, skip to the next phase, or extend the current phase by 5 or 10 minutes.

The intended experience is watch-first and does not depend on a phone during a session. Pebble Time 2 firmware must be tested for Wakeup haptics and app foregrounding with the finished app before release in Story 1.8.

## What Makes This Different

The product is deliberately small: a repeating movement rhythm and a few controls directly on the watch, without requiring phone interaction during use. Its value depends on dependable reminders and low-friction control, not on a novel technology or an unverified market advantage.

## Who This Serves

The primary user is the project owner, who wants a personal reminder while working or otherwise sitting for extended periods. Success means being prompted at the intended transitions and being able to adapt or end a session quickly from the watch.

## Success Criteria

- A user can start, manually pause/resume, and stop a session from the watch.
- A 5-, 10-, or 30-minute timed pause freezes the current phase; its end opens the app, shows that phase with saved remaining time, and requests one haptic without advancing the cycle. If the user resumes early, the stale pause Wakeup neither reopens the app nor requests a haptic.
- In a running session, the 45/10/5-minute phases advance; on an automatic deadline the watch emits a haptic and opens the app to show the new phase and remaining time, on verified Pebble Time 2 firmware.
- A user can skip the current phase or extend it by +5 or +10 minutes without disrupting the session.
- The user can run a session without interacting with a paired phone.
- If any deadline is overdue on recovery, elapsed phases are processed in order, the resulting current phase restarts at full default duration, and no more than one haptic is emitted.
- UTC epoch remains authoritative when device time changes: forward jumps can make a phase due; backward jumps defer it.

These criteria describe intended outcomes; exact platform capabilities and test conditions need to be established during requirements and technical planning.

## Scope

**First version:** A watch-first Pebble Time 2 app; manually started/stopped sessions; repeating 45-minute Sit, 10-minute Stand, and 5-minute Move phases; haptic plus app UI on automatic phase deadlines; manual pause/resume; timed pause for 5/10/30 minutes that resumes the same interrupted phase; skip; +5/+10-minute extensions; and deterministic missed-deadline recovery.

**Out of scope:** Activity sensing, health claims, history or adherence analytics, social features, cloud services, extensive schedule management, and a required phone companion.

## Vision

If the personal version proves useful, Sit Stand Go can remain a lightweight watch utility that makes regular posture changes easier to remember and accommodates real-world interruptions without turning movement into another tracking project.

## Open Questions and Assumptions

- **Assumption:** Sessions start and stop manually; the user does not need an overnight schedule.
- **Assumption:** Manual pause/resume is available; phase change uses haptic with app UI display.
- **Decision:** Timed Pause intervals are 5, 10, or 30 minutes. The phase clock freezes and the same phase resumes at pause end with its remaining duration, app open, and one haptic request. Manual early Resume cancels the Wakeup; stale callbacks do not reopen or request a haptic.
- **Decision:** Skip advances immediately; extensions add +5 or +10 minutes to the active phase.
- **Decision:** Pebble Time 2 is the target model.
- **Decision:** The timer continues when the user leaves the app, but need not continue after explicit termination.
- **Decision:** On any recovery after a deadline is due, advance elapsed phases in order, restart the resulting current phase with full default duration, and emit only one haptic.
- **Decision:** UTC epoch remains authoritative when device clock changes; forward adjustment can make a phase due, backward adjustment delays it.
- **Decision:** Skip emits one haptic; timed-pause end requests one; Start/manual Pause/Resume/Stop/Extend do not. If UTC time is invalid, a new Session stays inactive with a visible time error; an active Session keeps its saved state until valid time is restored.
- **Open:** Measurable timing tolerance on Pebble Time 2, to be agreed before Story 1.8 pre-release acceptance; invalid-clock handling is fixed as an inactive Session with visible error.
- **Open:** A separate Pebble system alert for missed wakeups is disabled by default; reconsider only if target testing shows user value.
- **Open:** If Pebble Time 2 cannot foreground the finished app or request a haptic at timed-pause end, revisit whether timed pause belongs in v1 before release.
