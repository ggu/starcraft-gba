# Terran packed unit rendering

Canonical Terran now enables the packed unit sprite path together with the
already-tested vital-bar renderer and cold route-flood split. The original
indexed sprite path remains available for comparisons with
`make -B terran TERRAN_UNIT_WORD_FLAGS=`. No unit, save, map, AI or combat fields
change with these options.

## Paired replay evidence

The source includes the stock combat corrections merged in PR 34. The paired
builds use identical private art and input transitions. The baseline is the
previous canonical Terran flags; the candidate adds only
`SC64_ROUTE_FLOOD_SPLIT`, `SC64_VITAL_BAR_FAST` and
`SC64_UNIT_WORD_SPRITES`. The T7 and T10 controllers start through the explicit
mission probe entry and use ordinary input transitions afterward. These are
not normal-menu launch proofs.

Both host variants and both no-GDB mGBA ARM variants win. All 79 T7 and eight
T10 state/framebuffer checkpoints match between variants, and independently
between host and ARM. T7 consumes 1,293 transitions through tick 39,012;
T10 consumes 134 through tick 3,511. Both finish with zero input errors,
runtime faults and unsupported actions.

The weighted means below include the terminal partial window according to its
tick count. Render includes presentation overhead; raw draw is the separate
draw counter, so these are nested measurements rather than additive stages.

| Trace | Stage | Previous cycles/tick | Candidate cycles/tick | Change |
| --- | --- | ---: | ---: | ---: |
| T7 | full tick | 770,559 | 748,024 | -2.92% |
| T7 | render | 424,873 | 402,243 | -5.33% |
| T7 | unit draw | 45,862 | 33,668 | -26.59% |
| T10 | full tick | 899,010 | 894,518 | -0.50% |
| T10 | render | 464,314 | 459,754 | -0.98% |
| T10 | unit draw | 89,159 | 70,299 | -21.15% |

Update means change by about +0.02–0.03%, consistent with measurement noise;
this is a drawing improvement. The highest complete-window full means fall
from 983,192 to 916,323 for T7 and 985,184 to 968,898 for T10. These traces
correspond to about 22.4 and 18.8 fps equivalent, below the 30-fps target.
They do not establish a fixed frame rate or physical-GBA performance.

The diagnostic counters distinguish resolved frames from calls passing the
word blitter's initial geometry checks: T7 has 61,128 resolved / 61,005
validated calls; T10 has 33,557 / 33,527. Early geometry culls account for the
difference. Both traces have zero derived-raw calls; the imported actors
retain explicit source identities. Ordinary trained-unit derived identities
remain covered by the separate spawn/renderer fixture.

## Art, setup and memory

The standard Terran-late bank was regenerated with `--include-t6
--include-late-terran --prepack-unit-words`. All 220 selected indexed pixel
arrays are byte-identical to the old bank. Its 134 unused ZP-gated arrays
are omitted. The resulting header is byte-identical to the paired replay's
private input. Generated art and ROMs remain ignored.

`./dev sc64 prepare --rom PATH --profile terran` requests packed metadata.
Status/build reject a stale marker-only or indexed-only header, before Make.
The production ROM still uses `SCT8` and the existing save format. Its link
has IWRAM end `0x03004e08` and BSS end `0x0203ddd4`, within the unchanged
`0x03005000` / `0x0203e000` guards; it pads to 32 MiB. Private diagnostic
ELFs have different scaffolding sizes and are not substitutes for this
production memory check.

The full Terran host-controller target now uses the actual canonical ROM
flags for all ten missions. Late mission controllers retain their explicit
probe entry. Local logs and source/art/trace manifests stay under
`.local/build/terran-3cdd-qualification/` and
`.local/build/terran-canonical-word-promotion/`. No GitHub Actions runner was
enabled or used.
