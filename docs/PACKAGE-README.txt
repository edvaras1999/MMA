Show Player In Inventory LE 2.4-test7
Skyrim Legendary Edition 1.9.32.0.8 / SKSE 1.7.3 / Win32

LOCAL TEST BUILD - IN-GAME ACCEPTANCE PENDING
This is a native LE adaptation of Ivy / ItzIvy05's Show Player In Inventory 2.4.
Offline checks pass. Visual behavior and compatibility in an actual load order
have not been validated. Keep this as a separate mod so it can be removed easily.

INSTALLATION
Install the SKSE folder into the game's Data folder, preferably through your mod
manager. Start Skyrim through skse_loader.exe. Requires Skyrim LE and SKSE 1.7.3.
No ESP, Papyrus scripts, Address Library, FLICK or animation behavior generation
is required. Never install the upstream SE DLL into LE. Install only this LE DLL.
KHA-OS for LE is optional: it provides the live settings panel. Without it, edit
SKSE/Plugins/ShowPlayerInInventoryLE.ini and press F10 in game to reload.

BEHAVIOR
Inventory and Magic are enabled by default. Barter and non-actor Container menus
can be enabled in the INI. Combat and unsupported camera states suspend the preview.
The camera moves around the player and restores the previous perspective, FOV,
owned projection planes and held controls when the menu closes. Item preview
hiding, blur removal, remembered framing, paused actor animation and neutral
facial expression are configurable. The player's gameplay orientation is retained.

CHANGES IN TEST7
Fixed a later source of portrait recentering on both I and Tab -> Inventory.
Native FOV updates can rebuild a centered NiCamera frustum after the earlier menu
and camera-state callbacks. The owned portrait camera now reapplies horizontal/
vertical placement immediately before NiCamera builds its world-to-screen matrix.
The original camera callback and context are retained exactly once; other cameras,
closed/blocked/disabled views and foreign-thread updates remain native.
This final pass preserves the current native frustum span, near/far clip, viewport
and field of view. It changes only the four owned projection planes and performs
no extra node, animation or physics update. Two thousand late native resets are
covered offline; live acceptance with your camera/ENB/UI mods is still required.
Default horizontal position is 0.82 in both the INI and missing-key fallback.
Keep your personal setting if desired; fScreenX chooses the portrait center as a
fraction of screen width. A larger value moves the portrait further right.
The log records the requested opening position, projection callback/owner thread
and final projection repair count to distinguish settings from late overwrites.

CHANGES RETAINED FROM TEST6
The camera no longer repeats its scene propagation when its live local/world
transform, FOV, projection planes and complete camera data are unchanged. Native
camera overwrites, child camera transforms, clipping/viewport/matrix changes and
manual framing still trigger immediate propagation. Engine readiness and RTTI are
checked live; this does not retain page-validation results between callbacks.
The input sink returns immediately outside an active inventory preview, avoiding
pointer walks and stale wheel accumulation during normal gameplay.
Paused actor animation, scene updates and the working physics path are retained
at their existing cadence. No physics provider or world-pause behavior was changed.
The supplied INI retains the right-side 82% portrait position. To keep personal
settings, retain your INI and change only fScreenX=0.8200 if needed.
With bEnableLogging=1, every 64th active frame/camera/menu callback is sampled.
Closing the inventory writes a bounded summary of mean/peak microseconds for owned
work, face, actor/scene, item previews, framing, native camera, reapply and native
menu advance. These inclusive samples can overlap and can miss isolated stalls;
they do not measure GPU cost, frame rate, or every installed mod. Logging disabled
also disables profiling clock reads. No per-frame disk logging is added.
Source-level reductions are verified offline; actual FPS gains need live testing.

CHANGES RETAINED FROM TEST5
Fixed the Tab -> Inventory path. LE can keep Tween registered while its movie is
hidden behind Inventory. Test4 incorrectly waited for Tween to close, so direct I
worked while the Tab route stalled. The live child inventory now takes priority.
The hidden parent no longer blocks camera reapplication; a late native Tween
camera return is repaired while the child owns the view. Closing the child returns
to the native Tween camera, retaining its original saved gameplay perspective.
Closing Tab then returns to the original first- or third-person view.
Native Tween End restores its baseline FOV. That restored value and lens are saved
before the first owned view, so the temporary Tween zoom is not saved as gameplay
FOV. Wide native FOV values are preserved independently of inventory setting limits.
Hidden-parent blur is suppressed alongside the child contribution, restored for
the foreground parent and balanced before its destructor. Parent update messages
are normalized around the original handler so refresh cannot add a leaked blur
contribution. Other menus' blur and original callback return values are preserved.
The test fixture keeps the parent open during 2000 camera/message frames, tests
both gameplay perspectives and returns through Tab; this no longer assumes that
opening a child closes the parent. The revised build still needs in-game acceptance.

FEATURES RETAINED FROM TEST4
Inventory opening remains menu-driven, with no key remapping or timed key workaround. The player view is reapplied
after the original third-person camera update so a later native pass does not
replace the frontal position, rotation or FOV. A late first-person return is repaired
while the inventory owns the view; pending foreign camera transitions are respected.
Right-side framing now shifts the view projection rather than placing the camera
beside the player, keeping the portrait frontal while retaining its screen position.
Face-camera mode uses the rendered model heading when valid. Reset orbit on opening
is enabled by default, so an old saved side angle does not defeat frontal framing.
To retain a chosen orbit across openings, disable Reset orbit on opening or set
bResetOrbitOnOpen=0. Zoom, pan and other remembered framing remain available.
Paused animation now propagates the player scene before camera framing. This keeps
pose, skinning/bounds and attached objects/lights current. It does not add a second
light, change exposure/ENB settings or replace an installed face-light plugin.
Disable Animate player in paused menus to disable both forced graph and scene
updates for custom unpause providers. Projection/FOV commits wait for a valid full
transform, avoiding a partial projection change if a parent transform is invalid.
The precise live brightness, shadows and compatibility still need visual acceptance
in the user's cell, lighting setup and load order. Test5 has not been run in game here.

CHANGES RETAINED FROM TEST3
Native item previews are hidden by default while the player camera is active.
Previous unversioned configurations are migrated even if the old INI stored
bHideItemPreviews=0. Saving any setting writes iSettingsVersion=4; after that,
a deliberate preview preference is respected. The packaged INI includes both keys.
The hide option now covers every retained model, including array/selection changes,
and stays enabled while quantity dialogs and the settings panel are open. Closing
the player view returns only visibility changed by this plugin and releases its
model references; objects already hidden stay hidden. Item names, statistics,
icons and normal inventory actions remain available; only the 3D preview is masked.
To show native item previews deliberately, untick Hide item previews in KHA-OS,
or set iSettingsVersion=4 and bHideItemPreviews=0, then press F10.

CHANGES RETAINED FROM TEST2
Corrected camera-owner tracking after early menu callbacks, and queued cross-thread
resets for the actual camera owner. Deferred load cleanup discards scene-owned
snapshots so a replacement camera does not receive the old FOV/projection. Inactive HUD work is reduced; covered fixture
menu probes fall from 11 to 5 per frame, retaining live readiness checks.
Controller sticks now leave the deadzone smoothly; stick/trigger noise no longer
marks settings as changed. Live panel edits are grouped after 400 ms of quiet and
flushed on close, instead of writing the whole INI each slider frame. Closing the
panel clears queued input and resets its input state before reopening. NiCamera
lookup tolerates empty/non-camera earlier children while still checking RTTI.
These source-level improvements do not imply a measured FPS gain on your load order.

CONTROLS
Begin a drag on the right of the screen (outside the leftmost 45% item-list area).
Right mouse drag: orbit. Middle mouse drag: pan. Ctrl+middle: pitch.
Ctrl+wheel: zoom. Ctrl+Shift+wheel: field of view.
Controller: hold RB; right stick orbits/zooms; left stick pans; triggers change FOV.
F10 reloads the INI. Mouse button and controller hold button are configurable.
The default fit assumes a list on the left; custom menu layouts may need manual
framing (bAutoFrame=0, fScreenX/fScreenY). Dot and comma decimals are accepted;
saved INI values use dots. With bRememberCamera=0, only framing resets on opening.

LE DIFFERENCES AND LIMITS
The SE camera framework and SE integrations were replaced by a native LE backend.
SmoothCam/FLICK APIs and the SE weapon-attachment refresh are not included.
Headtracking targets are not forcibly cleared. Physics follows the installed LE
provider; this plugin does not unpause the world or force a Havok/cloth step.
Paused animation and neutral face are skipped for recognized SkyrimSouls DLLs;
other custom unpause plugins may require disabling those two INI options.
Native animation advances the actor graph; cloth and third-party physics behavior
need testing with the installed providers. Custom skeletons, camera plugins,
SkyUI variants, face mods and ultrawide layouts have not been certified.

FIRST IN-GAME CHECK
Use a disposable save. Open/close Inventory and Magic from first and third person,
including Tween transitions; check framing, face, animation, weapon visibility,
input and restored FOV. Test supported containers/barter only after enabling them.
Try dialogue, combat, loading and saving, repeated menu changes, mouse/controller,
KHA-OS if installed, and your normal camera/physics mods. If anything is wrong,
disable the feature or remove the DLL and INI, then provide the log and conditions.
The log is ShowPlayerInInventoryLE.log under Documents/My Games/Skyrim/SKSE.
The log records thread ownership, existing hooks, signature changes and faults.
After three guarded faults the feature stops; original menu calls stay chained.
Offline results do not establish performance or correctness in game.

VALIDATION
Release Win32; static CRT; exactly the two SKSE exports and six KHA-OS exports.
Actual-source fixture: 17782 checks, zero failures; 2000 simulated steady frames
in each of the direct and retained-parent paths, plus 300 stable camera frames, with interleaved native camera/
message work and zero own C++ allocations; late-exit callbacks and timeout tested.
Seventy-eight separately compiled defects were detected by diagnostic assertions.
Static TESV checks: 18 function signatures and 14 exact hook targets.
Production DLL loaded and queried in a separate x86 process (7 checks).
A clean rebuild from the bundled corresponding source is checked before packaging.
The optional graphical panel has not been exercised on a live D3D9 game device.

SOURCE AND LICENSE
GPL-3.0, original source and license included. LE adaptation: Mhacks79.
Original: Ivy / ItzIvy05; upstream credits and third-party notices are retained.
Dear ImGui: MIT, included. SKSE/common source licenses: included.
See Source/README.txt for the build instructions and porting evidence summary.
Upstream: https://github.com/ItzIvy05/Show-Player-In-Inventory/
