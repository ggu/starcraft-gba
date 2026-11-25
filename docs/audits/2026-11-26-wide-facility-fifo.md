# Wide facility FIFO trial

This opt-in experiment stacks on the wide point-field memo candidate. Define
`SC64_ROUTE_FACILITY_FIFO_PROBE` and `SC64_ROUTE_FACILITY_FIFO_WIDE_ONLY`
together to try the existing FIFO expansion only on 128×128 maps. Its
left/right/up/down discovery order and whole-pop origin stop match the
existing oracle; overflow falls back to the previous implementation. The
small-map path remains the previous one. This experiment is not enabled in
standard build flags. The wide-only expansion runs from ROM because placing
it in IWRAM exceeded the Z10 hot-code linker limit.

## Evidence and limits

`make test-sc64-facility-fifo` passes the scalar packed-field comparison,
including dedicated wide-only and forced-32-entry-overflow variants, under
address and undefined-behavior sanitizers. The test also checks field validity,
cache, grid and diagnostics across map shapes. The Z10 trial's linker guard
and `tools/gba_iwram_hot.py check` pass: IWRAM ends at `0x030062d0`, the
main stack bound is 5,892 bytes, the required margin is 512 bytes and 556
bytes remain beyond that margin. This does not cover other cartridges.

The paired Z7 battle replays the same 4,901 ordinary inputs, saved state and
rendered input tape used in `2026-11-25-wide-point-reuse.md`. Both builds use
the current release Z10 command and IWRAM configuration, with
`SC64_WIDE_POINT_MEMO_PROBE` on. The experiment adds only the two facility
probe flags. The control is the candidate ROM from that audit. The fixture's
72 semantic-world, both framebuffer pages, palette and menu samples and
its recorded start and end observations match. The displayed-page register
differs at one cut (1,024: `0x0404` control, `0x0414` trial); both page
hashes match at that cut. The mean is 1,745,430 versus 1,639,439 guest
cycles per update, 6.07% lower. The control ROM SHA-256 is
`1d3f8f098900371404523bc4d591f7ac568dddf4325cf9a4190e8c76526c38f6` and
the final candidate is
`e14e491edc17d8dbbfc76abd76fda40d3893e8cb9a142b820f9bc57d86c5dcb1`.
These checks do not prove equality at every update or on physical hardware.

A separate Z9 route on the final candidate ran 8,851 ordinary inputs and
256 after fresh SRAM reload. All 86 sampled state and display observations,
restored state and SRAM matched the memo-only control. The trial consumed
7,315,775,980 guest cycles over the first interval, 0.0882% less than
control. The save SHA-256 is
`28395f85eb499cdb4311ae4b7df7946a35dbe586364015a851d0da4af306c5cb`.

### Display-page follow-up

The same paired Z7 input was replayed with every update from 1,008 through
1,041 sampled. At each stop the observer read both actual 38,400-byte VRAM
pages, the palette, display control, frame and semantic world. At update
1,024 both corresponding pages, palette, frame and world matched, but the
control selected page 0 and the candidate page 1. The visible indexed images
differed at 14,545 of 38,400 pixels. The candidate image at 1,024 exactly
matched the control image at 1,025; at 1,025 both selected page 1 and matched.
All other sampled updates in that interval matched in visible image and page
selection. This is a transient one-sample early selection in this replay,
not a persistent rendering delay; it is still a visible difference at 1,024.
The sample does not establish the sub-update flip time or other situations.

### Campaign continuation and combined release build

The final FIFO probe on the original memo test base replayed the Z7
continuation from tick 136,697 to 270,585. All 45 remaining checkpoints
and their semantic world fingerprints matched the earlier control. It won
at frame 267,585. The victory SRAM matched the control and a fresh reload
byte for byte. This run starts from the retained battle save.

Separately, both probes together were built on the shared combat and
Zealot-vitals release with the corrected 56-patch Blood Bath pack.
Over the same 4,901 Z7 updates, the unmodified corrected Z10 ROM
(`81cba018…`) averaged 1,902,744 guest cycles. Both probes together
(`3b0873b9…`) averaged 1,639,400, a 13.84% reduction. Effective update
rate rose from 8.82 to 10.23 per second. All 72 sampled world states,
both video pages, palettes, menus and selected pages matched in this
comparison. The earlier FIFO-only comparison isolates its smaller gain.

All four combined products linked at 32 MiB. The smallest remaining
EWRAM margin was 3,920 bytes on Protoss. The race stack checks cleared
the required 512-byte safety margin by 256 bytes on Terran, 556 on Zerg
and 820 on Protoss. Showcase cleared its static IWRAM boundary by 392
bytes. Additional busy scenarios, a full campaign run on the integrated
release, and physical GBA timing remain open.

## Corrected-pack custom matches

Eight physical-button mGBA comparisons used the corrected shared release as
control and the combined point-memo plus wide-facility probes as candidate.
For each of the four cartridges and two maps, a three-seat match ran 10,000
updates, was saved, then each binary loaded that same SRAM and advanced
2,048 updates. The 64 samples at 32-update intervals matched in semantic world,
mineral bank bytes, raw unit and building arrays, both framebuffer pages and
palette; the newly exported SRAM matched byte-for-byte in all eight cases.
These are 64×64 maps, where this experiment retains the old facility path;
they are cross-mode regressions, not wide-map speed measurements.

| Cartridge | Outpost cycle change | Blood Bath cycle change |
| --- | ---: | ---: |
| Terran | +1.310% | +0.043% |
| Zerg Z10 | +1.685% | +0.318% |
| Protoss P10 | +0.610% | +0.054% |
| Showcase | +0.653% | +0.030% |

Cycles are total guest cycles over each 2,048-update interval; positive is
slower than control. Both page buffers matched, but the display-page selection
was different at one Z10 Blood Bath sample (1,120 updates) and one Showcase
Outpost sample (1,472 updates). In both cases the two pages had different
hashes, so those selections imply visibly different images at those samples;
the pixel counts and duration were not measured. All other sampled selections
matched. These results do not establish equality between sample points or
physical-device performance.

## Corrected-pack Z7 from fresh HOME

On the corrected shared release, the historical physical-input tape ran from
a fresh HOME process to victory on both the control and the combined probes.
Both reached update 270,585, game frame 267,585, with `won=1`, no fault and
zero enemy buildings. All 206 retained historical named observations matched
on each build. At 475 sampled cuts, the two builds matched in named state,
semantic-world fingerprint, both framebuffer pages and palette. At three cuts
(35,000, 48,536 and 172,000) the displayed-page selector and visible-page hash
were different; these are visible differences and their duration and pixel
counts were not measured. The final SRAM matched byte-for-byte, SHA-256
`ad920008ce08bad0f493d8c2831cd5676f2360f11a71ab4e9a1905b1d61ae831`.
Fresh-process reload on each build restored the same terminal state and world
fingerprint and re-exported the same SRAM. Sampled comparisons do not prove
all intermediate updates identical.

For the corrected 4,901-update busy Z7 window, the shared-release control
without either probe used 1,902,744 mean guest cycles per update; the candidate
with both probes used 1,639,400, a 13.84% reduction. All 72 sampled world,
framebuffer-page, palette, page-selection and menu observations matched. This
comparison measures the combined probes, not the facility FIFO alone, and
includes presentation waits and UI. It does not establish full-campaign timing
or physical-device frame rate.

A second ordinary-input route was generated by the corrected-source host
controller with its early-gas policy and a render between updates. It won on
the host and was then replayed from fresh HOME on both ARM builds. Both won at
input 237,213, frame 234,563, with zero enemy buildings and fault 0. All 481
sampled named states, semantic-world fingerprints, both pages, palette and
page selections matched between ARM builds, as did all retained host named
observations. Terminal SRAM matched at SHA-256
`7c1e2eea1b83f1a56b4260827f181fc86fb78f9c2d111d52695d6bb70ae4fc26`;
fresh-process reload restored the same result and re-exported identical SRAM.
These two runs use the corrected shared release and the two route probes, not
the separate source-gas depletion candidate.
