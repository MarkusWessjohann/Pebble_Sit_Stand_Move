---
title: 'Publish current phase end through a phone Timeline bridge'
type: 'feature'
created: '2026-11-06'
status: 'done'
route: 'dispatch'
review_loop_iteration: 0
baseline_commit: '3d196f2f7d34152d73d50c785bdc5f19c3b23bff'
context:
  - _bmad-output/implementation-artifacts/spec-add-phase-end-to-timeline.md
  - _bmad-output/implementation-artifacts/epic-1-context.md
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The end of the active Sit/Stand/Move phase is not available from Pebble Timeline.

**Approach:** Add a PebbleKit JS phone-side bridge that receives authoritative session snapshots from the watch and publishes one informational Pebble Timeline pin for the current phase's expected end. This supersedes the earlier decision to defer the pin: the owner now asks for the phone-side bridge, while retaining watch-owned session control and a non-alerting pin.

**Decision (owner, 2026-11-06):** The phone bridge is optional. Missing phone, pairing, or network must never block watch-owned session timing or controls. The pin is informational and must not alert.

**Decision (owner, 2026-11-06):** Use a concise pin title in the form `Sit ends at 10:45`, with the phase name and local end clock time together in the title.

**Decision (owner, 2026-11-06):** The bridge reads a user-configured Timeline API token from phone-side configuration. Do not commit real credentials.

## Boundaries & Constraints

**Always:** Keep the Session Coordinator and UTC deadline authoritative. The phone bridge only mirrors state, never commands or times the session. Maintain one stable informational pin for the current session/phase; update it after committed changes, extensions, recovery, and app launch. For `running`, pin time is the committed phase deadline. For `timed_paused`, pin time is the predicted phase end after the break: pause-end deadline plus saved phase remainder. Remove the pin for `inactive` and indefinite `paused` states, and when Stop commits. A missing phone, messaging failure, or Timeline service failure must not block watch behavior; send/reconcile the latest snapshot when the phone bridge reconnects, never queue stale deadlines. Do not configure alerts/reminders on the pin. Pin times are UTC instants rendered in the user's local time by Pebble Timeline.

**Never:** Move lifecycle or deadline ownership to JavaScript, require phone connectivity for watch sessions, publish a second pin per event, or add an alerting notification channel.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Running snapshot | Committed running phase and deadline | Create or update the stable pin with phase name and deadline | Keep watch session unchanged; retry latest snapshot on reconnection |
| Deadline mutation | Extend, Skip, transition, Resume, or recovery commits | Update the same pin to the newly authoritative phase end | A stale snapshot must not replace a newer generation |
| Timed break | Timed pause with pause deadline and saved phase remainder | Pin the predicted resumed phase end at pause deadline + saved remainder | No pin alert; watch pause behavior unchanged |
| Inactive/manual pause | Stop, inactive, or indefinite paused state | Delete the active session pin | Idempotently tolerate an already-absent pin |
| Offline/failed publish | AppMessage or Timeline request unavailable/fails | Watch keeps operating; next valid snapshot reconciles the single pin | Retry latest state only; do not queue stale deadlines |
| Out-of-order message | A snapshot has an older coordinator generation | Leave the newer pin unchanged | Ignore stale snapshot |

</frozen-after-approval>

## Code Map

- `src/c/session_coordinator.c` -- sole owner of committed session mutations and deadline/recovery decisions; expose or publish a compact outbound state snapshot only after state is committed and after initialization/recovery.
- `src/c/session_coordinator.h` / `src/c/session_store.h` -- current `SessionView` includes the persisted record and derived remaining seconds; serialize lifecycle, phase, generation, deadline fields, and pause remainder to the phone.
- `src/c/sit_stand_move.c` -- app initialization/deinitialization currently manages the coordinator and UI; initialize AppMessage and retry the latest snapshot when its outbox becomes available.
- `src/pkjs/` and `src/common/` -- absent; `wscript` already bundles these globs with entry point `src/pkjs/index.js`.
- `package.json` -- currently declares only placeholder AppMessage key `dummy`; replace it with stable snapshot field keys.
- `tests/pebble-stubs/pebble.h`, `tests/session_coordinator_test.c`, and `scripts/test.sh` -- C API stubs and coordinator test runner; extend to verify snapshot generation at commit boundaries.
- Add Node built-in tests for the phone bridge with mocked Pebble events and Timeline requests; do not require a live account or network for host tests.

## Tasks & Acceptance

**Execution:**
- [x] Add a coordinator-owned AppMessage snapshot contract and publish only committed, current session state.
- [x] Add `src/pkjs/index.js` to parse snapshots, reject older generations, and idempotently upsert/delete one informational Timeline pin.
- [x] Replace manifest placeholder message keys with the snapshot schema and test phone-side pin behavior using mocked events/requests.
- [x] Document phone pairing, connectivity limits, and how the bridge behaves independently of watch session control.
- [x] Replace the prior Timeline deferral entry with this active implementation record once the feature is complete.

**Acceptance Criteria:**
- Given a committed running session, when its snapshot reaches the phone bridge, then one informational pin shows the active phase and committed UTC end time.
- Given the pin is displayed, when its content is rendered, then it names the current phase and its expected local clock-time end, with no alert/reminder configured.
- Given a phase deadline changes or a new phase is committed, when its newer snapshot arrives, then the same pin is updated and an older snapshot cannot roll it back.
- Given a timed break, when its snapshot is published, then pin time is pause-end plus saved phase remainder and the pin does not alert.
- Given Stop, inactive, or manual pause, when the snapshot arrives, then the session pin is deleted because these states have no running phase end.
- Given the phone or Timeline service is unavailable, when publication fails, then watch controls/timing are unaffected and the latest current snapshot reconciles the pin when connectivity returns.

## Implementation Notes

- The watch publishes version 1 snapshots after coordinator recovery/initialization and after each coordinator queue drain. Its AppMessage retry retains only the newest committed record.
- The phone stores the newest snapshot and generation locally, PUTs/DELETEs the stable pin idempotently, ignores older generations, and retries the latest state. Configure a private token in `src/pkjs/config.json`; the checked-in token is blank.
- The earlier watch-only deferral is superseded by this owner-approved optional phone bridge; see [the active phone Timeline bridge spec](spec-create-phone-timeline-bridge.md).
- Follow-up verification fixes: validate phone payload fields and restored storage, ignore equal/older generations, timeout stalled Timeline requests, and request the latest watch snapshot when the phone bridge becomes ready. Eleven Node tests cover bridge behavior, retries, and the AppMessage contract; host suite and syntax/JSON/diff checks pass.
- No review findings were deferred after fixes. Live Timeline service/device integration remains unverified here; SDK 4.33.1 is not active and no paired device/token is configured.

## Review Triage Log

- medium (patch, fixed) -- Phone parser accepted unknown lifecycle/phase and invalid active deadlines, which could delete a valid pin or preserve stale content; validate snapshots before accepting and validate restored storage.
- medium (patch, fixed) -- XHR without a terminal event left `requestInFlight` true indefinitely and blocked newer pin state; add a 30-second timeout and callback-once guard.
- medium (patch, fixed) -- Phone reconnect had no request/response path to refresh current watch state; on phone `ready`, request a fresh snapshot from the watch.
- medium (patch, fixed) -- Watch serializer and JS parser used parallel numeric key contracts with no automated consistency check; add a contract test across the manifest, sender constants, and parser fields.
- false -- Equal generations can replace the bridge's snapshot; the handler rejects `generation <= latestGeneration` after first accepted snapshot, so equal-generation content cannot roll back state.
- false -- Break pins should use phase deadline directly; the approved contract intentionally uses `pauseDeadlineUtc + remainingSeconds` to show predicted end after timed pause.

</frozen-after-approval>
