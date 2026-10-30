# Changelog

Test builds of the Legendary Edition port, newest first. Each test fixes what the previous one left open; none has
had in-game acceptance yet (see `docs/STATUS.md`).

## 2.4-test7 - 2026-10-30 (source published)

- **Final projection ownership.** Native field-of-view setup can write a centred frustum and update the camera node after
  the earlier menu and camera-state callbacks. A chained `NiCamera` callback now reapplies only the four owned
  projection planes, immediately before the world-to-screen matrix is built, and forwards the original call exactly once.
  It keeps the native frustum span, clip planes, viewport and FOV, and performs no extra node, animation or physics update.
- Default horizontal position `fScreenX=0.82` in the INI and in the missing-key fallback.
- The log records the requested opening position, the callback/owner thread and the final projection repair count.

## test6

- The camera no longer repeats its scene propagation when its live transforms, FOV, projection planes and camera data are
  unchanged. Any native overwrite, child transform, clipping, viewport, matrix or manual framing change still propagates.
- The input sink returns immediately outside an active inventory preview.
- With `bEnableLogging=1`, every 64th active callback is sampled and a bounded timing summary is written when the preview closes.

## test5

- Fixed the `Tab` > Inventory path: the live child inventory now has priority over the hidden Tween parent.
- The native Tween FOV is saved before the first owned view, so the temporary zoom is not saved as gameplay FOV.
- Blur contributions of the hidden parent are balanced around the original handlers.

## test4

- Framing is reapplied after the original third-person camera update. A late first-person return is repaired.
- Right-side framing shifts the view projection instead of translating the camera, keeping the portrait frontal.
- Face-camera mode uses the rendered model heading; an optional orbit reset on opening.
- Paused animation propagates the player scene before camera framing.

## test3

- Native item previews are hidden by default while the player camera is active; older configurations are migrated once
  (`iSettingsVersion=4`).

## test2

- Camera owner tracking corrected; cross-thread resets are queued for the owner thread; deferred load cleanup discards
  scene snapshots.
- Controller sticks leave the dead zone smoothly; live panel edits are grouped and flushed on close.

## test1

- First native Legendary Edition backend, replacing the Special Edition camera framework and integrations.
