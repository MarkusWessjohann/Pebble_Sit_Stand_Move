# Timed Pause Cross-Document Review

**Scope:** Current PRD, product brief, and architecture spine, with emphasis on Timed Pause lifecycle behavior, FR/AD/UJ traceability, haptics, metrics, MVP framing, and the Pebble Time 2 verification gate.

**Verdict: Conditional pass.** The three artifacts agree on Timed Pause's core behavior: suspend for 5/10/30 minutes, preserve phase remaining time, resume the same phase without advancing, and request an app open and haptic at pause end. The PRD and architecture also specify early-resume cancellation and stale-callback rejection. Resolve the architecture's FR-7 binding error, make the timed-paused Resume transition explicit in the coordinator lifecycle contract, and align the brief, haptic wording, and success metrics with the more precise behavior already present in the PRD and spine. Pebble Time 2 behavior remains an unpassed pre-implementation gate.

## Actionable findings

### 1. AD-15 is bound to the wrong functional requirement

- **Evidence:** PRD §4.2 labels FR-6 “Skip a Phase” and FR-7 “Extend a Phase.” Architecture AD-15 is the extension rule but says `Binds: FR-6` (§AD-15, lines 111–115). The capability map correctly places AD-15 in the FR-7 row (line 190).
- **Finding:** The decision's own traceability contradicts both the PRD IDs and the capability map.
- **Action:** Change AD-15's binding to FR-7 and check any generated references that use the AD-15/FR-6 pairing.

### 2. Timed-pause Resume needs an explicit lifecycle transition in the command contract

- **Evidence:** PRD FR-5 permits the user to manually resume before pause end and requires cancellation of the pause-end Wakeup (lines 97, 102). AD-17 specifies the resulting `timed_paused` → `running` transition and stale-callback rejection (lines 123–127). AD-10, however, describes “Resume from manual pause” and says invalid/repeated lifecycle commands are no-ops, without listing Resume from `timed_paused` (line 85).
- **Finding:** The dedicated AD-17 rule supplies the intended behavior, but the general serialized command/lifecycle rule does not say that Resume is valid in `timed_paused`. An implementer following AD-10's command table could treat it as an invalid no-op.
- **Action:** In AD-10 or a compact lifecycle transition table, specify `timed_paused + Resume → running`, cancel the timed-pause Wakeup, preserve the saved phase remaining duration, set the new phase deadline from current UTC, and rely on generation/ID validation to reject stale callbacks. State that the `now >= pause-end deadline` reconciliation runs first, so a resume processed after the deadline does not suppress the due PauseEnd.

### 3. AD-4's recovery rule is too broad for Timed Pause

- **Evidence:** AD-4 says that at any recovery where `now >= persisted deadline`, elapsed phases are processed and the resulting phase is rebased to its full default duration; it also binds FR-5 (lines 45–49). A due Timed Pause deadline instead resumes the interrupted phase with its saved remaining duration under AD-12 and AD-16 (lines 97, 121).
- **Finding:** AD-12 gives the correct Timed Pause exception, but AD-4's general wording appears to apply the full-default-duration recovery rule to timed-pause deadlines as well. That contradicts FR-5 and the timed-pause-specific decisions unless readers infer an unstated exception.
- **Action:** Narrow AD-4's rebase rule to an overdue running-phase deadline, and state that timed-pause recovery follows AD-12/AD-16: resume the preserved phase with saved remaining time, without cycle advancement.

### 4. Capability-map rows omit decisions that define Timed Pause semantics and presentation

- **Evidence:** The FR-5 and UJ-2 rows cite AD-1–AD-4 and AD-9–AD-12 plus AD-16/17 (architecture lines 188 and 192). AD-6 defines pause/skip/extend semantics (line 61); AD-7 defines pause-end haptic, app open, and displayed resumed phase (line 67). These decisions are not cited in either map row.
- **Finding:** The source rules exist, but the map does not trace the two Timed Pause capabilities to all governing behavior, weakening implementation traceability.
- **Action:** Add AD-6 and AD-7 to the FR-5 and UJ-2 capability-map bindings. Keep AD-16/17 for the timed-pause-specific and early-resume rules.

### 5. AD-14's fixed haptic policy omits Timed Pause completion

- **Evidence:** PRD FR-5 and AD-16 require a pause-end haptic (PRD lines 91–98; architecture line 121). AD-12 includes PauseEnd in the one-haptic request contract (line 97). AD-14 says v1 uses haptics for “automatic phase deadlines and Skip” and binds FR-3/all v1 notification behavior, but does not mention timed-pause completion (lines 105–109).
- **Finding:** The specific behavior is present elsewhere, but the architecture's policy-level statement is incomplete and can be read as excluding a Timed Pause haptic.
- **Action:** Include PauseEnd/timed-pause completion in AD-14's rule and FR-5 binding, or explicitly refer to AD-12/AD-16 as the complete set of haptic-producing events. Keep Extend and lifecycle controls silent.

### 6. Brief omits early-resume cancellation and stale-callback outcomes

- **Evidence:** PRD FR-5 requires canceling the pause-end Wakeup and ensuring an already-delivered stale callback neither vibrates nor reopens the app (lines 97, 102). Architecture AD-17 gives the same contract (line 127). The brief describes timed-pause completion and general pause/resume, but its Executive Summary, Success Criteria, Scope, and decisions do not cover early resume or stale callback behavior (lines 14, 36–41, 48, 60–66).
- **Finding:** The brief's user-facing scope and success criteria do not capture a key Timed Pause control/race behavior defined by the downstream requirements and design.
- **Action:** Add early-resume cancellation and “no haptic, no app reopen” for a stale pause-end callback to the brief's timed-pause success criterion and/or decision statement.

### 7. Success metrics blur haptic request, attempt, and physical delivery; stale reopen is not measured

- **Evidence:** PRD SM-3 says overdue Timed Pause “emits one haptic” (line 171), while FR-5 says “at most one” for overdue recovery (line 98). Architecture AD-12 specifies a persisted at-most-once haptic attempt and best-effort delivery (line 97). SM-4 says early resume “cancels that haptic” but does not include the no-reopen outcome (line 172), although FR-5 does.
- **Finding:** The metric can be read as guaranteeing physical delivery during overdue recovery, which exceeds the architecture's best-effort API contract; it also does not measure one of the stale-callback protections.
- **Action:** State whether success means one haptic request/attempt or observed physical delivery on the verified device. For overdue recovery, use “at most one request/attempt” unless the product requirement is intentionally stronger. Extend SM-4 (or add a criterion) to verify that early resume prevents both a haptic and app reopen from a stale callback.

### 8. Brief does not state the Timed Pause scope contingency if the device gate fails

- **Evidence:** PRD §5.1 says to revisit Timed Pause with the user if timed-pause Wakeup is unsupported (line 132). Architecture §Deferred repeats that contingency (line 196). The brief includes Timed Pause in first-version scope and says platform behavior must be verified, but does not state what happens if the target cannot support the required timed-pause haptic/foreground behavior (lines 14, 24, 48).
- **Finding:** The brief presents Timed Pause as committed first-version scope without carrying forward the explicit feasibility contingency used by the PRD and architecture.
- **Action:** Add that Timed Pause's MVP inclusion depends on the physical-device gate and that unsupported Wakeup/foreground/haptic behavior triggers a product decision to revisit the feature.

## Verified alignment / no finding

- **Core lifecycle:** PRD FR-5, architecture AD-2/AD-6/AD-9/AD-10/AD-16/AD-17, and the brief agree on the 5/10/30-minute choices, frozen phase clock, saved remaining time, same-phase resume, and no phase advancement at pause end.
- **Requirement and journey IDs:** PRD defines FR-5 as Timed Pause, FR-6 as Skip, FR-7 as Extend, and UJ-2 as the timed-pause journey. Architecture AD-16/17 bind FR-5 and UJ-2, and its current frontmatter binds FR-1–FR-7 and UJ-1/UJ-2. No frontmatter or UJ-ID omission is present in the current spine.
- **Early-resume stale callback device assertion:** PRD FR-5 and architecture AD-17 require neither haptic nor reopen. The current architecture §Deferred test explicitly says the stale callback “neither vibrates nor reopens” (line 196); this is covered by the current device gate.
- **MVP presence:** PRD §7.1 and the brief's First version scope both include Timed Pause; architecture also includes it in AD-16/17 and the pre-implementation acceptance gate. The gap is the brief's missing contingency noted above, not conflicting baseline scope.
- **Target verification gate:** PRD §5.1 and architecture §Deferred require exact target/toolchain identification and separate physical Pebble Time 2 verification of phase and timed-pause Wakeups, haptic delivery, and app foreground/open after leaving the app before implementation. The architecture gate also exercises 5/10/30-minute pauses, early resume, stale callbacks, overdue recovery, event races, and injected persistence/schedule/cancel failures. These checks are specified but have not been passed.

## Sources

- PRD: `_bmad-output/planning-artifacts/prds/prd-sit_stand_go-2026-10-21/prd.md` (§2.2 UJ-2; §4.2 FR-5–FR-7; §§5.1, 7, 8)
- Product brief: `_bmad-output/planning-artifacts/briefs/brief-sit_stand_go-2026-10-21/brief.md` (Executive Summary; Success Criteria; Scope; Open Questions and Assumptions)
- Architecture spine: `_bmad-output/planning-artifacts/architecture/architecture-sit_stand_go-2026-10-21/ARCHITECTURE-SPINE.md` (AD-6–AD-17; Capability → Architecture Map; Deferred)
