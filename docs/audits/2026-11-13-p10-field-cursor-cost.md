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
