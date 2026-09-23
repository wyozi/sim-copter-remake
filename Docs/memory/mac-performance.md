# Mac performance — the GPU is the whole story, and the clouds are most of it

*Measured 2026-09-23 on an Apple M1 (8-core GPU, 16 GB), macOS 26, UE 5.8.3, packaged Mac
Development build, Sea Cliff (career city 0), 1280x720 windowed. Harness: `Tools/MacBench/bench.sh`.*

See [[mac-build-and-run]] for getting a Mac build at all, [[simcopter-low-power-mode]] for what Low
Power Graphics switches.

## How to measure

`Tools/MacBench/bench.sh <label>` boots the packaged Development app with
`/Game/CityRender -SimCopterCareerCity=0`, runs `-SimCopterBenchCmds` to board the helicopter and park
it on the hospital roof overlooking the skyline, starts `csvprofile` 16 s in, screenshots at 14 s,
and summarises the last 600 frames. `PROFILE=epic|high|medium|lowpreset|low` pins the saved settings
(the player's file is backed up and put back with `bench.sh --restore`); `EXEC="cvar v,..."` layers
CVars on top; `VIEW=tour` hops hospital/police/fire every 3 s for game-thread work.

Traps found building it — each cost a run:

- **Shipping has no CSV profiler.** Package `-clientconfig=Development`.
- **The packaged app is sandboxed**: logs, CSVs, screenshots and `GameUserSettings.ini` are under
  `~/Library/Containers/com.YourCompany.SimCopterRemake/Data/Library/...`, shared by the Shipping and
  Development apps — a player's Low Power toggle in one is live in the other.
- **Opening `/Game/CityRender` directly used to deadlock** in `FEngineLoop::Init` →
  `WaitForMovieToFinish`: the loading screen is manual-stop and was only stopped from a later frame.
  `USimCopterLoadingSubsystem::Show` now uses the viewport widget until `GIsRunning`.
- **Without `-SimCopterCareerCity` the level loads its authored city**, a developer's absolute
  `S:/...` path. The switch lives in `USimCopterSessionSubsystem::Initialize` because the city
  actor reads the session in its own BeginPlay, before the game mode's.
- **`-ExecCmds` runs on frame one**, before the pawn exists; hence `-SimCopterBenchCmds="@t:cmd;..."`.
- **Per-pass GPU stats lie on Apple's TBDR GPU.** Cutting `r.VolumetricCloud.ViewRaySampleMaxCount`
  moved `GPU/VolumetricCloudShadow` from 27 to 8 ms and the frame not at all. Judge by `FrameTime`.
- `screencapture` from the shell only sees the wallpaper without Screen Recording permission; the
  harness uses the game's own `HighResShot`.

## Results (median frame ms, aerial view; run-to-run noise < 1 ms)

| Setup | ms | Notes |
| --- | --- | --- |
| Epic, no Mac profile | 101.7 | GPU 105; p95 ~290 |
| High | 87.2 | |
| Medium | 78.3 | clouds alone keep every preset above 40 ms |
| Epic + `r.VolumetricCloud.ShadowMap 0` | 80.7 | no visible difference in the screenshots |
| … + `r.TSR.History.ScreenPercentage 100` | 76.3 | **what the Mac device profile now ships: 75.3** |
| … + `r.Shadow.Virtual.Enable 0` | 77.0 | VSM costs nothing measurable here — leave it |
| Epic + `r.VolumetricCloud 0` | 58.2 | the clouds are ~44 ms at Epic |
| Low Power Graphics | **9.8** | p95 11.3 — no stalls |
| Low Power, 100% screen percentage | 13.7 | sharper, still ~73 fps |
| Low Power + Lumen GI | 17.4 | no visible gain from this view |
| Low Power + clouds (no cloud shadow) | 28.9 | the clouds are ~19 ms even at Low |

The volumetric cloud layer (CelestialVault's `m_SimpleVolumetricCloud_TOD`) is the single biggest
cost on Apple GPUs. `ProfileGPU` puts it in one pass, `CloudView (PS) 240x135` - 18 ms for 32k
pixels, so it is samples per ray, not resolution. **The engine's `r.VolumetricCloud.*SampleMaxCount`
CVars do nothing here**: they only clamp, and the component (a 10 km layer at 5 km, traced 50 km,
sample scales 1.0, a 282-node material with 3D Perlin-Worley and lightning volumes) asks for fewer.
The component's own **`ViewSampleCountScale` and `ShadowViewSampleCountScale`** are the levers.

### Clouds, re-enabled on Mac (2026-09-23)

Measured with a fixed camera looking at the sky over the skyline (`SimBenchView 3560 -10990 6000
12 60`), Low Power + clouds, GPU median:

| Cloud setup | GPU ms | clouds cost |
| --- | --- | --- |
| none | 12.6 | - |
| authored (scales 1.0) | 54.9 | ~42 |
| scales 0.5, TracingMaxDistance 20 km | 23.0 | ~10 |
| scales 0.25, TracingMaxDistance 20 km | 15.6 | ~3 - loses the horizon clouds |
| **scales 0.25, authored 50 km** | **16.3** | **~3.6 - indistinguishable overhead and at the horizon** |

`LayerHeight` and `StopTracingTransmittanceThreshold` moved nothing. With the clouds on, the
real-time sky capture re-rendering them gave a p95 of ~46 ms; the capture CVars in the Mac profile
bring it back to the no-cloud ~27 ms.

Shipped: `SimCopter.Clouds.ViewSampleCountScale` / `ShadowViewSampleCountScale` (`SimCopterCloudTuning`)
at 0.25 and `SimCopter.LowPower.KeepVolumetricClouds=1` in the Mac device profile, so Low Power
Graphics keeps the sky on Mac. **Trap:** applying the scales once at the city's BeginPlay did not
stick - the cloud layer is re-initialised after it - so the city actor re-applies them once a
second, touching the component only when a value differs. `SimCloudSet <Property> <Value>` sets a
cloud property from the console for more measurements.

**Benchmark hygiene:** check `ps -Ao %cpu,comm | sort -rn | head` before trusting a run. One batch
here ran beside an 8-core `ffmpeg` transcode and every game-thread stat came out 2.5-3x slow. The
sky-view cloud table above may overlap it too; its rows were taken back to back, so they compare
with each other, but re-measure before quoting them as absolutes.

The Epic p95 stalls (~190 ms, a quarter of frames) show no pass that grows on stall frames; they are
the GPU running at 10 fps with frames queued behind each other, and vanish at Low.

**The game thread is not the problem on Mac**: ~4.8 ms median at Low, ~8 ms at Epic, one frame over
16.7 ms in 2400 of the tour. The CSV scopes `SimCopterTraffic_*`, `SimCopterCity_*`,
`SimCopterGroundAgent_Tick`, `SimCopterMissions_Tick` show where it goes (agents 1.6 ms, traffic
interactions 0.5, missions 0.4).

## What shipped

- `Config/DefaultDeviceProfiles.ini` `[Mac DeviceProfile]`: cloud shadow map off, TSR history 100%.
  Epic 101.7 → 75.3 ms. Windows untouched.
- `Config/Mac/MacGameUserSettings.ini`: Low Power Graphics defaults ON for a Mac first run.
- Clouds re-enabled on Mac, including in Low Power Graphics, at ~3.6 ms (section above).
- `FMaxisMeshLibrary::GetShared`: vehicles no longer re-read and re-parse the three sim3d*.max files
  per spawn. Correct but not visible in these benchmarks — the tour triggers few respawns.

## Not done, and why

- **`r.Shaders.BoundsChecking` / `ZeroInitialise`** are on in the engine's Mac profile and cost GPU
  time, but Epic sets them to stop Metal GPU faults, and this project already has two Metal crashes
  on record. Needs a recook to test.
- **Low Power at 100% resolution on Mac** (13.7 ms) is a sharper default within budget, but it is a
  look/feel call for the owner, not a measurement.
- The review's other game-thread items — flashing-light cards rebuilt on every camera move, smoke /
  fire / particle sections recreated per frame, O(n²) traffic passes — are real but ~1-2 ms on
  this machine. Worth doing for Windows high-refresh; not what makes the Mac slow.
