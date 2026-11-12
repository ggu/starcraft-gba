# Z10 battle cost and rectangular point-flood trial

The canonical dense point flood in [#389](https://github.com/ggu/starcraft-gba/pull/389)
helps 64×64 maps. This trial tested whether extending it to rectangular maps
would similarly help the final Zerg mission. The trial stays out of normal builds.

## Paired ARM replay

Both normal Z10 cartridges loaded the same 32 KiB SRAM image through the menu
and replayed the ordinary controller inputs from call 2,666 through 35,999.
No guest memory was written by the runner. At all 34 checkpoints the mission,
screen, game frame, result, wallet and fault count matched the previously
qualified replay; the raw unit, building and missile arrays also matched
between the two cartridges. Each measured window is 1,000 consecutive calls.
These are mGBA guest CPU cycles, not GBA SP frame rates.

| Workload | Mean loop cycles, base → trial | p95, base → trial | Max, base → trial |
| --- | ---: | ---: | ---: |
| Z10, calls 19,000–19,999 | 1,568,109 → 1,566,138 (−0.13%) | 1,965,123 → 1,965,796 | 4,176,639 → 4,168,119 |
| Z10, calls 35,000–35,999 | 1,995,868 → 1,995,619 (−0.01%) | 2,375,134 → 2,378,759 | 4,136,425 → 4,143,260 |
| Z10 ROM, 64×64 custom battle | 1,368,916 → 1,372,580 (+0.27%) | 1,726,162 → 1,734,801 | 3,321,546 → 3,358,420 |

The custom comparison uses the same physical input tape, runner and selected
window as the base. Its 1,000 frame/screen rows and 21 sampled actor-count rows
match. Z10's stock simulation means alone are 791,327 and 992,461 cycles in the two campaign
windows. Both exceed the entire 561,792-cycle budget for two GBA video frames,
so a rendering change alone cannot bring these battles to 30 fps.

The discarded trial reused `bfs_queue` scratch for a padded byte grid and a
bounded ring frontier. It passed the 321,445-query host route comparison,
including 21,704 resident-field checks; a forced small-frontier test also
verified that overflow handed the untouched result back to the original
packed flood. Z10 used 272 more bytes of IWRAM with unchanged EWRAM. The tiny
campaign improvement and regression on the established small-map fast path
do not justify spending that memory. No trial code ships.

Base ROM SHA-256: `bb286c9c4fe3c70c12b0dec7abb298ba24576f2213dd5dbf8fcb3b692dc70412`.
Trial ROM SHA-256: `e9cc3b579329a46fa4e51ef9929887fe66e731bbd3eaf5b57b9e8dea3600faba`.

## Where the stock build spends time

The instruction profiler ran the unchanged base cartridge with the same SRAM
and physical keys. All 34 gameplay checkpoints still matched. We sampled 12
advancing calls spaced 79 calls apart from the start of each window; in each
sample the instruction and call-stack totals equal the elapsed guest clock,
with no call-stack overflow. These smaller samples attribute cost; the full
windows above are the baseline for measuring a change.

| Scope, inclusive | Mean cycles, 19,000 window | Mean cycles, 35,000 window |
| --- | ---: | ---: |
| Entire loop | 1,498,723 | 1,989,495 |
| Render | 734,316 (49.0%) | 1,004,479 (50.5%) |
| Sprite blitter (within render) | 105,986 | 217,933 |
| Simulation | 724,263 (48.3%) | 937,425 (47.1%) |
| Orders (within simulation) | 311,446 | 385,295 |
| Defenses (within simulation) | 203,096 | 288,294 |
| Defense target search (within defenses) | 85,177 | 167,679 |
| Cannon Pylon checks (within defenses) | 79,060 | 79,078 |
| Exploration (within simulation) | 110,048 | 151,979 |
| Next-step route queries (within simulation) | 489 | 1,277 |

The listed scopes nest; their rows cannot be added. Only the defense pass
called `race_site_powered` in these samples. First compare a Pylon slot list
scoped to one defense pass against the scalar power check, retaining current
ownership and completion checks. For larger gains, measure defense target
search and unit/building sprite drawing separately. Keep the same target
eligibility and tie-breaks and compare both video pages and palettes before
promoting a renderer change. Requalify on campaign and both custom maps
before enabling a change for all applicable profiles.

## Compact Cannon power probe

The first experiment recorded live Pylon slots and reread each building for
every Cannon. It matched all 34 campaign checkpoints but raised mean loop
cycles by 3.37% and 2.56% in the two windows. The compiled lookup added slot
addressing and out-of-line geometry work to the inner loop. It was discarded.

The second experiment snapshots complete Pylon coordinates grouped by owner
once per defense pass, only when the first Cannon needs them. Production and
mission updates run before that pass; defenses attack units and cannot change
Pylon coordinates, completion or ownership during it. The probe still uses
the same psi geometry and checks power before advancing weapon cooldown. It
keeps the scalar path for out-of-range owners and requires the compact
building coordinate layout at compile time.

| Workload | Mean loop cycles, base → probe | Mean simulation cycles, base → probe | p95 loop, base → probe |
| --- | ---: | ---: | ---: |
| Z10, calls 19,000–19,999 | 1,568,109 → 1,535,741 (−2.06%) | 791,327 → 759,116 (−4.07%) | 1,965,123 → 1,935,035 |
| Z10, calls 35,000–35,999 | 1,995,868 → 1,962,519 (−1.67%) | 992,461 → 959,268 (−3.34%) | 2,375,134 → 2,342,683 |
| Z10 ROM, 64×64 custom battle | 1,368,916 → 1,369,291 (+0.03%) | — | 1,726,162 → 1,726,001 |

The 34 campaign checkpoints, including all unit, building and mineral bytes,
match. The same 1,000-call custom window matches the base's frame/screen rows
and 21 sampled actor-count rows. The focused sanitizer test compares 800
worlds with the scalar full scan and exercises completion, ownership, death
and unpowered cooldown retention. The normal ROM verifier passes, with the
same IWRAM and EWRAM end addresses. Probe ROM SHA-256:
`dc580ae5f2ca3151b6b616056b23ae730832a11a5fdbe5573bfb743f3cc39ee3`.

This is an opt-in source probe. Z10's optimized defense stack frame grows
to 1,144 bytes including saved registers; falling back on a malformed
owner can hold both that frame and the 912-byte scalar frame. Run a dynamic
stack high-water check before release. Qualify other defense dispatchers and
both custom maps; use matching coordinate widths for noncompact builds
before enabling any shared default.
