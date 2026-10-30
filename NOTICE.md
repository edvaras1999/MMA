# Notices and licences

## Show Player In Inventory LE

Show Player In Inventory LE is a port to Skyrim **Legendary Edition** of **Show Player In Inventory** by **Ivy**
(ItzIvy05) (<https://github.com/ItzIvy05/Show-Player-In-Inventory>, Skyrim Special Edition), and is a **modified
version** of that work.

It is distributed under the same licence as the original, the **GNU General Public License, version 3**
([`LICENSE`](LICENSE), copied unchanged from the original repository).

- The idea, the behaviour, the controls and the settings are those of the original.
- The Legendary Edition port is by **Mhacks79**: a native x86 backend for `TESV.exe` 1.9.32.0.8 and SKSE 1.7.3 that
  replaces the Special Edition engine layer, the optional KHA-OS settings panel, the offline test harness and the
  tests.
- This is a modified version of the original work. The changes are the whole of this repository's history
  (`git log`) and are described in [`CHANGELOG.md`](CHANGELOG.md).
- The complete corresponding source of the plugin is in this repository, and an unmodified copy of the original
  source is in [`upstream/`](upstream/Show-Player-In-Inventory/).

Show Player In Inventory LE is an independent community port. It is not affiliated with or endorsed by Ivy.

## Credits kept from the original

The original lists the following credits, which this port keeps:

- **JPSteel2**, for **Character Menu SE**, on whose camera code the original is based.
- **myztikrice**, for **Show Player In Menus**, which inspired Character Menu SE and so, indirectly, the original.
- **mwilsnd**, for **SmoothCam** and its API.
- **Fuzzlesz**, for **FLICK** and **FUCK-RACE**, whose per-frame menu camera and controls the original's camera is
  built on. (The Legendary Edition port does not use FLICK or the SmoothCam API.)

## Third-party code included

| Component | Where | Licence |
|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) (core, DirectX 9 and Win32 backends) | `ThirdParty/imgui/` | MIT, [`licenses/DearImGui-MIT.txt`](licenses/DearImGui-MIT.txt) |
| SKSE `skse/` sources (Ian Patterson, Stephen Abel and contributors) | `sdk/skse/skse/` | The notice in [`licenses/SKSE.txt`](licenses/SKSE.txt): the Skyrim Online team is explicitly disallowed from using these files for any purpose. Keep it with any redistribution |
| SKSE `common` library (Ian Patterson) | `sdk/common/` | zlib-style, [`licenses/Common.txt`](licenses/Common.txt): altered versions must be plainly marked, and the notice may not be removed |
| An SKSE LE SDK mirror, pre-configured to build plugins with Visual Studio 2022 | `sdk/config/`, `sdk/README.md`, `sdk/LICENSE` | Its own README and licence note are kept as supplied: "The license is set by Ian Patterson (ianpatt)" |
| KHA-OS plugin interface header (`KhaosAPI.h`) | `src/KhaosAPI.h` | An interface definition published by the KHA-OS developers for plugins that integrate with the hub; it remains under its authors' terms and is not covered by this repository's GPL grant |

## Not included

- `TESV.exe` and every other file of the game. They are never committed (see `.gitignore`).
- **KHA-OS** itself (`KHAOS.dll`), an optional separate mod. The plugin works without it, from the INI.
- Prebuilt binaries. The plugin is built from source by the workflow in `.github/workflows/build.yml`.
