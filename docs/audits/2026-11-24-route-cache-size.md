# Route cache size no longer changes play

The route pool holds 14 cached shortest-path fields (6,144 bytes each on a
128×128 map, 86,016 bytes in all). It is the largest block of work RAM, so a
smaller pool is the most direct way to make room for larger unit tables in
every ROM. Until now its size was not a free choice, because weapon-range
movement read cache residency in two ways
([2026-11-10 profile](2026-11-10-busy-combat-profile/README.md)):

1. `sc64_range_cached_point_step` used a point field only if one happened to
   be cached.
2. The completed range snapshot lives in an ordinary cache slot. Point and
   depot misses could evict it as the oldest field, so how long it survived,
   and whether a unit kept its range guidance, followed the cache size.

This change removes the cache-only read (and the far-read proof that served
only it) and pins the snapshot: ordinary misses skip its slot, as they already
skipped a copy in progress, and it is released only by the range worker
itself (a replacement, a retired target, an idle expiry or a reset). Every
remaining route answer is exact, so the field count only trades memory
against flood work. A build may now set `SC64_ROUTE_FIXED_SLOTS` (6 to 56);
the default stays 14 and no ROM changes its size here.

All cycle counts are mGBA guest cycles, not hardware measurements.

## Size independence

HOST builds ran 54 custom games for up to 30,000 updates: three canonical
profiles, both maps, every human race and one to three opponents, with the
human planned by the opponent planner (`--mirror`). Each run wrote a digest
of the world and of every live unit's movement plan after every update.

Before this change (main 35781d0f):

| Build | Games identical to 14 fields | Results changed | Route floods |
| --- | ---: | ---: | ---: |
| 12 fields | 44/54 | 1 | +0.2% |
| 10 fields | 37/54 | 5 | +16.5% |
| 8 fields | 30/54 | 9 | +144% |
| 6 fields | 24/54 | 9 | +622% |

With this change (on main a9afd778), 12 and 8 fields played all 54 games
identically to 14. The change itself alters play once: 35 of the 54 games
differ from main and 9 results change. Among the Protoss P3–P10 controllers
under P10 flags, all eight still win and the P8 and P10 runs differ from main;
12 fields gives byte-identical controller traces to 14 in all eight.
`make test-sc64-route-cache-size` checks six of these games at 12 and 14
fields, update by update through the first offensives.

## Cost on the GBA build

P9 (Shadow Hunters, 128×128) floods the most of any Protoss mission at 12
fields (1,244 floods at 14, 2,717 at 12 on HOST before this change). Its
281,405-update controller tape plays the same game on main and with this
change, so one tape drives every build below.

| P9 replay ROM | Loop cycles | Update cycles | Peak update |
| --- | ---: | ---: | ---: |
| Main a9afd778 | 396,831,064,359 | 226,124,819,025 | 37,367,793 |
| This change, 14 fields | +0.006% | +0.011% | 37,355,004 |
| This change, 12 fields | +0.257% | +0.455% | 37,359,209 |

All three builds match at the 282 periodic checkpoints and win at the same
frame. The change costs about 89 cycles per update here. Twelve fields would
free 12 KiB for about 3,570 more cycles per update than 14; the slowest update
does not move. The custom-game battle windows were not re-measured on ARM,
because the private custom tapes no longer replay on current main.

## The 32 KiB flood scratch is not spare

The `bfs_queue` area is the linear flood's working memory on 96- and
128-column maps: a 6,144-entry ring (12 KiB), a 17.7 KiB byte-per-cell grid
and the 2 KiB retained walkability. In about 32,000 measured floods the ring
never overflowed and the classic fallback, which can use the whole area as a
plain queue, never ran. Shrinking it needs a different flood design, not a
smaller constant.
