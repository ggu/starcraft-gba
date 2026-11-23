# P10 cursor query cost after packed sight

Sixteen matched, ordinary packed-sight P10 updates average 1,713,563
guest cycles. The renderer accounts for 768,097; its explicit flip wait
takes 25 cycles on each sampled update, and `gfx_end` takes 93. The
samples exclude trigger sweeps and depot stalls, and do not estimate
display waits on other workloads.

The render-only cursor action query takes 61,459 cycles on average
(3.59% of the whole update), rising to 87,863 in the five late samples.
Its busiest direct child is the hostile unit/building scan
(26,660 cycles, or 42,730 late); it evaluates the mission hostility
policy for every eligible actor before checking cursor distance.
The selection scan and mineral helper add 14,434 and 11,597 cycles
under this query. The worker scan adds 7,504 directly, and the
mineral helper repeats worker/selection work. These are nested scopes
and must not be summed twice. The larger `draw_unit_pass` takes 156,085,
but spans culling, sprite lookup, several blitters and drawing; the
cursor path is a more direct opportunity with a tighter equivalence test.

The optional `SC64_FIELD_CURSOR_QUERY_PROBE` filters nearby geometry
before expensive owner or selection predicates, keeps exact distance
and slot tie order, and reuses the worker and selection results in the
race-profile HUD. It defers selection until needed, preserving the
double-tap, gas, building, attack and mineral priorities. It changes
no render cache, actor lists, saved data or rendering order. Gameplay
uses the same hostile, selection and mineral rules.

The asset-free ASan/UBSan differential compares the cursor outcomes
over 1,200 deterministic worlds and the full framebuffer for 60 of
them. The established field UI regression also passes with the option
on. A prepared P10 flag-vector host pair matches query results with
the option on and off.

## Normal cartridge comparison

The cursor-only P10 candidate uses the exact KODAK icon control
runtime source (`cbba1c5`) plus the three cursor runtime changes and
the opt-in flag. It does not enable packed sight or other probes.
The first 43,000 physical-button calls from HOME match the existing
control on all 17 HOST checkpoints, ten briefing records, seven modal
acknowledgments and 32 ordered video/world samples. Pixels, palette,
fog, cursor, actor/mineral pools and route observations match exactly.

| Scope | Control cycles | Cursor cycles | Change |
| --- | ---: | ---: | ---: |
| 43,000-call mean | 1,879,249 | 1,856,196 | −1.23% |
| Calls 5,001–6,000 mean | 1,637,408 | 1,622,839 | −0.89% |
| Calls 24,001–25,000 mean | 2,060,827 | 2,036,236 | −1.19% |
| Calls 42,001–43,000 mean | 2,208,690 | 2,170,418 | −1.73% |
| 16 ordinary instruction samples, whole loop | 1,759,540 | 1,723,853 | −2.03% |
| Same samples, render | 768,096 | 735,144 | −4.29% |
| Same samples, cursor action | 61,454 | 28,506 | −53.62% |
| Same samples, hostile cursor scan | 26,658 | 8,012 | −69.94% |

The five late instruction samples improve 2.53% in total. Mean
simulation cost is effectively unchanged (935,215 to 935,221), and
explicit flip wait stays at 25 cycles. Prefix rate rises from 8.93
to 9.04 emulated updates/s; this is not hardware performance.

The ROM verifies under the unchanged guards. IWRAM and BSS ends
are identical at `0x03004ab0` and `0x0203c0e0`; linked text grows
424 bytes. This is a bounded P10 comparison, not full victory,
fresh-process SRAM, or the other race products. The probe stays off.

- Candidate ROM SHA-256: `11be32e0b879dc1334fe09b16cf57500f5f368624d8fa2f61cf78f23956fb14f`.
- Control receipt SHA-256: `d8b2919ddc66671328f2932041f85484268258acba1927784f8ab1407a8b1f47`.
- Candidate receipt SHA-256: `1e554b63e6183ffc8b95a6189b837e3f8a2482286cc9dba561809a425825c46e`.
- Matched ordered video/state receipt SHA-256: `aa16bad817241888dc4c7184e9773ecfe2efc28d727bd5d0aed1f928dab1752a`.

## Current four-cartridge candidate

The shared cursor path is composed over the direct Bring census.
The default build and all four regular ARM profiles use the same
distance-first queries. The host differential retains the original
query as its separate reference build.

The P10 victory replay matches all 81 ordered cuts and final SRAM.
Its mean falls from 1,484,787 to 1,464,990 cycles per update over
9,489 updates, a 1.33% saving. In the two matched 16-update P9
windows, the combined cursor change saves a further 3.89% and
0.38% beyond direct Bring. The earlier hostile scan falls from
about 50,900 to 13,694 cycles per update. The 32 sampled worlds,
display pages, palettes and route counts agree.

An ordinary-button replay on both P10 builds restores the same
P9 pause journal, then presses A twice over a crowded selection.
The first press selects one of 27 nearby candidates, actor slot
95. The second selects 14 actors of the same type. All six cuts
agree on selection state, cursor, camera, both display pages and
palette. The separate Terran foreground fixture checks opaque
cross-layer picks, painter ties and visibility, with matching
ASan/UBSan output. The four ARM builds retain their RAM ends.
All 30 campaign openings match, and all 24 regular Custom Game
pause and fresh HOME cases pass. Six 12,000-update three-CPU
matches across both maps agree on all 120 sampled cuts. Guest cycle
totals decrease by 0.012–0.115% across these six matches.

The full P9 route reaches victory at frame 272,857 after 281,447
ordinary calls. All 93 checkpoints and 11 briefing entries match the
pre-Bring control and the direct Bring build. The pause reload matches
all 80 subsequent checkpoints. A third emulator process reloads the
victory journal. The pause, victory and readback SRAM files each match
byte for byte across all three builds.

The cursor build takes 383,550,668,426 guest cycles across the full
route, down 1.24% from 388,382,113,584 with direct Bring and 9.49% from
423,749,329,236 before both changes. It averages 12.31 emulated
updates per second, including UI and presentation time. This replay
doesn't measure the frame rate on a GBA. The tested P10 ROM matches
the default cursor release ROM byte for byte. Its SHA-256 is
`562b93c371cf7a3734a361eddee88ac8c2e9394ff76684f0b1b186faca75c3c8`.

In the two 16-update windows after loading the P9 pause journal,
the cursor build still averages 1.10 million cycles per update.
The defense pass takes 82,271–83,002 cycles and the target-index
rebuild takes 78,814–80,012 inside the order pass. The remaining
location/count calls take 52,594–53,347 cycles and already use the
direct Command and Bring paths. These timings identify remaining
cost in this replay. Qualify any changes in different campaign and
skirmish populations before activating them.

| Local receipt | SHA-256 |
| --- | --- |
| P9 instruction windows | `fb9b4ec58c53a6d75053bfd1ed23b3045722b874c8460a698bc83b52e9bf9379` |
| P9 full victory and save reload | `f759b2f0be24f5bdbde2fa1bdc663e680765cd4fd1bde44a750da1100e4136a1` |
| P10 victory tail | `ccc6b12c7ac12a617b1282fd072ed92314f891f2e6d51304ce95ebd8f72177bc` |
| P10 crowded double-tap | `67fbf75f93a5ae70ae3aadfe4c46da081b188ab873394c6f790ec6d96e0aba74` |
| Custom Game timing matrix | `5ab87d5efa509ed543d217a703625dcf3758b545d58f722b2d9d18cf2b9ea3cd` |
| Custom Game saves | `0790a5c3378dd81349b8988de78be00f6ad6329228248d40cf97357ae75e669f` |
