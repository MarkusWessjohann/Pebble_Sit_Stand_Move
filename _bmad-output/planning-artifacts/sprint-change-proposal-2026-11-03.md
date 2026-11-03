# Sprint Change Proposal — Owner-Selectable Haptic Pattern (Settings Menu)

**Date:** 2026-11-03
**Project:** sit_stand_go
**Trigger:** Owner request for a Settings menu to change the haptic notification behavior (Short, Long, Double, Until Select)
**Status:** Approved edit by edit and applied by the project owner on 2026-11-03

## 1. Issue Summary

The owner asked for a Settings menu in which the haptic notification can be set to a short pulse, a long pulse, a double pulse, or a pattern that repeats until Select is pressed. Epic 3 (Stories 3.1 and 3.2) was written to deliver this.

The sprint-planning readiness gate then returned CONCERNS: the adopted planning documents contradicted Epic 3. Architecture AD-14 ("Haptic mode is fixed for v1") said "Do not add notification-mode customization", and its stated purpose was to prevent different notification settings. The UX contract said "no settings … surface is in scope". PRD SM-C1 asked that reminder volume not increase, which a repeating alert works against. The conflict was missed when Epic 3 was drafted because AD-14 was not read at that point.

Evidence: `ARCHITECTURE-SPINE.md` AD-14 and the Notification mode row; `EXPERIENCE.md` Information Architecture, Components, and Interaction Primitives; `DESIGN.md` Components and Layout; PRD §4.2, §6, §7.1, and SM-C1.

## 2. Impact Analysis

### Epic impact

- **Epic 1 and Epic 2:** Done. No story needs rework.
- **Epic 3:** The stories are correct as written. The only gap was that other artifacts disagreed with them.

### Story impact

No story text changes. Story 3.2's safety cap was fixed at 2 minutes (owner decision) before this proposal. Both stories already require `DESIGN.md` and `EXPERIENCE.md` to be updated in the same change; that is now done ahead of implementation, so the stories need only verify them.

### Artifact conflicts and updates

- **Architecture Spine:** AD-14 superseded and rewritten. New AD-18 records the Until Select alert. Consistency table and the FR-3 capability row updated. Frontmatter `binds` extended with FR-9 to FR-11.
- **PRD:** Added FR-9 (choose the pattern), FR-10 (apply to every haptic request), and FR-11 (repeat until Select). §6 Non-Goals and §7.1 In Scope updated. SM-C1 carries an explicit exception for Until Select.
- **UX `EXPERIENCE.md`:** Settings row and window, alert cue, Until Select state, Settings and Select-while-alerting interaction rules, and the open-items menu order.
- **UX `DESIGN.md`:** Menu row, Settings window, alert cue (replacing the clock line while an alert repeats), and the surface rule.
- **Product brief:** Scope line and a dated decision added. The Solution paragraph is unchanged.
- **Epics:** No change; `epics.md` already contains FR9–FR11, NFR10–NFR11, UX-DR18–UX-DR20, and Epic 3.

### Technical and MVP impact

No code exists for this feature yet. The single haptic call site is `prv_request_haptic()` in `src/c/session_coordinator.c`, and Select is also the middle control (`session_coordinator_middle_control()`), which AD-18 handles by dismissing the alert before the middle-control dispatch. The setting uses its own persist key and does not change the session record schema. MVP goals are unchanged. This does not reopen Story 1.8, which is closed by owner waiver; physical feel of each pattern is an owner check.

## 3. Recommended Approach

**Recommendation: Direct adjustment.** Update the planning artifacts so they agree with Epic 3, then implement Stories 3.1 and 3.2 in order.

- **Effort:** Low. Documentation only, no code change.
- **Risk:** Low to medium. The repeating alert is the main risk: it must remain one haptic attempt, must not replay after relaunch, and must not let the dismiss press trigger the middle control. AD-18 and Story 3.2's criteria cover each case.
- **Timeline:** No schedule impact. Story 2.1 is still in `review` and touches the same view files; close it before building Epic 3.
- **Rollback:** Not applicable.
- **MVP review:** Not required.

## 4. Detailed Change Proposals

Each proposal was shown as old and new text and approved individually by the project owner on 2026-11-03.

| # | Artifact | Change | Status |
|---|---|---|---|
| 1 | Architecture AD-14 | Superseded: haptic pattern is owner-selectable and the only notification setting; stored in its own key | Applied |
| 2 | Architecture AD-18 | New: Until Select is one committed attempt, repeats until Select, 2-minute cap, volatile, dismiss before middle-control dispatch | Applied |
| 3 | PRD | FR-9 to FR-11, §6, §7.1, SM-C1 exception | Applied |
| 4 | UX `EXPERIENCE.md` | Settings row and window, alert cue, state and interaction rules | Applied |
| 5 | UX `DESIGN.md` | Menu row, Settings window, alert cue replacing the clock, surface rule | Applied |
| 6 | Product brief | Scope line and dated decision | Applied |
| 7 | Epics | None needed | n/a |

Decision recorded during review: the alert cue `Select to stop` replaces the clock line at the top of the session view while an alert repeats (option A). The alternative, sharing the error line, would have dropped the cue whenever an error was shown.

## 5. Implementation Handoff

**Scope: Minor.** Direct implementation by the Developer agent after the planning artifacts agree.

- **Sprint Planning:** re-run `bmad-sprint-planning` to add Epic 3 and Stories 3.1 and 3.2 to `sprint-status.yaml` as `backlog`.
- **Developer agent:** build Story 3.1, then Story 3.2. Verify the already-updated `DESIGN.md` and `EXPERIENCE.md` against the built screens.
- **Owner:** confirm the feel of each pattern on Pebble Time 2; emulator results are recorded as emulator results.

Success criteria: the planning artifacts, epics, and code agree; the pattern persists; Short matches today's behavior; Until Select stops on Select, the 2-minute cap, Stop, or supersession and never resumes after relaunch; and the Select dismiss press starts no timed pause.
