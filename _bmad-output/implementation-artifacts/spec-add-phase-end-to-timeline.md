---
title: 'Add the current phase end to the Pebble Timeline'
type: 'feature'
created: '2026-11-04'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
context: []
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The end of the active Sit, Stand, or Move phase is not represented in Pebble's native Timeline.

**Approach:** Publish a Timeline pin for the current phase's expected end. Keep the coordinator's UTC deadline as the sole timing authority and keep the Timeline pin synchronized as the session changes.

**Decisions (owner, 2026-11-04):** Keep the feature watch-only; the pin, if feasible, should be informational and must not alert. Defer the Pebble Timeline pin rather than add a phone/network dependency.

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c` -- sole owner of phase deadlines and all session transitions, including Start, Pause/Resume, timed break, Skip, Extend, recovery, and phase completion.
- `src/c/session_coordinator.h` -- `SessionView` exposes the authoritative record and derived remaining time; currently has no UI/phone event contract.
- `src/c/sit_stand_move.c` -- native watch UI; renders coordinator state but has no Timeline publishing responsibilities.
- `src/pkjs/` -- absent. `wscript` includes this path in the Pebble JS bundle glob, but no phone-side integration currently exists.
- `package.json` -- Pebble app manifest has only a placeholder `dummy` AppMessage key and declares no Timeline integration.
- `_bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` -- AD-1/AD-2 require coordinator ownership and UTC deadlines; AD-5 excludes a required phone runtime; AD-14 specifies haptics as the only notification channel.
- `_bmad-output/planning-artifacts/ux-designs/ux-sit_stand_go-2026-10-23/EXPERIENCE.md` -- current watch surfaces and behaviors; it defines no Timeline content or publication lifecycle.

## Implementation Notes

Investigation confirms this request adds a Pebble Timeline pin, not a watch-screen label. Pebble Timeline publishing requires phone-side PebbleKit JS and the Timeline service; the repository has no JS/AppMessage bridge and the selected watch-only constraint rules out the required mechanism. Owner chose to defer the pin rather than add phone/network dependence. No application code was changed.
