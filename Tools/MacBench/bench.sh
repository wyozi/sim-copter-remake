#!/bin/bash
# Usage: [PROFILE=epic|high|medium|lowpreset|low] [VIEW=air|tour] [RESX=1280 RESY=720] [FRAMES=1200]
#        [EXEC="cvar v,cvar v"] [TOPGPU=N] [GTSTATS=N] bench.sh <label> [extra game args]
#
# Boots a packaged Mac *Development* build (CSV profiling is compiled out of Shipping) straight into
# career city 0 (Sea Cliff) with -SimCopterCareerCity, boards the helicopter and parks it on the
# hospital roof (VIEW=air) or hops between hospital/police/fire every 3 s (VIEW=tour), captures
# FRAMES frames with the CSV profiler, exits, and summarises the steady-state tail. PROFILE pins the
# saved settings: 'low' is Low Power Graphics, the rest are the scalability presets with it off.
# The player's own GameUserSettings.ini is backed up on first use and restored by `bench.sh --restore`.
# See Docs/memory/mac-performance.md.
set -u
LABEL="$1"; shift
HERE="$(cd "$(dirname "$0")" && pwd)"
OUTDIR="${OUTDIR:-$HERE/../../Docs/scratchpad/mac-bench}"
APP="${APP:-$HOME/Documents/SimCopterBuilds/Mac-Development/SimCopterRemake.app}"
BIN="$(ls "$APP"/Contents/MacOS/* | head -1)"
CONTAINER="$HOME/Library/Containers/com.YourCompany.SimCopterRemake/Data/Library"
CFG="$CONTAINER/Application Support/Epic/SimCopterRemake/Saved/Config/Mac/GameUserSettings.ini"
CSVDIR="$CONTAINER/Application Support/Epic/SimCopterRemake/Saved/Profiling/CSV"
BACKUP="$OUTDIR/GameUserSettings.user-backup.ini"
mkdir -p "$OUTDIR/runs"
if [ "$LABEL" = "--restore" ]; then cp "$BACKUP" "$CFG" && echo "restored $CFG"; exit $?; fi
[ -f "$BACKUP" ] || cp "$CFG" "$BACKUP"
python3 "$HERE/profile.py" "$BACKUP" "$CFG" "${PROFILE:-epic}"
BEFORE="$(ls -t "$CSVDIR"/*.csv 2>/dev/null | head -1)"
"$BIN" /Game/CityRender -SimCopterCareerCity=0 -windowed -ResX="${RESX:-1280}" -ResY="${RESY:-720}" -nosound -nomovie -unattended \
  "-SimCopterBenchCmds=${BENCHCMDS:-$( [ "${VIEW:-air}" = tour ] && echo "@8:SimBoardHelicopter;@11:SimGotoBuilding 209;@16:csvprofile exitoncompletion;@16:csvprofile frames=${FRAMES:-1200};@19:SimGotoBuilding 210;@22:SimGotoBuilding 211;@25:SimGotoBuilding 209;@28:SimGotoBuilding 210;@31:SimGotoBuilding 211;@34:SimGotoBuilding 209;@37:SimGotoBuilding 210;@40:SimGotoBuilding 211;@43:HighResShot 1" || echo "@8:SimBoardHelicopter;@11:SimGotoBuilding 209;@16:csvprofile exitoncompletion;@14:HighResShot 1;@16:csvprofile frames=${FRAMES:-1200}")}" \
  -csvGpuStats \
  -ExecCmds="t.MaxFPS 0,r.VSync 0${EXEC:+,$EXEC}" "$@" > "$OUTDIR/runs/$LABEL.stdout" 2>&1 &
PID=$!
for _ in $(seq 1 300); do kill -0 $PID 2>/dev/null || break; sleep 1; done
kill -0 $PID 2>/dev/null && { kill $PID; echo "$LABEL: TIMED OUT"; }
AFTER="$(ls -t "$CSVDIR"/*.csv 2>/dev/null | head -1)"
if [ -z "$AFTER" ] || [ "$AFTER" = "$BEFORE" ]; then echo "$LABEL: no new CSV"; exit 1; fi
cp "$AFTER" "$OUTDIR/runs/$LABEL.csv"
cp "$CONTAINER/Logs/SimCopterRemake/SimCopterRemake.log" "$OUTDIR/runs/$LABEL.log" 2>/dev/null
SHOTDIR="$CONTAINER/Application Support/Epic/SimCopterRemake/Saved/Screenshots/Mac"
SHOTFILE="$(ls -t "$SHOTDIR"/*.png 2>/dev/null | head -1)"
[ -n "$SHOTFILE" ] && sips -Z 960 "$SHOTFILE" --out "$OUTDIR/runs/$LABEL.png" >/dev/null && rm -f "$SHOTDIR"/*.png
python3 "$HERE/summarize.py" "$OUTDIR/runs/$LABEL.csv" "$LABEL"
