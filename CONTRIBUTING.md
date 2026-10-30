# Contributing

Thanks for helping keep Skyrim Legendary Edition alive.

## Reporting a bug

Open an [issue](https://github.com/Mhacks79/Show-Player-in-Inventory-LE/issues/new/choose) with your Skyrim LE and SKSE
versions, the plugin version, the steps to reproduce (which menu, first or third person, how it was opened), and whether
it still happens with other camera, UI, face and ENB mods disabled. Set `bEnableLogging=1` and attach
`Documents\My Games\Skyrim\SKSE\ShowPlayerInInventoryLE.log`.

## Code

- Win32 (x86), MSVC v143, C++17. Match the style of the surrounding code.
- Engine addresses are read from `TESV.exe` 1.9.32.0.8; never copy an address from a Special Edition source or the
  Address Library. A finding is welcome when it says how it was established (calling convention and how it was proved,
  the function it was read in, tested in game or only read from the binary).
- A new engine site needs a signature in `src/Signatures.h` so the plugin refuses to install on a different executable.
- Keep the fixture green and add a check with every behaviour change (`tests/Harness.cpp`); a new regression test should
  fail with the bug restored.
- Do not edit `upstream/`: it is an unmodified copy of the original. Never commit `TESV.exe` or any other game file.

## Licence of contributions

This project is a modified version of Show Player In Inventory by Ivy, under the **GPL-3.0** (see `LICENSE`). By
contributing you agree that your work is distributed under the same terms.
