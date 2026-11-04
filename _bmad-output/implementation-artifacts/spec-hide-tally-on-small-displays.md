---
title: 'Hide the tally on small displays'
type: 'feature'
created: '2026-11-04'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
context: []
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The daily statistic does not fit some smaller watch displays.

**Approach:** Hide the daily statistic when either display dimension is under 180 pixels. Keep it visible when both dimensions are at least 180 pixels.

</frozen-after-approval>

## Implementation Notes

- `src/c/sit_stand_move.c` hides the daily tally layer when either root-layer dimension is below 180 pixels; a display with both dimensions at least 180 pixels keeps the tally visible.
- No coordinator or tally-counting behavior changes.
- Boundary check: 179 pixels in either dimension hides the tally; 180 pixels in both dimensions leaves it visible, matching the `< 180` condition.
- `./scripts/test.sh` passed the session coordinator tests and all four Python tests. Its Pebble CLI check reported that SDK 4.33.1 must be active and was unavailable, so the watch app build was not verified.

## Review Triage Log

- low — The UI threshold has no automated boundary test and is outside `scripts/test.sh`'s compilation coverage; checked the `< 180` condition against 179/180 pixel cases for both dimensions. Pebble SDK build remains unverified because SDK 4.33.1 is not active in this environment.
