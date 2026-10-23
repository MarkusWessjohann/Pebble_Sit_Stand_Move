# Technology Verification — Architecture Spine

**Subject:** `ARCHITECTURE-SPINE.md`  
**Re-evaluated:** 2026-10-23 against the current spine, PRD, and brief and the public-documentation findings recorded in the 2026-10-21 technology check. This refresh does not represent a new physical-device test or a fresh live-documentation check.  
**Scope:** Pebble/Rebble API contracts, SDK/toolchain pin, Wakeup identity and limits, storage, haptic/UI behavior, and the target verification gate.
**Change boundary:** The architecture spine was not edited.

## Verdict

The current spine now reflects the documented API contracts and closes the earlier Wakeup-ID and missed-wakeup-alert ambiguities. It calls `wakeup_cancel_all()` on every app launch, persists and queries the current `WakeupId`, uses the generation cookie and callback guards, sets `notify_if_missed = false`, defines transaction/recovery and explicit-retry behavior, and distinguishes app haptic attempts from readable UI. General API documentation still does not establish Pebble Time 2 firmware behavior: Wakeup-after-leaving-app, haptic delivery, and foreground/UI presentation remain physically unverified. The exact SDK package is also not pinned; `latest` is not a reproducible version, and target compatibility has not been demonstrated. Firmware and SDK verification remain an unpassed gate.

## Current contracts and remaining verification

### 1. Wakeup identity, query, and recovery — contract resolved

AD-9/AD-11 persist the current `WakeupId`, query it with `wakeup_query(id)`, bind the mutation generation to the cookie, reject callbacks unless ID/cookie/state/deadline match, and retain/re-arm an early matching callback. Every app launch first calls `wakeup_cancel_all()` to remove orphans/duplicates, then repairs the last committed record and re-schedules the one intended Wakeup for a running session. The API check recorded in the prior report documents IDs in schedule results/callbacks and query by ID, not a list-by-cookie operation; the spine correctly avoids relying on such enumeration.

AD-11 also defines candidate schedule/commit/cancel ordering, launch cleanup, and explicit user-command retry after a failed proposal. The Deferred gate specifies injected schedule, persistence-write, cancellation, and launch-repair failures plus crashes at transaction boundaries, with persisted-state, callback, cleanup/retry, UI, and haptic-attempt assertions. `notify_if_missed = false` is explicit: application recovery owns phase reconciliation and its single current-phase haptic, rather than relying on the separate system missed-wakeup alert. These are documented architecture/test contracts; their operational execution and target firmware behavior remain unverified.

### 2. SDK/toolchain and Pebble Time 2 compatibility — unresolved

The prior documentation check records Rebble guidance for CLI 5.0.18 with Python 3.13 and installation via `pebble sdk install latest`. `latest` is a moving selector, not an exact SDK pin. Neither that guidance nor the documented emulator example establishes a reproducible SDK package or compatibility with the specific Pebble Time 2 firmware.

**Required evidence:** Resolve and record exact CLI/SDK package and build identifiers, then build/install and run on the target. Exact SDK pinning and target compatibility remain an unpassed gate; no successful target build or run is claimed here.

### 3. Haptic, app launch, and foreground behavior — physically unverified

The prior API review supports general Wakeup app launch and vibration APIs, but does not establish that the target Time 2 firmware will deliver a haptic on scheduled Wakeup or foreground/present the app UI after the user leaves it. A haptic is not a readable text notification; phase and remaining time are rendered by the app UI when presented. PRD §5.1 and AD-3/AD-7/AD-14 correctly retain these as target checks.

**Required evidence:** On the physical target, separately observe leave-app Wakeup execution, haptic delivery, and app foreground/UI presentation. Do not describe any as validated until observed.

### 4. Clock and measurable timing tolerance

The spine and PRD specify UTC epoch wall time as authoritative through device-clock changes: forward corrections can make deadlines due, and backward corrections defer them. Start with invalid UTC is rejected and leaves the session inactive with a visible clock-time error. If UTC becomes invalid during a running session, the session state is retained and scheduling/haptics are suppressed until valid UTC returns, then the deadline is reconciled. This resolves the clock-policy ambiguity; invalid-clock behavior is not an open wording decision. A measurable transition tolerance on verified firmware remains open and is not established by API documentation.

## Documentation-supported API facts carried forward

The 2026-10-21 source check recorded these general documentation claims; they do not prove target-firmware behavior:

- Wakeup scheduling uses a UTC timestamp and accepts a cookie and `notify_if_missed` flag; the documented API limits an app to eight scheduled events and applies a one-minute exclusion window. The API returns a `WakeupId`, callbacks include ID/cookie, and `wakeup_query(id)` checks a specific ID.
- Persistent storage documents a 256-byte maximum for one string/blob and 4 KB total per app; writes report success/failure rather than providing an atomic transaction with Wakeup scheduling.
- The vibration API provides haptic patterns, not a readable text-payload notification. General Wakeup launch support does not guarantee foreground presentation on Pebble Time 2.
- Rebble guidance documents CLI 5.0.18/Python 3.13 and `latest` SDK installation; it does not make `latest` an exact pin.

## Bottom line

The spine’s current launch cleanup, Wakeup ID/query, generation/retry, `notify_if_missed = false`, UTC/invalid-clock, callback, failure, and haptic-attempt contracts are recognized as resolved architecture decisions. Keep the pre-implementation gate for exact SDK pin/build and physical Pebble Time 2 Wakeup, haptic, and foreground verification; measurable timing tolerance also remains open. No physical validation is asserted by this report.
