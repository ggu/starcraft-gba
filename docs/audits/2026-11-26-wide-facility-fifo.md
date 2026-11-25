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

A separate Z9 route on the first equivalent wide-only trial ran 8,851
ordinary inputs and 256 after fresh SRAM reload. All 86 sampled state and
display observations, restored state and SRAM matched the memo-only control.
The trial consumed 7,315,775,980 guest cycles over the first interval, 0.0882%
less than control. The trial save SHA-256 begins `28395f85`. This comparison
used ROM `b86e97768fed36ee330f4b60bcc0559a4976a1dfcb13a92d4da1455f039b7396`;
the final source uses the same wide-only logic with the scope expressed as a
separate flag. It needs its own longer replay qualification.

Still needed before enabling the option: final-source full campaign and SRAM
qualification, other map shapes and all four production cartridge builds,
additional busy scenarios, profiling for presentation effects, and real GBA
measurements. The mGBA cycle counts include presentation waits and UI and do
not establish hardware frame rate.
