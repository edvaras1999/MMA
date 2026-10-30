# Building and testing

Requirements: Windows, Visual Studio 2022 C++ tools (v143, Win32), the Windows SDK and Python 3. Nothing is downloaded:
the SKSE LE SDK sources are in `sdk/` and Dear ImGui is in `ThirdParty/imgui/`.

Run these from a Visual Studio developer prompt, in the repository root. If MSBuild is not on `PATH`, pass
`--msbuild <path to MSBuild.exe>` (or set `MSBUILD_EXE`). A different equivalent LE SDK can be selected with
`--sdk <path>` or `SKSE_LE_SDK`.

```bat
python build.py                          :: the plugin: build\ShowPlayerInInventoryLE.dll
python build.py --tests                  :: the offline fixture: build\tests\SPIIHarness.exe
python run_harness.py                    :: run it under its native supervisor (45 s limit)
python run_harness.py --prove-timeout    :: prove that a deliberately stalled child is terminated
python mutation_tests.py --sdk sdk --msbuild MSBuild.exe
```

- The build compiles the SDK `common` implementation directly: no `common_vc9.lib` or prebuilt library is needed.
- Each build writes `build-inputs.json` next to its output, binding the binary to the SHA-256 of every source file.
  `docs/production-build-inputs.json` is the one recorded for the test-7 package.
- The fixture reserves fake engine addresses in an owned, suspended child process. It never loads Skyrim and does not
  modify an installed game. `run_harness.py` refuses a harness older than the sources ("stale").
- `mutation_tests.py` compiles 78 separately mutated copies of the sources and checks that the diagnostic assertions
  reject each one. It takes a long time, so CI runs it only on demand.

## Porting notes

`docs/SOURCE-GUIDE.txt` is the original source guide of the test-7 package: build notes, the engine facts the port relies
on (camera constructor, `SetNode` and `Update`, menu advance and destructors, blur counter, input dispatcher, player
animation slot) and the reasoning behind each test fix. `docs/PACKAGE-README.txt` is the package's README with the
per-test change notes and the validation record.
