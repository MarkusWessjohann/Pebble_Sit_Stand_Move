---
title: 'Rename app to Sit Stand Move and align Move terminology'
type: 'chore'
created: '2026-11-04'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
context: []
baseline_commit: '4553b2d1f0e4070284f6c13ef0844f6f7f2c6633'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The watch app is named `Sit Stand Walk`, inconsistent with the third phase's established name `Move`.

**Approach:** Rename the Pebble display name and README heading to `Sit Stand Move`, and update active brief/PRD/architecture/UX/epic planning artifacts to use the `Move` phase name consistently. Preserve the package name, UUID, source paths, historical records, internal identifiers, and unrelated uses of “walk”.

**Decisions (owner, 2026-11-04):** Apply the rename to package display metadata, README, and active planning docs. Preserve historical implementation records, memlogs, internal identifiers, generated artifact paths, and unrelated uses of “walk”.

</frozen-after-approval>

## Implementation Notes

- Changed the Pebble display name, README heading, and active brief, PRD, architecture, UX, and epic text to `Sit Stand Move` / `Move`. Preserved UUID, package name, source paths, project slug, historical memlogs, prior rename spec, review reports, and unrelated “walk” occurrences.
- `build/appinfo.json` is generated output and remains stale (`Sit Stand Walk`) because the pinned Pebble CLI could not run: uv could not download `certifi==2026.10.23` in this environment. The next successful Pebble build should regenerate it.

## Review Triage Log

- maybe-false (deferred): Launcher fit for `Sit Stand Move` and generated Pebble metadata remain unverified; check a successful pinned Pebble build and launcher display on target/emulator. The current generated `build/appinfo.json` still contains `Sit Stand Walk`; the CLI dependency could not be downloaded here.
- false: Historical review artifact still says `Sit Stand Go`; it is intentionally preserved as a record and is not active product documentation.
