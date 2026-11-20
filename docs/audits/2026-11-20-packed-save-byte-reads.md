# Packed save field reads

The 35-byte ZP unit and 28-byte packed building codecs read fields in
byte slices. They retain the existing least-significant-bit-first encoding,
field limits, padding checks, zero-row treatment and atomic decode. The
reader validates the entire span before reading and touches only its
covering bytes. Journal selection, candidate validators, encode and commit
order remain the same.

Z10 uses both readers. Terran uses the packed building reader. For this
first change, the P10 and Showcase typed compact codecs remain outside
the changed path. Their candidate ROMs are byte-identical to their controls.

## Cartridge comparison

The control is the `916bb897` runtime from PR #606, SHA-256
`d2633b10d0c890ec1a410d6899b35476f0aaa90787c2a0e4692c9274f034cdaa`.
The Z10 candidate SHA-256 is
`cff00c7f0d2af5bc60658474db376e72977da8e161c1286389c7c3b6d2fd30a3`.
Both use the same compiler arguments, imported data and normal ROM profile.
The Z8 ARM controller tape SHA-256 is
`b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb`.

The paired runs matched all 46,487 controller calls, 50 gameplay checkpoints,
34 page, palette and visibility samples, and 19 host checkpoints. Each run
exports the pause journal after call 17,121, reopens it with HOME L in a fresh
process at frame 16,659, and wins at frame 45,468. The exported pause and
victory SRAM journals match byte for byte. Both contain 32 KiB.

| Ordinary controller call | Control clock cycles | Candidate clock cycles | Change |
| --- | ---: | ---: | ---: |
| 17,120 pause save | 25,380,687 | 18,722,441 | −26.2% |
| 17,122 fresh HOME L load | 43,043,187 | 30,900,401 | −28.2% |
| 46,487 victory save | 60,095,088 | 39,865,784 | −33.7% |

The fallback check copies the victory journal and writes a noncanonical
`path_timer` into one packed unit in its newer slot. It recomputes the slot
checksum. Both normal cartridges load the newer record from the clean
journal and restore the exact older frame 16,659 from the checksum-valid
edited journal. This covers semantic rejection after checksum acceptance.

The normal Terran and Z10 controls and candidates also match on Blood Bath
with all three independent AI seats occupied. Each ordinary pause save at
frame 361 exports identical SRAM. HOME L restores the named world in both
versions, and a 240-update replay matches at frame 601. The custom smoke
receipt SHA-256 is
`1ae32fdd59054d617dc6272bc418b19697be1d7ca866e15d5f665c5daca03b62`.

The four normal ROMs pass the verifier and keep their control IWRAM and
EWRAM link endpoints. In the Z10 disassembly, the unit decoder plus field
reader uses eight fewer stack bytes and the building decoder plus reader
uses twenty more. Terran's building decoder plus reader uses twelve more.
The save and load entry-point stack frames remain the same in both builds.

## Host tests

The sanitized ZP unit fixture compares the byte reader with the per-bit
algorithm at every field offset and width through 32, including fields that
straddle five bytes and invalid tails. The tests cover the compact building
and typed codecs, legacy row fields, malformed decode without mutation,
prepared journal fallback and atomic predecessor preservation. The normal
Z10 host startup and save fixture passes 234 skirmish configurations.

## Typed building follow-up

P10 and Showcase now use the shared byte reader for their 25-byte typed
building rows. The existing fixed-offset reader continues to decode typed
unit rows. The typed building encoder and the checks for the row marker,
type range, padding and field values retain their existing behavior.

Both candidates use the normal compiler commands and pinned inputs from
the `06353a5d` runtime. They pass the ROM verifier and retain their IWRAM
and EWRAM endpoints. The typed building decoder and reader together use
12 fewer bytes of stack in both ARM builds. The save and load entry frames
are unchanged. The sanitized typed codec, packed codec and prepared journal
fixtures pass.

The Eye of the Storm A/B replay matches 47,266 physical controller calls,
10 briefing pages, 14 modal pages and 20 gameplay checkpoints. Both ROMs
restore the pause world with HOME L at frame 36,761 and finish at frame
46,129 with zero faults. Their 32 KiB pause, victory and victory readback
journals match the archived release journals byte for byte.

| P10 event | Control clock cycles | Candidate clock cycles | Change |
| --- | ---: | ---: | ---: |
| Pause B save at call 37,703 | 17,138,906 | 16,667,429 | −2.8% |
| Fresh HOME L from pause | 34,061,620 | 31,257,728 | −8.2% |
| Victory autosave at call 47,266 | 36,545,515 | 33,694,918 | −7.8% |
| Fresh HOME L from victory | 36,269,856 | 33,466,129 | −7.7% |

Clearing the typed building marker in the newer victory entry and
recomputing its checksum makes both normal P10 ROMs restore the same older
pause world. The unedited journal restores the newer victory on both.

Blood Bath with three occupied AI seats also produces byte-identical
pause SRAM and matching worlds 240 updates after fresh HOME L on both
P10 and Showcase. The respective save and load clock reductions are
1.1% and 1.5% for P10 and 0.6% and 1.5% for Showcase.

The P10 replay comparison receipt SHA-256 is
`a63416f8c98c75d49ad4990059f224b747ad2a39839b5dc4807495363e8b2625`.
The three-AI comparison receipt SHA-256 is
`9b86e75d2cc488fd196bb30fd4e7b920f8bf54c8f6d66df36681048fe78c266e`.

### Replay with the stationary-effect depth fix

Paired normal cartridges from release `f6869829` confirm all 47,266
controller calls and 20 gameplay checkpoints, including the fresh HOME L
load at frame 36,761 and victory at frame 46,129. Both builds reproduce
all three archived SRAM images byte for byte. Both also recover the
older pause entry from the checksum-valid journal with an invalid
typed-building marker in the newer entry.

| P10 event | Release cycles | Candidate cycles | Change |
| --- | ---: | ---: | ---: |
| Pause B save at call 37,703 | 17,140,329 | 16,668,832 | −2.8% |
| Fresh HOME L from pause | 34,061,620 | 31,257,728 | −8.2% |
| Victory autosave | 36,543,443 | 33,692,621 | −7.8% |
| Fresh HOME L from victory | 36,269,856 | 33,466,129 | −7.7% |

Mean across the full route stays at 10.52 emulated calls per second
rounded to two decimal places. The gain is concentrated at save and load.
Both current P10 and Showcase builds keep their RAM endpoints. Both match
the release save at frame 361 and the world at frame 601 after a fresh
HOME L in the three-AI Blood Bath comparison. Release `f5ea4cf3` then
updates the Unified HOST fixtures and keeps these two cartridge recipes
unchanged.

The campaign comparison receipt SHA-256 is
`1c1b4b4b5ddb043e04f674368b1905453aef892c8806b777a600e553d7b5538b`.
The three-AI receipt SHA-256 is
`e0ba425c919e495cdee46740636b9829acde9c07612a213e97837c669dedb00f`.

## Opt-in fixed-width compact Unit reader

`SC64_COMPACT_UNIT_FIXED_READER` uses the existing field sequence and
running bit cursor. GCC propagates the positions and widths through the
straight-line assignments. The decoder reads only the bytes each field
spans. The encoder, 38-byte wire row, all-zero slot, 303-bit field length,
reserved padding check, field validation and final assignment after
successful validation retain the existing behavior. Journal tags,
checksums and candidate selection use the existing load path.

The normal and opt-in ASan/UBSan builds pass the existing 10,000-row
fixture. A differential using the production 56-byte Unit layout
compares 300,307 unaligned wire and dimension cases. Both readers accept
the same 4,834 cases and produce byte-identical outputs. Rejected inputs
leave the output sentinel unchanged. The compact ZP and Guardian wire
fixtures also pass with the opt-in flag. A standalone ARM7TDMI Thumb
object matches the original object byte-for-byte with the flag off.
With the flag on, it uses the same code as the evaluated explicit-offset
form and removes calls to the generic compact Unit bit getter.

The paired, pre-word-packing Terran control uses compiled source
`afffebec` from the #627 result-save audit. The opt-in cartridge
uses the same recipe and inputs and keeps the same EWRAM and IWRAM
endpoints. All 27 observations in the 4,190-call T6 suffix match.
Both versions export identical victory SRAM and frozen-result readback.
[T6 journal timing](2026-11-13-t6-normal-physical.md).

| T6 event | Control guest cycles | Fixed reader guest cycles | Reduction |
| --- | ---: | ---: | ---: |
| HOME load from pause | 28,727,633 | 24,091,076 | 16.14% |
| Result autosave | 37,255,972 | 30,193,003 | 18.96% |
| HOME load from victory | 31,093,497 | 26,454,592 | 14.92% |

A full ordinary T6 run from HOME also matches all 42 checkpoints,
including nine after a fresh HOME load. The pause, victory and frozen
readback SRAM files match the retained full replay byte-for-byte. The
private driver uses a 48 MiB disk reserve for its bounded outputs.
This opt-in measurement predates the aligned route-word packing default
and uses emulated guest cycles.

| Input or receipt | SHA-256 |
| --- | --- |
| Pre-word-packing control ROM | `01240c5e95c1f0e648028dad75005e3185904c02e6c9b0577aa368d14e7958f3` |
| Control ELF | `85aa28c9206b11cd9baebdc9f33d8d9bf6de49fa624146480c1cf351d87dbc10` |
| Fixed-reader ROM | `277c302b87ae06437d5c9e2ee95b538cd30782297f2632be716f9244d4778563` |
| Fixed-reader ELF | `e437be5747bfd59caaab2299614e46dd2d4d71585cce09a57a4d052a10590b3c` |
| Paired timing receipt | `0f9a44cf1157742ec9259283d84e14206c78b2166cec5185f4cd407d3ef208f3` |
| Full T6 replay receipt | `923cf831e5839f05f08a558b2b4279a7eddb9eb60b460531f32c3ce7f94f63fc` |

## Normal Terran activation with aligned route packing

The released Terran control rebuilt from `603755a7` matches ROM
`1ff78b901e13b6af59d89ab93634b0c04eea3d05694b5092893fa34082e2d1f9`.
Adding fixed-width decoding to that recipe produces
`4b66e7e8be4b3655aded09c1adaf56836fe4f18b4856b6684d092489ad5e2cf7`.
The ordinary late Terran codec now selects the fixed reader by default.
The normal production recipe, with no opt-in compiler flag, reproduces
the tested candidate ROM byte for byte. The explicit
`SC64_COMPACT_UNIT_LEGACY_READER` setting reproduces the control ROM.
These settings leave the wire format and both bank validation paths
intact.

On the combined release, all 27 observed T6 suffix states match and
the two builds export identical victory and fresh-result SRAM. The
observed timings are the same as in the table above. The mixed
three-opponent Blood Bath replay matches all 35 named samples,
both pause journals, and both fresh 600-update continuations. This
includes the active Bunker foundation and four loaded passengers.

Terran keeps IWRAM `0x030048a8` and EWRAM `0x02038e5c`. The linked
Z10, P10 and Showcase ROMs remain byte-identical to the released
word-packed products, including when the codec flag is passed in
their qualification recipes. The combined receipt SHA-256 is
`9c7221662961da0ab25d9900cb6e669899771ff553ce97128bb640ead6719b2a`.
The default and legacy Terran identity receipts are
`d30aac01132a613fdd816a44405fab05f31d166255adb70d232538361de42542`
and `ee1198c120bac61775ab896da684e23ff522d9f65d800404f828544a59e75c0f`.

The source-boundary check, doctor, asset-free build and full default
suite pass locally. The Python suite runs 1,166 tests with 35 optional
cases skipped in its isolated asset-free checkout. The prepared Terran
sanitizer lane separately executes the full campaign targets and the
T6 and bank-validation fixtures. It passes all 33 cases from 30 binaries.

### Remaining save cost after activation

The same combined Terran ROM was profiled on the result autosave at
frame 23,536. All 27 suffix observations and both SRAM exports match
the ordinary replay. The main-loop counter reads 30,193,003 cycles
and the instruction interval spans 30,193,530 cycles.

| Function | Inclusive cycles | Self cycles | Calls |
| --- | ---: | ---: | ---: |
| `race_save_game` | 28,342,471 | 778,513 | 1 |
| `sc64_unit_save_decode` | 8,190,416 | 2,259,491 | 2,881 |
| `sc64_building_save_decode` | 6,958,491 | 1,369,108 | 866 |
| `sc64_packed_building_get` | 2,966,450 | 2,966,450 | 21,650 |
| `save_write_selected` | 1,570,936 | 809,369 | 1 |

The inclusive decoder rows overlap their children. Fixed Unit
decoding reduces its inclusive work from 15.10 to 8.19 million
cycles. The packed-building getter is the next direct bit-reader
cost. Use the same fixed-width differential and byte-identical
journal checks if testing that row. The stack receipt SHA-256 is
`74b16ec56a73e38499d4e441effe1fc00c7b65187c1ddcb07acd5499bdafe54a`.

## Inlined packed Building reads on the detached-depth release

The opt-in `SC64_COMPACT_BUILDING_FIXED_READER` inlines the existing
shared reader at the known Building field positions. The checked spans,
decoded fields, reserved-bit test and output commit stay in the same
order. The candidate keeps the established Unit decoder and reuses the
existing byte reader. Disabling the option reproduces the release
`25e9e8b9` Terran ROM byte for byte.

The asset-free sanitizer fixtures pass for both codecs and both Building
settings. A differential against the release decoder checks 150,001
unaligned rows, including 467 accepted rows. All 149,534 rejected rows
leave the caller's destination untouched in both builds.

The paired normal T6 test starts in a fresh process with Raynor aboard
a Dropship at frame 19,466. It observes the same 27 controller and
dialogue cuts, wins at frame 23,536 with Raynor and two Dropships at
extraction, and reloads the frozen result through a second fresh
title. Both runs export identical 32 KiB victory journals and
result readbacks, including the unchanged first pause bank.

| Event | Release guest cycles | Candidate guest cycles | Reduction |
| --- | ---: | ---: | ---: |
| Paused-game title load | 24,091,103 | 21,508,948 | 10.72% |
| Victory autosave update | 30,193,167 | 26,815,427 | 11.19% |
| Frozen-result title load | 26,454,610 | 23,872,479 | 9.76% |

The candidate links with the same IWRAM and EWRAM endpoints as the
release. This measures specific save and load stalls in emulation.
Run the remaining campaign journal cases and check hardware timing
before considering default activation.

| Input or receipt | SHA-256 |
| --- | --- |
| Release Terran ROM | `c9845065be3e4ec276ac4a94b12762c6eae79cdc835f8ffaa7d431bcc70c7cb1` |
| Candidate Terran ROM | `a07978856d278f636a881de0244cf12548472fff36d0f8b0ae11b4b2140c8345` |
| Paired timing evidence | `b9c6cf96c020b0b5847312da18d1b0aed8fddc18e28870f7147e81fa4a007bab` |
| Victory and result readback SRAM | `af0fa049ba02103ef3944a3f8f324b45c9eb0abc5b8db23318d70969713a87bc` |
