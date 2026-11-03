---
title: 'Rename the app to "Sit Stand Walk"'
type: 'chore'
created: '2026-11-03'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
baseline_commit: '1e56de1b8cd0687d33d9180282bee911bb9bc120'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The app is shown as `sit_stand_move`. The owner wants it called "Sit Stand Walk".

**Approach:** Set the Pebble `displayName` in `package.json` to `Sit Stand Walk` and update the README title to match. The npm package `name`, the `uuid`, source file names and all behavior stay unchanged.

</frozen-after-approval>

## Implementation Notes

- Changed `pebble.displayName` in `package.json` and the title in `README.md` to `Sit Stand Walk`. Left unchanged on purpose: npm `name`, `uuid`, `sit_stand_move.c`, `pyproject.toml`, `main.py` (internal identifiers; renaming risks the install identity and build paths without changing what the watch shows).
- Not verified on an emulator or device; `build/` is generated and stays stale until the next build.

## Review Triage Log

- Partial rename / internal identifiers differ from display name: low, not fixed. Intentional per Intent; renaming `name` or files is outside the request.
- Spec lacks tasks/verification: false. One-shot specs omit those sections by design.
- Story not in `sprint-status.yaml`: low, not fixed. 3-1 is not tracked there either; no epic 3 exists.
- Launcher rendering of a 14-character name on small screens: maybe-false, deferred. It would need an emulator/device check.
- "Walk" name vs. `Move` phase label: low, not fixed. Owner asked only for the app name; phase labels are a separate decision.
- README still template text: low, not fixed. Outside the request.
- `build/sit_stand_walk.pbw` mismatch: false. No `.pbw` exists in the repo.

