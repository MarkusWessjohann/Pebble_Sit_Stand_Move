# Independent Technology Verification — Architecture Spine

**Subject:** `ARCHITECTURE-SPINE.md`  
**Review date:** 2026-10-21  
**Scope:** Current web-documentation check of platform/SDK identification, Wakeup API, storage, lifecycle and recovery semantics, notification/vibration capability, and build-toolchain existence/fit. The spine was not edited.

## Verdict

**Not ready to treat as target-validated technology.** The core architecture is plausible for the documented Pebble-family APIs: a single persisted deadline plus one scheduled wakeup fits the API limits, and callbacks can launch an app that is not running. The reviewed sources do **not** verify that the specific Time 2 firmware supports this behavior end-to-end, nor do they establish that Rebble SDK 4 is the current/reproducible toolchain for that target. Several operational descriptions need tighter wording: a wakeup launches the app rather than acting as a persistent background worker; missed-power-off notification is distinct from app recovery; and vibration is not a text notification.

## Findings

### 1. SDK 4 exists and is documented; its fit as the selected current Time 2 build toolchain is unsupported

**Spine locations:** Stack (lines 92–93); Deferred (lines 133, 138).

Rebble’s SDK 4 page and release notes establish that an SDK 4 release and C APIs are documented. SDK 4.0 dates to 2016, and the changelog describes the original Pebble 2 platform addition; it does not claim compatibility with the later Time 2 platform. The live developer install page now documents a `pebble-tool` CLI, installing the latest SDK, building projects, emulator use, and phone-mediated install. It recommends Python 3.13 for CLI v5.0.18. So a build toolchain exists; the spine’s claim that no existing toolchain was present at architecture start may describe its project-state snapshot, but must not be read as a current ecosystem fact. Current hardware/platform documentation identifies Time 2 as platform Emery, while API/platform docs include Emery and the build-for-every-Pebble guide lists `PBL_PLATFORM_EMERY`. This establishes platform awareness in the docs, **not** that a particular SDK package/compiler release builds, installs, or runs correctly on the user’s target firmware.

**Necessary caveat:** SDK 4 “documented by Rebble” is not a version pin or evidence of target compatibility. Name the actual CLI and SDK package versions/platform target to use, then compile, install, and execute on the exact Time 2 hardware/firmware. Use emulator results only for what the documented emulator target supports; the install guide’s example is Pebble Time/Basalt.

Sources (accessed 2026-10-21):
- [SDK 4 overview](https://developer.rebble.io/sdk4/)
- [SDK 4.0 changelog (release date and platform changes)](https://developer.rebble.io/sdk/changelogs/4.0/)
- [Current SDK installation and CLI instructions](https://developer.rebble.io/sdk)
- [Hardware/platform comparison](https://developer.rebble.io/guides/tools-and-resources/hardware-information/)
- [C API platform identifiers](https://developer.rebble.io/docs/c/Foundation/Platform/)
- [Platform-specific compile defines, including Emery](https://developer.rebble.io/guides/best-practices/building-for-every-pebble/)
- [Command-line tool capabilities and emulator examples](https://developer.rebble.io/guides/tools-and-resources/pebble-tool/)

### 2. Wakeup limit claims are supported; scheduling errors and minute-window semantics must be handled explicitly

**Spine locations:** AD-3 (lines 37–43); Wakeup identity and error conventions (lines 83–86); Deferred wakeup constraints (line 134).

The current C API documents `wakeup_schedule(timestamp, cookie, notify_if_missed)`: the timestamp is UTC; an app may have at most eight scheduled events; another event may not be scheduled within one minute of a currently scheduled event. It returns an error for a conflicting window, a past timestamp, exhausted slots, or system error. `wakeup_cancel`, `wakeup_cancel_all`, `wakeup_query`, and callback cookies/IDs are documented. Thus, “maximum of eight” and “one-minute exclusion window” are supported. Scheduling only the next 5/10/45-minute phase deadline is within the documented spacing limit, assuming the requested time remains in the future and no other event conflicts.

The one-minute wording is a **spacing/exclusion constraint**, not a guarantee of exact callback timing or a precision/tolerance statement. The API’s own failure results matter to the architecture: persisting a new deadline does not make a wakeup successfully scheduled. Also account for a stale/already-fired `WakeupId` and check scheduling/query results rather than equating a successful state write with a pending event.

**Necessary caveat:** Keep the spine’s target-firmware verification gate. Treat every schedule result as authoritative, record a recoverable state when it fails, and do not claim background reminders are guaranteed after a failed schedule. Validate behavior with the actual firmware and any platform-specific limits.

Sources (accessed 2026-10-21):
- [Wakeup C API](https://developer.rebble.io/docs/c/Foundation/Wakeup/)
- [Persistent-storage status codes](https://developer.rebble.io/docs/c/Foundation/Storage/)

### 3. Wakeup supports launch while the app is not running; it is not a continuously running background worker

**Spine locations:** Design Paradigm (line 21); AD-3 and operational boundary (lines 37–43); AD-4 (lines 45–49); Deferred power-off verification (line 132).

The Wakeup API says it can schedule an app to be launched even when it is not running. Its callback is subscribed by the app, and the launch-reason API identifies `APP_LAUNCH_WAKEUP`; this supports the design’s event-driven model and recovery-on-launch. It does not document an app process remaining alive in the background between deadlines, nor does it promise that leaving the foreground is identical across firmware/exit conditions. In the app model, a wakeup causes the app to launch so code can handle the event; it is not evidence of a resident worker. The distinction between focus loss/coverage and app termination is also visible in the AppFocus API, which describes focus changes such as system windows covering an app, rather than a general background-execution guarantee.

**Necessary caveat:** Define “leave the app” as concrete test cases (e.g. app covered, user exits/dismisses it, app no longer running). State that the design relies on a scheduled launch and callback, not on uninterrupted execution. Verify whether and how wakeup launches present UI or deliver a visible/haptic alert on the target; do not infer either from callback support alone.

Sources (accessed 2026-10-21):
- [Wakeup C API](https://developer.rebble.io/docs/c/Foundation/Wakeup/)
- [Launch-reason API](https://developer.rebble.io/docs/c/Foundation/Launch_Reason/)
- [App event loop](https://developer.rebble.io/docs/c/Foundation/App/)
- [AppFocusService semantics](https://developer.rebble.io/docs/c/Foundation/Event_Service/AppFocusService/)

### 4. Power-off/missed-wakeup alert is not proof of automatic state reconciliation or app callback

**Spine locations:** AD-4 (lines 45–49); Deferred target verification (line 132); Wakeup API behavior (line 134).

`notify_if_missed` is documented as asking the system to alert the user on power-on when notifications were missed because the watch was off. Separately, `wakeup_get_launch_event` reports wakeup launch information, and `launch_reason()` identifies a wakeup launch. The public API does not say that the `notify_if_missed` alert invokes the app’s callback, reconstructs session state, walks every missed phase, or guarantees that the app is automatically launched for recovery. The spine’s own reconcile-on-launch rule is therefore a sensible app policy, but the power-on behavior remains target-dependent. Likewise, epoch deadlines support reconciliation only if the watch’s clock has a valid, sufficiently accurate time after startup.

**Necessary caveat:** Keep the explicit power-off/reboot test gate. Separately verify (a) whether the built-in missed-event alert appears, (b) whether the app is launched, (c) when persisted state can be read/reconciled, and (d) what happens when the device clock is unset or changes. Do not describe the built-in alert as the app’s one current-phase notification.

Sources (accessed 2026-10-21):
- [Wakeup C API, including `notify_if_missed` and launch-event lookup](https://developer.rebble.io/docs/c/Foundation/Wakeup/)
- [Launch-reason API](https://developer.rebble.io/docs/c/Foundation/Launch_Reason/)
- [Wall-time/timezone helpers](https://developer.rebble.io/docs/c/Foundation/Wall_Time/)

### 5. Persistent storage is suitable for a compact session record, but has strict limits and does not make writes/scheduling atomic

**Spine locations:** Persistence convention (line 83); error convention (line 86); exact record deferred (line 135); AD-2 UTC deadlines (line 35).

The C Persistent Storage API provides app-specific key/value storage for state. Its documentation sets a 256-byte maximum for one string or byte-array value and a total limit of 4 KB per app. Writes return status codes, including storage/resource errors. A compact session record containing lifecycle, phase, deadline, generation, and any needed pause remainder fits comfortably in principle. However, SDK docs do not claim a transaction spanning a record write plus wakeup schedule/cancel, nor that a multi-key sequence is atomic. The spine’s order to persist before scheduling is a convention, not a cross-API atomic guarantee. A crash or wakeup-scheduling error can leave the persisted record and pending wakeup temporarily inconsistent; generation checks/reconciliation are important precisely for that case.

UTC epoch deadlines align with `wakeup_schedule`’s UTC timestamp contract. The documentation does not establish that the target’s wall clock is always synchronized or monotonic; DST/time-zone resilience does not by itself address manual/system clock jumps.

**Necessary caveat:** Keep the record small and preferably single-value; check every persistence return code; document recovery for interruption between persistence and wakeup replacement. Treat clock-jump policy and target clock behavior as unverified runtime requirements.

Sources (accessed 2026-10-21):
- [Persistent Storage C API and limits](https://developer.rebble.io/docs/c/Foundation/Storage/)
- [Wakeup C API timestamp contract](https://developer.rebble.io/docs/c/Foundation/Wakeup/)
- [Wall-time API](https://developer.rebble.io/docs/c/Foundation/Wall_Time/)

### 6. Vibration is supported, but it is not equivalent to a readable phase notification

**Spine locations:** AD-7 (lines 63–67); capability map and diagram (lines 97–104, 124).

The C Vibes API supports short, long, double, and custom vibration patterns. It documents vibration-motor control, not a general app-authored text notification API that can display “active phase and duration” while the app is not open. The Wakeup API’s `notify_if_missed` is a distinct system alert on power-on. Consequently, AD-7’s readable-content requirement and the “vibration as initial notification mode” wording combine two different delivery mechanisms. A wakeup callback can run app code and can request vibration, but documentation alone does not establish that readable app UI is displayed to the user or that a haptic is delivered under all target conditions.

**Necessary caveat:** Specify the intended on-time signal precisely: haptic-only on wakeup, app UI when the user opens/when launch presents it, or another verified system mechanism. Keep any claim of visible phase text or reliable vibration conditional on target-device testing. Avoid calling vibration itself a notification payload.

Sources (accessed 2026-10-21):
- [Vibes C API](https://developer.rebble.io/docs/c/User_Interface/Vibes/)
- [Wakeup C API](https://developer.rebble.io/docs/c/Foundation/Wakeup/)
- [Current SDK documentation index](https://developer.rebble.io/docs/)

## Claims adequately supported by documentation

- SDK 4 is a real, documented Rebble/Pebble C SDK generation; its release lineage is historical (2016), and documentation availability alone says nothing about current target support.
- The SDK exposes a native C API for watchapps, app-specific persistent storage, scheduled wakeups, launch reasons, and vibration.
- Wakeup events may launch an app that is not running; up to eight may be scheduled per app, with the documented one-minute exclusion window.
- A compact authoritative session record is compatible in size with documented persistent-storage limits.
- The current developer site documents an installable CLI/latest-SDK build flow. Exact pinning and Time 2 runtime compatibility remain open.

## Bottom line

Keep AD-3 and the firmware/toolchain verification gates as real handoff gates. Before claiming the technology stack is verified, pin a reproducible CLI/SDK/platform build, prove install and wakeup behavior on the exact Time 2 firmware, and separately test foreground exit, stopped-app launch, power-off recovery, and user-visible/haptic signaling. The cited web docs support the API shape and stated limits, but not those device-level outcomes.
