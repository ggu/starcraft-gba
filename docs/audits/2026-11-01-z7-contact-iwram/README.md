# Z7 contact-mask IWRAM placement

The normal Z7 profile places `sc64_contact_blocked_mask_uncached` in IWRAM by
removing `SC64_CONTACT_STEER_ROM` from its canonical flags. The collision
predicate, bucket traversal, memory caps, save format, and input handling do
not change. P7 and P8 already use IWRAM for this function; the late Terran
profile forces ROM placement in `sc64_contact.h` and is unchanged. Z8 inherits
Z7's flags but explicitly retains its previous ROM placement.

## Normal product measurement

The reference normal ROM was rebuilt from source `79757a7` with the feature
gates at their defaults and is byte-identical to the prior shipping ROM
`59bd05448a2a579e6addd54cd37809a5678371b38feee0413bf9d52fff052b29`.
The placement-only normal ROM is
`d5857fc85b5d3646bd093f3238393ec05a74831bc7c5f18d11ef3be541d881c8`.
The compiler flag vectors differ only by omission of
`SC64_CONTACT_STEER_ROM`; neither includes the separate Z7 render-cache
experiment. The 109 prepared inputs were hash-checked before and after both
builds. Timing below uses emulator guest cycles at the normal physical-button
input boundary, including UI and presentation; it is not a physical-GBA FPS
measurement.

At ticks 50,001–50,900, both runs checked 135 trace observations through the
window. Nine sampled named-state sets, visible mode-4 frames, and palettes
matched; both recorded 2,242 route hits and 77 builds. Full-frame mean fell
from 2,445,439.6 to 2,106,257.4 cycles (13.87%); p99 fell from 6,456,860 to
6,175,745, and maximum from 9,546,615 to 8,985,008. These are bounded results,
not exhaustive full-world equivalence.

The placement ROM then completed the full input-only Z7 trace, matching all
303 recorded observations and winning at tick 179,764, frame 177,804
(`won=1`, `lost=0`, `outcome=1`, `fault=0`). Mean main-loop cost fell from
2,472,137.1 to 2,206,435.8 cycles (10.75%), approximately 6.79 to 7.60 guest
loop iterations per second. Peak since boot remained essentially unchanged:
49,718,805 versus 49,718,438 cycles. This does not meet the near-30-fps goal.
The full trace checks named model, action, entity-count and economy fields;
it is not a full-array comparison at every frame.

## Save continuation and limits

A separate build enabled the existing opt-in `SC64_SAVE_UI_CONTEXT` as well
as IWRAM placement. Its ROM SHA-256 is
`24a90222bb5ca0577ca032a2495cd21b80cc6019c6a905c87e51681e24b5128e`.
The unchanged physical input trace saved at tick 30,204 after 81 observations;
the SRAM SHA-256 `edec6a6bd4e5dd933fbea0b1d46388962462db80f2f6db68b6915432e3ab8c79`
matched the existing UI-on reference byte for byte. A fresh process restored
that save and won after 222 further observations; together the runs cover 303
distinct trace observations. Against the prior UI-on ROM
`64bea52aad8a8535256e2386339c2a3e888e05af6bbe73de9bd9342721068e9b`,
the resumed mean fell from 2,711,658.2 to 2,398,088.1 cycles (11.56%);
peak was effectively unchanged at 50,561,501 versus 50,561,390. The UI-on
feature remains off by default. The UI-off loader's existing reset of menu
and building context is unchanged and is not a strict continuation claim.

The normal placement link ends at EWRAM `0203d6f8` and IWRAM `03004f10`,
within the unchanged `0203e000` and `03005000` linker boundaries. The latter
leaves 240 bytes of code-placement headroom, not 240 bytes of stack. Startup
sets the system stack pointer to `03007e00`; the span from the code boundary
to that pointer is 11,776 bytes. Disassembly shows the relocated mask's own
stack frame decreases from 168 to 136 bytes. That local inspection is not a
whole-call-chain or interrupt high-water measurement for this ROM. Existing
historical painted-stack results used a different binary and do not qualify it.

## Source-scoped verification

The official `zerg-z7` target at source commit `b7f15f7` reproduced the
qualified `d5857f` ROM above. Its complete compiler argument vector matched
the manually qualified placement build, and all 109 prepared input hashes
matched before and after compilation. The subsequent `ae8eb1b` commit changes
only the test's expected flag string; Makefile, `common/`, and `starcraft/`
are unchanged from the qualified build.

At exact head `ae8eb1b`, a clean unprepared checkout passed `./dev doctor`,
`python3 tools/check_source_tree.py`, `./dev build`, and the full `./dev test`.
The latter includes 920 Python tests (25 skipped) and the native/host fixtures.
The first default test run failed only because the existing source test still
expected the removed flag; its failure was retained and the expectation was
updated before the successful rerun.

Separate normal P7, P8, and Terran T10 links at source `79757a7` passed their
unchanged EWRAM/IWRAM guards with all 109 inputs checked. P7 and P8 already
placed the mask in IWRAM, with IWRAM end `030049e0`; Terran T10's existing
source rule forced ROM placement, with IWRAM end `03004b40`. These are guard
and placement checks, not new cross-profile performance or full-play claims.

## Derived-profile and replay-link audit

The Z8 canonical vector inherits the Z7 vector, so source commit `5f46ff2`
explicitly restores `SC64_CONTACT_STEER_ROM` for Z8. A regression assertion
checks that selection. With identical prepared inputs, the expanded Z8
compiler argument multisets before and after the correction match. The
official Z8 builds are byte-identical at SHA-256
`f0f5cbd4a22bf3466b0c479f9df933ecc5b7ee4ca335d619f544259482dba95e`;
both end at EWRAM `0203dbb0` and IWRAM `03004400`, with the contact mask in
ROM. The 109 shared inputs and the recovered Z8 bank, descriptor and
generated program were checked around the links. This is a build/placement
comparison, not a full Z8 playthrough claim.

The same source's official Z7 ROM remains byte-identical to `d5857f` above.
Its separate full, save-prefix and resume-30204 ARM replay harnesses all
link within the unchanged guards at EWRAM `0203d7e0` and IWRAM `03004f10`.
The Z7 canonical host and construction-arrival fixtures also inherit the
Z7 vector; P7, P8 and Terran vectors remain independent. At `5f46ff2`, the
clean unprepared doctor, source-boundary, default build and full default
test pass, including 920 Python tests (25 skipped) and native/host fixtures.
