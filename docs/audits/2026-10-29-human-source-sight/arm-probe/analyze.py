#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize paired probe logs.

Usage: analyze.py <runs-dir> t8 [t7 ...], with <runs-dir>/pr-<t> and
<runs-dir>/base-<t>. The shared prefix is the leading run of 500-tick windows
whose gameplay digest (units, buildings, resources, frame; no fog) matches.
"""
import json, sys
from pathlib import Path

HZ = 16_777_216

def parse(path):
    windows, total, end, hist, launch = [], None, None, None, None
    for line in path.read_text().splitlines():
        head, *rest = line.split(" ")
        kv = dict(p.split("=", 1) for p in rest if "=" in p)
        if head == "W":
            windows.append({k: (v if k in ("digest", "play") else int(v)) for k, v in kv.items()})
        elif head == "TOTAL":
            total = {k: int(v) for k, v in kv.items()}
        elif head == "END":
            end = kv
        elif head == "LAUNCH":
            launch = kv
        elif head == "SIGHTHIST":
            hist = [int(x) for x in rest]
    return windows, total, end, hist, launch

def stats(ws):
    n = sum(w["n"] for w in ws)
    up, sight, full = (sum(w[k] for w in ws) for k in ("up", "sight", "full"))
    return {"ticks": n, "mean_update": round(up / n), "mean_sight": round(sight / n), "mean_full": round(full / n),
            "peak_update": max(w["pk_up"] for w in ws), "peak_sight": max(w["pk_sight"] for w in ws),
            "peak_full": max(w["pk_full"] for w in ws),
            "sight_share_update_pct": round(100 * sight / up, 2), "sight_share_full_pct": round(100 * sight / full, 2),
            "effective_fps": round(n * HZ / full, 2)}

runs, report = Path(sys.argv[1]), {}
for t in sys.argv[2:]:
    pr, base = parse(runs / f"pr-{t}" / "probe.log"), parse(runs / f"base-{t}" / "probe.log")
    shared = 0
    for a, b in zip(pr[0], base[0]):
        if a.get("play", a["digest"]) != b.get("play", b["digest"]):
            break
        shared += 1
    r = {"divergence_after_tick": pr[0][shared - 1]["tick"] if shared else 0,
         "windows": [len(pr[0]), len(base[0])]}
    for name, (ws, total, end, hist, launch) in (("pr", pr), ("base", base)):
        r[name] = {"launch": launch, "end": end, "whole": stats(ws)}
        if shared:
            r[name]["shared_prefix"] = stats(ws[:shared])
    report[t] = r
print(json.dumps(report, indent=1))
