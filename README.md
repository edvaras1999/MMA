<div align="center">

# Show Player In Inventory LE

**See your character in the inventory.**
A native Skyrim Legendary Edition port of Show Player In Inventory by Ivy.

[![Build](https://github.com/Mhacks79/Show-Player-in-Inventory-LE/actions/workflows/build.yml/badge.svg)](https://github.com/Mhacks79/Show-Player-in-Inventory-LE/actions/workflows/build.yml)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
![Skyrim LE 1.9.32.0.8](https://img.shields.io/badge/Skyrim-LE%201.9.32.0.8-5ab4f0)
![SKSE 1.7.3](https://img.shields.io/badge/SKSE-1.7.3-5ab4f0)
![x86](https://img.shields.io/badge/arch-x86-lightgrey)

[Original on GitHub](https://github.com/ItzIvy05/Show-Player-In-Inventory) ·
[Changelog](CHANGELOG.md) ·
[Status](docs/STATUS.md) ·
[Support on Ko-fi](https://ko-fi.com/mhacks79)

</div>

---

Show Player In Inventory LE moves the camera around your character while a menu is open, so that you can see them in
the inventory (and the magic menu), with the item list on one side and the character on the other. It is a native
[SKSE](https://skse.silverlock.org/) port of [**Show Player In Inventory**](https://github.com/ItzIvy05/Show-Player-In-Inventory)
by **Ivy** (ItzIvy05) for the original **Skyrim Legendary Edition** (`TESV.exe` 1.9.32.0.8, SKSE 1.7.3, 32-bit).

The Special Edition mod builds on CommonLibSSE-NG, the Address Library and FLICK. None of them exists on Legendary
Edition, so the engine side was rewritten as a native LE backend, with every address established from the LE executable.
No ESP, no Papyrus script and no animation behaviour are needed.

> **Status: version 2.4-test7, a development build.** The offline checks pass, but in-game acceptance is pending: visual
> behaviour and compatibility in a real load order have not been validated. Read [`docs/STATUS.md`](docs/STATUS.md).
> Keep it as a separate mod so it can be removed easily.

## Features

- **Inventory and Magic menus** enabled by default; **Barter** and non-actor **Container** menus can be turned on in
  the INI. Combat and unsupported camera states suspend the preview.
- The camera orbits the player and **restores** the previous perspective, field of view, projection planes and held
  controls when the menu closes. It follows both the `I` key and the `Tab` > Inventory route.
- **Right-side portrait** by default (`fScreenX`), done by shifting the view projection rather than placing the camera
  beside the player, so the character stays seen from the front.
- **Orbit, pan, pitch, zoom and field of view** with the mouse or a controller. Framing is remembered, or reset on
  opening.
- Optional: hide the native 3D item previews, remove the menu blur, keep the player's idle animation playing in paused
  menus, a neutral facial expression, face the camera.
- **Optional live settings panel** hosted by the KHA-OS hub; without it, edit the INI and press `F10` to reload.

## Controls

Begin a drag on the right of the screen, outside the leftmost 45% where the item list is.

| Input | Action |
|---|---|
| Right mouse drag | Orbit the character |
| Middle mouse drag | Pan the character on screen |
| `Ctrl` + middle drag | Change the camera pitch |
| `Ctrl` + mouse wheel | Zoom |
| `Ctrl` + `Shift` + mouse wheel | Change the field of view |
| Hold `RB` + right stick | Orbit and zoom (controller) |
| Hold `RB` + left stick | Pan (controller) |
| Hold `RB` + triggers | Change the field of view (controller) |
| `F10` | Reload the INI |

The mouse button and the controller button are configurable (`iRotateButton`, `iControllerButton`).

## Configuration

`Data\SKSE\Plugins\ShowPlayerInInventoryLE.ini`. The main settings:

| Setting | Meaning |
|---|---|
| `bEnable`, `bMagicMenu`, `bBarterMenu`, `bContainerMenu` | Which menus show the player |
| `bHideItemPreviews`, `bClearBlur` | Hide the native item previews, remove the menu blur |
| `bAnimateWhilePaused`, `bNeutralFace` | Keep the idle animation playing, use a neutral face |
| `bLockControls` | Stop the character walking or looking around while the menu is open |
| `bAutoFrame`, `bFaceCamera`, `bResetOrbitOnOpen`, `bRememberCamera` | Framing behaviour |
| `fScreenX`, `fScreenY`, `fDistance`, `fHeight`, `fPitch`, `fYaw`, `fFOV` | Framing values |
| `bEnableLogging` | Write `ShowPlayerInInventoryLE.log` (also enables per-session timing summaries) |

Dot and comma decimals are accepted; saved values use dots.

## Requirements

- Skyrim Legendary Edition **1.9.32.0.8** and [SKSE](https://skse.silverlock.org/) **1.7.3**
- Optional: KHA-OS, for the live settings panel

Do **not** install the Special Edition DLL of the original: only this one. The mod is not compatible with SE, AE or VR.

## Installation

The plugin is built from this repository by [GitHub Actions](.github/workflows/build.yml); each run attaches the files to
install (`SKSE\Plugins\ShowPlayerInInventoryLE.dll` and the INI) as an artifact. Install them with a mod manager, or
extract them into your Skyrim `Data` folder, and start the game through SKSE.

## Differences from the Special Edition mod

The SE camera framework and SE integrations were replaced by a native LE backend. The SmoothCam and FLICK APIs and the
SE weapon-attachment refresh are not included, and headtracking targets are not forcibly cleared. Physics follows the
installed LE provider: the plugin does not unpause the world or force a Havok or cloth step. Paused animation and the
neutral face are skipped for recognised Skyrim Souls DLLs; other custom unpause plugins may need those two options
turned off.

## Building

Requirements: Windows, Visual Studio 2022 C++ tools (v143, Win32), the Windows SDK and Python 3. Nothing is downloaded.

```bat
python build.py            :: build\ShowPlayerInInventoryLE.dll
python build.py --tests    :: the offline harness
python run_harness.py
```

See [`docs/BUILDING.md`](docs/BUILDING.md) for the details and the mutation tests.

## Repository layout

| Path | Content |
|---|---|
| `src/` | The plugin: `Engine.cpp` (camera, menus, animation, projection), `UI.cpp` (the KHA-OS panel), `Settings.cpp`, `Signatures.h` and `Verify.cpp` (the start-up address check) |
| `tests/Harness.cpp` | The offline fixture: a fake game in an owned child process |
| `ThirdParty/imgui/` | Dear ImGui, verbatim |
| `sdk/` | The SKSE LE SDK sources the build uses, with their licences |
| `upstream/` | An unmodified copy of the original source, see [`upstream/README.md`](upstream/README.md) |
| `dist/` | Files shipped next to the DLL (the INI) |
| `docs/` | Status, building, the original port guide and the recorded test-7 build inputs |

## Licence

Show Player In Inventory LE is a modified version of the original and keeps its licence, the **[GPL-3.0](LICENSE)**.
Third-party components keep their own licences, see [`NOTICE.md`](NOTICE.md).

## Credits

- **Ivy** (ItzIvy05): [Show Player In Inventory](https://github.com/ItzIvy05/Show-Player-In-Inventory), the mod this is
  ported from.
- **Mhacks79**: the Legendary Edition port.
- **JPSteel2**, **myztikrice**, **mwilsnd** and **Fuzzlesz**: credited by the original, see [`NOTICE.md`](NOTICE.md).
- **Ian Patterson, Stephen Abel and the SKSE team**: SKSE. **Omar Cornut and contributors**: Dear ImGui (MIT).

This is an independent community port. It is not affiliated with or endorsed by Ivy.

<div align="center">

[![Support me on Ko-fi](https://storage.ko-fi.com/cdn/brandasset/v2/support_me_on_kofi_blue.png)](https://ko-fi.com/mhacks79)

</div>
