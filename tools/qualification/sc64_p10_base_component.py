#!/usr/bin/env python3
"""Reproduce the P7 base-terrain component check from the pinned P10 ROM."""
import collections
import hashlib
import pathlib
import sys

rom = pathlib.Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(rom).hexdigest() == 'ff94a2ea9f98404c97f44a8b6b7bf557b3572c761de530d867032eeb8f76b10b'
# sc64_race_p7_blocked in the pinned release ELF: 0x09b4c5ec in the ROM window.
grid = rom[0x1b4c5ec:0x1b505ec]
assert len(grid) == 16384
assert hashlib.sha256(grid).hexdigest() == 'e067cce13f78899a185006aad500d9846dccf682cc1dd36e78679fcaf810da5f'

def neighbours(cell):
    x, y = cell % 128, cell // 128
    return [nx + ny * 128 for nx, ny in ((x-1,y),(x+1,y),(x,y-1),(x,y+1))
            if 0 <= nx < 128 and 0 <= ny < 128]

source, goal = 1044, 10270
assert not grid[goal] & 1
queue = collections.deque([goal])
distance = {goal: 0}
while queue:
    cell = queue.popleft()
    for n in neighbours(cell):
        if not (grid[n] & 1) and n not in distance:
            distance[n] = distance[cell] + 1
            queue.append(n)
for name, cell in [('source', source), ('goal', goal)]:
    print(name, (cell % 128, cell // 128), 'blocked', grid[cell] & 1,
          'neighbours', [(n % 128, n // 128, grid[n] & 1) for n in neighbours(cell)])
print('goal component', len(distance), 'source distance', distance.get(source))
assert len(distance) == 12723 and distance.get(source) == 210
