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
Compared with the pinned prior backend source `ad37a62d...`, it only adds
bounded read access to the two DISPCNT bytes; the prior backend already allowed
bounded palette and VRAM reads. It still denies writes in physical mode and
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
