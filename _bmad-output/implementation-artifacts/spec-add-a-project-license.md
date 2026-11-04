---
title: 'Add a project license'
type: 'chore'
created: '2026-11-04'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
context: []
baseline_commit: '706c2a0abd8dc0d5280178ff45bb1cb355492cb6'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The repository does not state the terms under which others may use, modify, or distribute the project.

**Approach:** Add the standard MIT License at the repository root with the confirmed attribution. Declare `MIT` in `package.json`, set its `private` field to `false` while retaining the current author value, and add a README link to the license.

**Decisions (owner, 2026-11-04):** Use the MIT License, with copyright notice `Copyright (c) 2026 Markus Wessjohann`. Set `package.json` to public by changing `private` from `true` to `false`; this explicitly authorizes making the package publishable, but does not itself publish it anywhere. Leave the existing package author field unchanged.

</frozen-after-approval>

## Implementation Notes

- Baseline commit: `706c2a0abd8dc0d5280178ff45bb1cb355492cb6`.
- Owner selected MIT, copyright holder Markus Wessjohann, and authorized public package metadata. `private: false` only enables publication; no package was published.
- `npm pack --dry-run --json` initially included 337 files, including internal BMAD planning and skill files. Added a `files` allowlist in `package.json` for the app, build/test inputs, and public-facing documentation; verified with a repeated dry run.
- The allowlist excludes repository planning, agent configuration, and Python `__pycache__` files; it includes the app, build inputs, host tests (required by the included test script), and public-facing docs. Removed the unused `main.py` stub from package contents.
- Review found that the published `scripts/test.sh` initially lacked its `tests/` dependencies. Added `tests/` to the allowlist; verified package contents again.
- Final `npm pack --dry-run --json` reports 25 package files; it contains the MIT license, source, resources, build files, test script and test inputs, with no BMAD planning/agent files or Python `__pycache__` entries.
- Verification: `sh scripts/test.sh` passed (session coordinator tests and four Python wrapper tests); `npm_config_cache=/tmp/opencode/npm-cache npm pack --dry-run --json` completed successfully.
