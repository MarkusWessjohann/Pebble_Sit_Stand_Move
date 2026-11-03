---
title: 'Use the Sit icon as the application launcher icon'
type: 'feature'
created: '2026-11-03'
status: 'done'
route: 'oneshot'
review_loop_iteration: 0
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The app has no launcher (menu) icon, so the Pebble application launcher shows the default icon. The owner wants the Sit icon (the chair) there.

**Approach:** Add a 25x25 monochrome chair PNG as a bitmap resource flagged `menuIcon` in `package.json`. The chair mirrors the in-app Sit icon. No C code changes.

</frozen-after-approval>

## Implementation Notes

- Added `resources/images/menu_icon.png` (25x25 RGBA, black chair on transparent background, drawn from the same shapes as `prv_draw_icon` Sit) and a `MENU_ICON` bitmap entry with `menuIcon: true` in `package.json` `resources.media`. No C changes; `RESOURCE_ID_MENU_ICON` is intentionally unused in code.
- Not built, not seen in an emulator or on a device: the Pebble SDK is not available in this sandbox. `package.json` parses as valid JSON.


## Review Triage Log

- Spec status stale / notes empty: false, fixed in finalization.
- RGBA PNG format: low, not fixed. The SDK converts PNG alpha to the platform format; a 25x25 transparent icon is the standard menu icon form.
- Looks wrong in launcher / not checked on display: maybe-false. Needs a `pebble build` and an emulator or Pebble Time 2 launcher view to settle; left to the owner.
- Chair not centred, reads like "H": low, not fixed. Margins are symmetric (6/6, 2/2); shape matches the in-app Sit icon.
- Differs from in-app icon geometry: low, not fixed. Same elements (back post, seat, legs); the 25x25 canvas needs different proportions.
- No per-platform variants: false. Pebble scales/uses one 25x25 menu icon on all platforms; not requested.
- Unused resource ID: false. `menuIcon` resources are consumed by the launcher, not by app code.
- No automated check / README / source asset / build verification: low, not fixed. Out of request scope; build verification is noted above as not done.
- Untracked files: false. Staged together in the commit.
