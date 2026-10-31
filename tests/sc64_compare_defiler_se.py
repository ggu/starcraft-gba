#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare an actual-game SE cast approach to a pinned OpenBW world trace."""
import argparse
import re
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--actual',type=Path,required=True)
p.add_argument('--source',type=Path,required=True)
p.add_argument('--source-q8',type=Path)
p.add_argument('--samples',type=int,default=120)
a=p.parse_args()
actual={};source={}
for line in a.actual.read_text().splitlines():
    if m:=re.fullmatch(r't=(\d+) x=(-?\d+) y=(-?\d+) heading=(\d+) phase=(\d+) blocked=(\d+) qx=(-?\d+) qy=(-?\d+)',line):
        t,x,y,h,phase,blocked,qx,qy=map(int,m.groups());actual[t]=(x,y,h,phase,blocked,qx,qy)
for line in a.source.read_text().splitlines():
    if m:=re.match(r'sample t=(\d+).*? state=(\d+).*?heading=(-?\d+) xy=(-?\d+),(-?\d+)',line):
        t,state,h,x,y=map(int,m.groups());source[t]=(x,y,h%256,state)
for t in range(1,a.samples+1):
    assert t in actual and t in source,('missing',t)
    assert actual[t][:3]==source[t][:3] and not actual[t][4],(t,actual[t],source[t])
assert next(t for t in range(1,a.samples+1) if actual[t][3]) == next(t for t in range(1,a.samples+1) if source[t][3]==2)
if a.source_q8:
    exact={}
    for line in a.source_q8.read_text().splitlines():
        if m:=re.match(r'sample=(\d+) exact=(-?\d+),(-?\d+)',line):
            t,x,y=map(int,m.groups());exact[t]=(x,y)
    assert exact
    for t,xy in exact.items():
        if t<=a.samples:assert actual[t][5:7]==xy,(t,actual[t][5:7],xy)
print(f'{a.samples} source positions/headings and debit match'+(f'; {len(exact)} exact Q8 rows match' if a.source_q8 else ''))
