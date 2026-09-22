# Building and running on macOS (Apple Silicon)

First brought up 2026-09-22 on an M1 / 16 GB, macOS 26, Xcode 26.6, UE 5.8.3 from the Epic Games
Launcher. The Windows notes ([[build-and-run]]) still describe the main machine; this is what is
different, in the order a fresh Mac hits it.

## Setup, once

1. **git-lfs before anything else.** Every `.umap`/`.uasset` is an LFS pointer; without it the
   checkout has 130-byte stubs and the editor cannot open the project.
   `brew install git-lfs && git lfs install --local && git lfs pull`.
2. **Original game data** into `Reference/SimCopterOriginalGame/` - the retail CD's `SIMCOPTER`
   folder copied as-is works (`bmp`, `cities`, `geo`, `sound`, `tweak`, `x`, `smk`, `help`).
3. **Metal toolchain.** Xcode 26 no longer ships the `metal` compiler; the engine dies on its first
   shader with a dialog naming `xcodebuild -downloadComponent MetalToolchain`. If that command fails
   to load a plug-in, run `xcodebuild -runFirstLaunch` first (no admin needed). The toolchain is a
   cryptex that takes a few seconds to mount after the download reports done.

## Building

`./RebuildUnrealCpp.sh` is the Mac twin of `RebuildUnrealCpp.bat` (`./RebuildUnrealCpp.sh Shipping`
for the game target). No Live Coding on Mac, so no `-NoLiveCoding`.

**Clang is stricter than MSVC**, and UBT builds with `-Werror`. The warnings that broke the first
Mac build, all of which MSVC accepts silently:
- `-Wunreachable-code-loop-increment` - a `for (TActorIterator<T> It(World); It; ++It) { return *It; }`
  "first actor" loop. Write `if (TActorIterator<T> It(World); It)`.
- `-Wbitwise-op-parentheses` - `Mask & 7 ^ Shf`. Bracket the `&`; precedence is unchanged.
- `-Wundef` - a bare `#if WITH_DLSS` when nothing defines it. The NVIDIA plugins are allow-listed
  to Win64 in the `.uproject` (they do not exist for Mac and the editor refuses to open the project
  otherwise), so `Build.cs` defines `WITH_DLSS=0` / `WITH_STREAMLINE=0` off Windows.

## Running

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
    "$PWD/SimCopterRemake/SimCopterRemake.uproject" -game -windowed -ResX=1600 -ResY=900 -log
```

The log is `~/Library/Logs/SimCopterRemake/SimCopterRemake.log`, not stdout and not `Saved/Logs`.
The first launch sits on a black, beachballing window for minutes: XProtect scans every engine
dylib on first load and Spotlight indexes the fresh engine. That is one-off.

The `LogPython: Error ... module 'unreal' has no attribute 'ToolsetDefinition'` tracebacks at
startup are the engine's experimental AI toolset plugins (`AllToolsets`) expecting editor-only
classes under `-game`. Harmless.

## Baking the gitignored generated content

The four movie/loading-screen bakes are plain Python + ffmpeg (`brew install ffmpeg`):
`BakeIntroMovies.py`, `BakeLoadingScreen.py`, `BakeMenuSky.py`, `BakeCareerPreviews.py`. The Game
target's `Build.cs` refuses to build without the intro movies and `HRGLASS.png`.

`BakeCityAtlas.py` needs the editor. There is no `UnrealEditor-Cmd` on Mac; use the commandlet:

```sh
UnrealEditor <uproject> -run=pythonscript -script="$PWD/Tools/Unreal/BakeCityAtlas.py" -unattended -nop4 -nosplash -stdout
```

It exits 1 because it logs a `LoadAsset failed` per asset it is about to create; the real verdict is
the `CITY ATLAS BAKE DONE` line. Do not run `CreateSimCopterMaterials.py` first unless the parent
materials changed - they are committed.

## Packaging (Shipping)

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
    -project="$PWD/SimCopterRemake/SimCopterRemake.uproject" -platform=Mac -clientconfig=Shipping \
    -build -cook -stage -pak -package -archive -archivedirectory=<out> -nop4 -unattended
```

- **`-package` is required on Mac.** Without it the archive step copies only the executable `.app`
  from `Binaries/` and leaves the paks, shaders and `SimCopter` data behind in `Saved/StagedBuilds`.
  With it everything lands in `<App>.app/Contents/UE/`, `SimCopter/` included, so the
  `../SimCopter` rule from [[simcopter-packaged-build]] still resolves.
- **Cook both Metal shader platforms.** An M1 runs the renderer at `METAL_SM5`; a cook that targets
  only `SF_METAL_SM6` launches straight into "shader platform SF_METAL_SM5 was not cooked".
  `DefaultEngine.ini`'s `MacTargetSettings` keeps both.
- Shipping writes no log and has no crash reporter. For a crash, run the binary under
  `lldb --batch -o run -k "thread backtrace all"`.
