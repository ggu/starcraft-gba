# Delivered P10 busy P7-input phase profile

This measurement uses the actual 32 MiB P10 package ROM, corresponding ELF
and an ordinary physical-button P7 tape. The ROM SHA-256 is
`ff94a2ea9f98404c97f44a8b6b7bf557b3572c761de530d867032eeb8f76b10b`;
the ELF SHA-256 is
`f961647217516de367b2f2e28b1b82dee4e744e3bef7f9c7b0f46e4100f55587`.
The assembled 738223a4 package documents this P10 binary as unchanged from the
94bc qualified package. The production runtime sources in the current
21bc709b checkout match the 738 package's recorded runtime manifest; the 392
original package dependencies all rehashed before and after. Source and
private inputs, tape, observer and output remain isolated; no ROM or game data
is committed. Subsequent default-off source merges need their own binary
identity check before claiming exact source equivalence.

The pinned retained P7 tape SHA-256 was
`44e68fec4bd45ef8e8df86144dcfe877dc99592f2adf99107351cfc1bb0ed3a6`.
The replay starts from a fresh title, asserts actual P7 identity, and measures
tape ticks 8020–9019. It inserts physical A/Start taps only when the real
text-box queue requests them, preserving held inputs and tape numbering;
three modal acknowledgments occurred before the measured window. There were
958 advancing and 42 nonadvancing play updates; no measured title, pause or
result updates. At the first/last checkpoint the world contained 177/178 live
units and 112/113 live buildings; owner counts, resources and other sampled
state are in the private receipts. This is a single established ordinary-input
combat workload, not a complete campaign, global 30-fps claim, or hardware
measurement.

The control and counter-reading runs use the same unmodified package ROM and
physical-button runner. They match all 1,000 elapsed cycle values, frame/screen
and mode transitions, and 11 sampled selected-world/fog fingerprints and
complete displayed indexed framebuffers plus palettes. No guest code or
memory writes are injected. Counter reads are outside each measured interval;
the observed guest-cycle overhead is zero in this pair. These sampled checks
are not full intermediate-state or every-frame visual equivalence.

| Existing production scope, advancing calls | Mean guest cycles | Maximum |
| --- | ---: | ---: |
| Input-to-input elapsed | 2,253,117 | 3,955,382 |
| Update | 1,636,526 | 3,430,050 |
| Orders, inside update | 1,152,654 | 2,984,063 |
| Other update | 338,302 | 359,695 |
| Visibility | 81,499 | 308,522 |
| Map | 383,755 | 612,820 |
| Background, inside map | 210,925 | 373,300 |
| Unit/effect draw, inside map | 76,155 | 126,796 |
| HUD | 124,001 | 379,051 |
| Draw before presentation wait | 544,897 | 980,287 |
| Audio IRQ delta, overlapping other scopes | 52,538 | 105,661 |

Scopes are nested/overlapping, IRQ time can occur inside any scope, and wait
behavior is included in elapsed. Do not add or subtract them as a predicted
saving. The slowest call was tape tick 8347; orders alone consumed 2,984,063
cycles there. This identifies orders as the largest measured target in this
window; it does not identify a safe algorithm or establish a global gain.

The read-only observer backend uses pinned mGBA static library SHA-256
`a112d35cfa83d108a89fdcfb42ada9f6136c0b74ea219b2c67e1ca8f77c0bf37`
and source matching `tools/sc64_headless_replay.c` in this branch, SHA-256
`8d98b6250e891841e1358d10b33cb967504e215f8b55ded7133719aeec8fb798`;
its binary SHA-256 is
`f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d`.
Compared with the pinned private backend source `ad37a62d...`, the measured
variant only adds bounded read access to the two DISPCNT bytes; that private
backend already allowed bounded palette and VRAM reads. Relative to the public
21bc source, this branch adds all three bounded read regions. It still denies writes in physical mode and
all other MMIO, SRAM and OAM reads. Runtime boundary tests verified the
allowed endpoints, crossing/other-MMIO rejection and denied guest writes.
The `rawRead8` mGBA API supplies these observational reads. This also corrects
earlier audit wording: those prior Z9 runs did not compare guest pixels, but
it was incorrect to say the backend could not read palette or VRAM at all.

Private receipts: isolated checkout `.local/qualification/p10-busy/` contains
`inputs-before.json`, `inputs-after.json`, `control.json`, `profile.json`,
`result.json`, backend bounds log, package/tape and run logs. The next step is
a separately guarded setup/movement/target diagnostic on the same tape, with
state/pixel pairing, explicit overhead and linker guards. No optimization is
proposed until that evidence identifies a safe operation.


## Current-source identity and nested orders attribution

A clean `c27bb48c` merged-source checkout, using the same package vector and
392 verified dependencies, rebuilt the exact packaged P10 ROM `ff94a2ea...`.
Thus the uninstrumented package measurement also identifies the flag-off
current merged binary; this does not qualify future changes.

A separate diagnostic ROM enabled the existing `SC64_DIAGNOSTIC_ORDERS` and
`SC64_DIAGNOSTIC_SETUP` scopes on the identical package input set. Its SHA-256
was `394ba03d328b6e4d00ccd608343052a80da439ee5103de8056491055bb1ff777`.
The control link reproduced the packaged ROM, and control/diagnostic BSS and
IWRAM ends were `020391e0/03004b20` and `02039218/03004bc8`; existing guards
passed, but stack high water was not measured. The diagnostic replay matched
all 11 selected-world/fog/displayed-indexed-framebuffer/palette samples, all
frame/screen/mode transitions, 958 advancing and 42 nonadvancing play calls,
and three modal acknowledgments. This does not prove every intermediate state
or every pixel on every frame. Mean advancing loop was 2,312,666 cycles versus
2,253,117 for control, a 2.643% net observer overhead. Code layout and nested
timing change, so stage values remain approximate inclusive observations.

| Diagnostic nested scope | Mean cycles | Maximum |
| --- | ---: | ---: |
| Orders | 1,202,751 | 3,030,862 |
| Order setup | 589,745 | 768,887 |
| Arbiter due processing within setup | 464,761 | 644,051 |
| Owner target index within setup | 78,002 | 87,969 |
| Movement calls within orders | 370,902 | 2,673,153 |
| Target queries within orders | 84,776 | 104,267 |

At tick 8347, orders were 3,030,862 cycles and movement 2,673,153; the
Arbiter due scope was only 306. Other samples have high Arbiter cost, so mean
and tail have different causes. These nested scopes cannot be added and are
not a proof of an exact safe optimization. The next source-level task is to
classify due Arbiter rows/owner masks and movement route calls on this window,
then select a bounded exact computation with an independent oracle.

The backend read-range predicate was subsequently factored into a shared
header with an asset-free ASan/UBSan boundary test, including endpoints,
overflow and denial of other MMIO/SRAM/OAM and physical writes. That source
refactor produces backend binary SHA-256 `e342cc659c60d0a7e1a335bb2aed5772ac813b2cd4f188f7552589d21f848660`,
which also passed the live protocol boundary/write-denial tests. The earlier
measurement used the pre-refactor `f25b8924...` binary stated above.
