#!/usr/bin/env python3
"""Summarise a UE CSV profile: medians/p95 over the steady-state tail (last TAIL frames)."""
import csv, os, statistics, sys
path, label = sys.argv[1], (sys.argv[2] if len(sys.argv) > 2 else "")
tail = int(os.environ.get("TAIL", "600"))
rows = list(csv.reader(open(path, newline="")))
# With continuous writes the header row is written LAST, just before the metadata row, and rows
# grow as new stats appear mid-capture; pad short rows.
if rows[-1] and rows[-1][0] == "[HasHeaderRowAtEnd]":
    header, body = rows[-2], rows[1:-2]
else:
    header, body = rows[0], rows[1:-1]
data = [r + [""] * (len(header) - len(r)) for r in body if r and r[0] != "EVENTS"]
def num(r, i):
    try: return float(r[i])
    except ValueError: return None
data = data[-tail:]
def col(name):
    if name not in header: return None
    i = header.index(name); v = [x for x in (num(r, i) for r in data) if x is not None]
    return v or None
def fmt(name):
    v = col(name)
    if not v: return f"{name}=n/a"
    v.sort(); return f"{name} med {statistics.median(v):6.2f} p95 {v[int(len(v)*0.95)-1]:6.2f}"
keys = ["FrameTime", "GameThreadTime", "RenderThreadTime", "RHIThreadTime", "GPUTime"]
print(f"{label:28s} frames={len(data)}  " + " | ".join(fmt(k) for k in keys))
if os.environ.get("TOPGPU"):
    gpu = [h for h in header if h.startswith("GPU/")]
    stats = sorted(((statistics.median(col(h) or [0]), h) for h in gpu), reverse=True)[:int(os.environ["TOPGPU"])]
    for ms, h in stats: print(f"    {h:48s} {ms:6.2f} ms")
if os.environ.get("GTSTATS"):
    names = [h for h in header if h.startswith("Exclusive/GameThread/")]
    out = []
    for h in names:
        v = col(h)
        if v: out.append((statistics.mean(v), max(v), sum(1 for x in v if x > 5.0), h))
    for m, mx, n5, h in sorted(out, reverse=True)[:int(os.environ["GTSTATS"])]:
        print(f"    {h[len('Exclusive/GameThread/'):]:44s} mean {m:6.2f}  max {mx:7.2f}  frames>5ms {n5}")
    ft = col("GameThreadTime"); print(f"    GameThreadTime frames>16.7ms: {sum(1 for x in ft if x > 16.7)}  >33ms: {sum(1 for x in ft if x > 33)}")
