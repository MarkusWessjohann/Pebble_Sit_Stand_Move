# PRD-to-Architecture Reconciliation

Compared the PRD with the current architecture spine. This report includes only PRD requirements, constraints, or meaningful product intent that are missing, weakened, or insufficiently represented in the spine. The spine was not edited.

## Gaps

### 1. Fixed cycle durations and v1 duration constraint are absent

- **PRD:** §1, §3, §4.1 FR-2, §7.1–7.2 specify a repeating 45-minute Sit, 10-minute Stand, 5-minute Move cycle; defaults remain fixed in v1, with user-configurable default phase lengths out of scope.
- **Spine:** AD-2 through AD-4 and the conventions table define deadline handling and phase order, but not phase durations or that the default lengths are fixed. The stack/architecture should preserve the actual schedule, not just its ordering.
- **Gap:** Record 45/10/5 and the v1 fixed-default constraint as binding product behavior; distinguish the user-selected per-phase extension from changing default phase lengths.

### 2. Session and control action semantics are not stated as architecture rules

- **PRD:** §2.2, §4.1 FR-1 and §4.2 FR-4–FR-6 require manual start/stop; every new session starts in Sit; stopping ends the session and a later start is fresh (stopped sessions do not resume); pause preserves the active phase and remaining time and produces no phase notifications; skip immediately enters the next phase at its default duration without carrying over skipped time; extension adds selected time to the current phase before normal advancement.
- **Spine:** AD-1 assigns state ownership and names lifecycle states; AD-2 covers pause/resume deadlines; the capability map refers to the FRs. It does not spell out these externally observable semantics, including which lifecycle/action outcomes the coordinator must enforce.
- **Gap:** Preserve these start/stop, pause, skip, and extend outcomes explicitly so implementations do not infer incompatible behavior from the general state-machine and deadline rules.

### 3. Notification content requirement is weakened

- **PRD:** §2.2, §4.1 FR-3 and its consequences require a phase-change notification that identifies the newly active phase and its duration or remaining time. Vibration is the initial notification-mode assumption.
- **Spine:** The notification convention says to notify on an on-time phase transition and the diagram names watch notification/vibration, but it does not bind the notification content to the new phase and its duration/remaining time.
- **Gap:** Specify the notification payload/content requirement, not only that a notification occurs.

### 4. Background support boundary remains underspecified

- **PRD:** §5.1 requires architecture to verify background timing and notifications on a specific Pebble Time 2 firmware/OS before implementation. It explicitly distinguishes app not foregrounded, dismissed, and restarted after termination, and leaves open which of these behaviors the requirement covers.
- **Spine:** AD-3 and Deferred require target verification and mention launch while not foregrounded; AD-4 covers launch/recovery after missed deadlines. The distinct app-dismissed and app-terminated/restarted cases are not identified as separate support expectations to resolve and verify.
- **Gap:** Carry the PRD's unresolved background-scope distinction into the architecture verification criteria; do not treat “not foregrounded” as settling dismissed and post-termination behavior.

### 5. Reminder-only product boundary is only partially represented

- **PRD:** §1, §6, and §7.2 define this as a personal reminder utility, not an activity tracker: no posture/step/movement sensing, health claims, activity history, adherence analytics, or automatic working-hours/overnight schedules. §6–7 also rule out a required phone companion, cloud, and social features.
- **Spine:** AD-5 explicitly rules out required phone/cloud runtime dependencies, but does not preserve the reminder-only boundary, sensing/analytics exclusions, or the user-started/no-automatic-schedule constraint.
- **Gap:** Retain these product boundaries where architecture or implementation choices could otherwise introduce tracking, telemetry, or automatic session scheduling.

## Refinement accounted for

The powered-off-across-deadlines behavior is represented in PRD §4.1 FR-3 and spine AD-4: process every elapsed phase in order, then issue at most one notification for the resulting current phase. No gap is reported for this behavior.

## Sources

- PRD: `_bmad-output/planning-artifacts/prds/prd-sit_stand_go-2026-10-21/prd.md`
- Architecture spine: `_bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md`
