# Adversarial divergence review

Scope: concrete choices two independently implemented units could make while still obeying the spine. Each fix is a minimal contract to add as an AD or consistency convention.

## Findings

### 1. Authoritative session record has no shared shape

- **Location:** AD-1; Consistency Conventions — Persistence; Deferred — exact session-record fields
- **Trigger condition:** One implementation stores `running` plus a phase enum and absolute deadline; another stores an `active` flag, phase index, and remaining seconds. Both keep one authoritative record and derive display time from it, but coordinator, storage, and recovery code cannot exchange or interpret the record consistently.
- **Guard snippet:** Define the canonical record fields and types: schema version, lifecycle enum, session ID, stable phase ID, UTC deadline when running or remaining duration when paused, and transition generation. Specify inactive/paused field validity and migration or invalid-record behavior.
- **Potential consequence:** Independently written coordinator, persistence, and recovery units disagree on which value is authoritative or reject each other's stored state.

### 2. Missed-deadline reconciliation can preserve cadence or rebase it

- **Location:** AD-4; AD-2; Deferred — timer tolerance
- **Trigger condition:** After a delayed wakeup, one implementation advances each deadline from the prior deadline, preserving the original Sit/Stand/Move cadence; another sets the reconciled phase deadline to recovery time plus its default duration. Both advance through elapsed phases in order, but subsequent phase boundaries diverge. The exact-deadline comparison (`now == deadline`) is also unspecified.
- **Guard snippet:** Specify whether catch-up advances from each expired deadline or rebases from recovery time, and whether a deadline is elapsed at `now >= deadline`; define any clock-tolerance rule.
- **Potential consequence:** The same persisted session and wakeup time lead to different active phases or next reminder times.

### 3. Simultaneous commands and wakeups lack a deterministic serialization rule

- **Location:** AD-1; Design Paradigm
- **Trigger condition:** A phase deadline and user Skip arrive together. One event loop processes Skip first; another reconciles the expired phase first. Both serialize all inputs through the coordinator, yet produce different resulting phases and notification opportunities.
- **Guard snippet:** Require one FIFO coordinator queue and define the tie rule; for example, reconcile any due deadline before applying a user command, using the event's captured UTC time.
- **Potential consequence:** Identical user actions at the same deadline yield different state transitions across implementations.

### 4. “Generation” does not define a wakeup identifier contract

- **Location:** Consistency Conventions — Wakeup identity
- **Trigger condition:** One implementation uses a session ID plus a monotonically increasing mutation generation as the Pebble wakeup cookie; another uses the deadline epoch or an independent alarm ID. The convention requires association with session/deadline generation but does not define fields, uniqueness, increment points, or cookie encoding.
- **Guard snippet:** Define the wakeup payload/identifier and its comparison rule; specify generation scope and that every deadline-changing mutation, including pause/resume, skip, extend, and restart, invalidates the prior generation.
- **Potential consequence:** The wakeup adapter and coordinator disagree about which alarm is current, allowing a stale callback to be accepted or the current callback to be discarded.

### 5. Callback acceptance is underspecified beyond matching persisted state

- **Location:** Consistency Conventions — Wakeup identity; AD-3
- **Trigger condition:** A callback arrives early, late, or after a pause/resume cycle. One handler requires the callback identifier to match and `now >= deadline` before reconciling; another accepts any callback whose stored deadline matches, or ignores early callbacks without re-arming. Both claim to ignore callbacks that do not match the persisted active session and deadline.
- **Guard snippet:** Accept a callback only when lifecycle is running and its session ID, generation, and deadline match the persisted record; if it fires before the deadline, retain/re-arm the current wakeup; otherwise run the common reconciliation path.
- **Potential consequence:** Early or stale callbacks can cause premature transitions, lost alarms, or inconsistent recovery.

### 6. Persist-then-schedule leaves a crash window without a recovery protocol

- **Location:** AD-3; Consistency Conventions — Persistence
- **Trigger condition:** The app persists a new deadline and crashes before replacing the old wakeup. One implementation re-arms the intended wakeup on every launch; another assumes the scheduler operation completed. Both persist before scheduling and ignore stale callbacks.
- **Guard snippet:** Require startup/recovery to idempotently compare the persisted running record with pending wakeups and install exactly its intended wakeup before presenting the session as reliably scheduled; define the behavior when re-arming fails.
- **Potential consequence:** A valid persisted running session can remain without a future callback, or an old callback can be the only remaining alarm.

### 7. Notification delivery is not tied to a durable transition identity

- **Location:** AD-4; AD-7; Consistency Conventions — Notifications
- **Trigger condition:** A crash occurs between committing a transition and issuing its notification. One implementation records a notification marker before calling the notification API (possible lost alert); another records it afterward (possible duplicate alert). The “once” rule does not define a transition ID or crash semantics.
- **Guard snippet:** Define a stable notification key (session ID plus transition generation) and choose at-most-once or retry semantics; persist the corresponding delivery marker at the specified point and apply it to both on-time and reconciled transitions.
- **Potential consequence:** Restarting around a transition can suppress a reminder in one build and duplicate it in another.

### 8. Notification content allows incompatible user-facing contracts

- **Location:** AD-7
- **Trigger condition:** One implementation reports the phase's default duration; another reports remaining time at notification delivery. Both satisfy “duration or remaining time,” but the message text and user expectation differ, especially after delayed delivery or extension.
- **Guard snippet:** Choose one required content contract and define the value source (for example, active phase label plus remaining time calculated when the notification is emitted).
- **Potential consequence:** The same transition presents contradictory activity timing between builds.

### 9. Lifecycle commands have no complete transition table

- **Location:** AD-6; Consistency Conventions — Session lifecycle
- **Trigger condition:** Pause while inactive, resume while running, repeated Start, or Stop while paused occurs. One implementation ignores the command; another starts or mutates a session. The specified normal-path meanings do not define these inputs.
- **Guard snippet:** Add a lifecycle-by-command table defining accepted commands, resulting state, and whether they persist, schedule/cancel a wakeup, or notify; include repeated commands and invalid-state commands.
- **Potential consequence:** UI behavior and coordinator state diverge on ordinary repeated taps or stale controls.

### 10. Failure handling names outcomes but not the state contract

- **Location:** Consistency Conventions — Errors; AD-3
- **Trigger condition:** Persistence succeeds but wakeup scheduling fails, or persistence itself fails. One implementation retains a running in-memory session and reports an error; another pauses or marks the session inactive. Both avoid claiming background transition is guaranteed and surface failure.
- **Guard snippet:** Define the persistent/in-memory state and allowed user actions for each failure point, whether retry is automatic, and the exact condition for showing a session as background-scheduled. Do not claim recoverability when the authoritative write failed.
- **Potential consequence:** UI, persisted state, and actual wakeup availability disagree after an operational failure.

### 11. UTC deadline behavior under manual clock changes is open

- **Location:** AD-2; Deferred — timer tolerance and device-clock changes
- **Trigger condition:** The device clock is moved forward or backward while a session is running. One implementation treats UTC wall-clock deadlines as authoritative and immediately reconciles; another compensates using elapsed monotonic time. Both preserve duration across timezone/DST changes, but those changes do not cover explicit clock adjustment.
- **Guard snippet:** State whether manual UTC clock changes alter phase timing; define forward/backward adjustment handling and the authoritative clock source used by recovery and display.
- **Potential consequence:** The same session expires early, late, or not at all depending on implementation and device history.

### 12. Device verification and release checks are not reproducible gates

- **Location:** AD-3; Stack; Deferred — firmware verification, SDK/toolchain pin, packaging and test setup
- **Trigger condition:** One implementation treats a simulator build as sufficient and ships with a chosen SDK; another requires a physical-device leave-app, wakeup, vibration, power-off recovery, and install/update test. The spine says to verify these behaviors but does not define versions, procedure, acceptance criteria, or release gate.
- **Guard snippet:** Pin the supported SDK/toolchain and target firmware (or an explicit compatibility range), and define a shared build/install/test checklist with pass criteria for background wakeup, missed-deadline recovery, notifications, power-off recovery, and app update. Block release until required device checks pass.
- **Potential consequence:** Units can both claim completion while targeting incompatible toolchains or shipping with unverified reminder behavior.
