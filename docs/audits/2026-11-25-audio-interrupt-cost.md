# Cost of the sampled-audio interrupt

The VBlank interrupt that decodes and mixes the StarCraft 64 sound bank
runs on every video frame in every mode. On `7316734e` its mean over a
window was 5,700 to 6,500 CPU cycles per VBlank with only music playing
and up to 26,700 in late three-computer Custom Games, where an update
spans three to four VBlanks. This change brings those means to
3,000–3,400 and up to 14,800 cycles, and the longest single run from
29,800 to 16,900, with the same bytes written to the two Direct Sound
buffers.

All figures are from mGBA with an instruction-level cycle profiler in a
private runner. Nothing here was measured on hardware or listened to.

## What changed

- **Stream decoder.** The two linear predictors, which are all the bank
  uses, decode in `audio_decoder_linear` (`common/audio_codec.h`): one
  loop that takes the residual width as a register, written by hand as
  ARM code and placed in IWRAM. It is 460 bytes. It replaces two Thumb
  state machines of 2,072 and 2,520 bytes that ran from ROM with one
  unrolled block body per width. The third predictor keeps its Thumb
  state machine.
- **Voice sum.** Two to four voices now decode into the columns of one
  table that interleaves them word by word, with each sample's top bit
  already flipped, so `audio_mixer_sum` reads all voices through one
  pointer. Its body is hand-written Thumb with every constant in a
  register. The compiler's loop reloaded three constants from ROM for
  each word. A voice that ends inside a frame is padded with silence, so
  the frame is summed in one pass instead of span by span.
- **Silence.** A frame without voices is written once to each half of
  the mix buffer and then left alone.
- **Reference code.** Each hand-written body sits beside a C function
  that defines it. Host builds run the C; ARM builds run the assembly.

`common/gba.ld` now keeps IWRAM code below `0x03005200` instead of
`0x03005000`. See [IWRAM and the stack](#iwram-and-the-stack).

## Cycles per VBlank

Twelve windows per build: 150 updates from each late-battle save (10
updates after resuming it), 150 updates from a save made 30 updates into
a match, and 200 updates on the title screen. "Interrupt" is measured
from entry to `audio_vblank` to its return and equals the four parts.
"Rest" is the handler's own code: the request queue, the voice table,
the music sequencer, two clock reads, the DMA re-arm and the page flip.
The last two columns are the ROM's own `audio_irq_cycles` /
`audio_irq_frames` and the largest single value in the window.

Before (`7316734e`):

| Window | Voices | Interrupt | Music decode | Effect decode | Mix | Rest | ROM counter | Peak |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Terran, Outpost late | 3.00 | 26,623 | 4,684 | 14,046 | 6,137 | 1,756 | 26,364 | 29,755 |
| Terran, Blood Bath late | 1.32 | 12,568 | 4,195 | 5,537 | – | 2,836 | 12,308 | 19,800 |
| Zerg, Outpost late | 0.00 | 5,833 | 4,054 | 0 | 0 | 1,779 | 5,582 | 6,160 |
| Zerg, Blood Bath late | 2.37 | 21,468 | 4,566 | 10,719 | 3,406 | 2,777 | 21,214 | 29,076 |
| Protoss, Outpost late | 2.98 | 26,702 | 4,765 | 14,171 | 5,946 | 1,820 | 26,449 | 29,793 |
| Protoss, Blood Bath late | 0.32 | 7,523 | 4,497 | 1,435 | – | 1,591 | 7,270 | 19,501 |
| Terran, early match | 0.00 | 5,840 | 4,040 | 0 | 0 | 1,800 | 5,566 | 5,965 |
| Zerg, early match | 0.00 | 5,723 | 3,922 | 0 | 0 | 1,801 | 5,448 | 6,063 |
| Protoss, early match | 0.00 | 6,344 | 4,559 | 0 | 0 | 1,786 | 6,070 | 6,632 |
| Terran, title | 0.00 | 6,442 | 4,646 | 0 | 0 | 1,796 | 6,167 | 6,772 |
| Zerg, title | 0.00 | 6,442 | 4,646 | 0 | 0 | 1,796 | 6,167 | 6,772 |
| Protoss, title | 0.00 | 6,466 | 4,670 | 0 | 0 | 1,796 | 6,191 | 6,797 |

The compiler inlined the two-voice sum into the handler on this
revision, so the two "–" windows carry their mixing under "Rest".

After:

| Window | Voices | Interrupt | Music decode | Effect decode | Mix | Rest | ROM counter | Peak |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Terran, Outpost late | 3.00 | 14,776 | 2,583 | 7,732 | 2,863 | 1,598 | 14,514 | 16,885 |
| Terran, Blood Bath late | 1.35 | 7,495 | 2,363 | 3,173 | 885 | 1,075 | 7,235 | 11,203 |
| Zerg, Outpost late | 0.00 | 3,117 | 2,325 | 0 | 0 | 792 | 2,866 | 3,207 |
| Zerg, Blood Bath late | 2.37 | 12,177 | 2,540 | 5,990 | 2,291 | 1,357 | 11,922 | 16,084 |
| Protoss, Outpost late | 2.93 | 14,657 | 2,609 | 7,622 | 2,840 | 1,585 | 14,402 | 16,763 |
| Protoss, Blood Bath late | 0.32 | 4,157 | 2,478 | 796 | 39 | 844 | 3,903 | 11,093 |
| Terran, early match | 0.00 | 3,100 | 2,288 | 0 | 0 | 812 | 2,826 | 3,094 |
| Zerg, early match | 0.00 | 3,047 | 2,235 | 0 | 0 | 812 | 2,773 | 3,096 |
| Protoss, early match | 0.00 | 3,311 | 2,499 | 0 | 0 | 812 | 3,036 | 3,262 |
| Terran, title | 0.00 | 3,383 | 2,570 | 0 | 0 | 813 | 3,108 | 3,380 |
| Zerg, title | 0.00 | 3,409 | 2,595 | 0 | 0 | 813 | 3,133 | 3,406 |
| Protoss, title | 0.00 | 3,383 | 2,570 | 0 | 0 | 813 | 3,108 | 3,380 |

The interrupt costs 40–48% less in every window, and its largest single
run falls from 29,793 to 16,885 cycles. Decoding costs 12.7–14.8 cycles
per sample, down from 22.3–27.1. The sum costs 5.4–6.6 cycles per voice
sample from ROM, down from 8.8–11.6.

## Cycles per update

Paired runs of the two ROMs from each late-battle save, 300 updates
each. Every update compares the world-state fingerprint, the camera,
both framebuffer pages, the palette and the unit and building tables;
the run ends with a pause save. All six pairs match in every update and
in the saved image.

| Save | Before | After | Change | Updates per second |
| --- | ---: | ---: | ---: | --- |
| Terran, Outpost | 773,910 | 742,009 | −4.12% | 21.68 → 22.61 |
| Terran, Blood Bath | 870,175 | 848,327 | −2.51% | 19.28 → 19.78 |
| Zerg, Outpost | 1,057,283 | 1,046,833 | −0.99% | 15.87 → 16.03 |
| Zerg, Blood Bath | 1,021,878 | 984,767 | −3.63% | 16.42 → 17.04 |
| Protoss, Outpost | 973,020 | 929,075 | −4.52% | 17.24 → 18.06 |
| Protoss, Blood Bath | 1,002,396 | 988,653 | −1.37% | 16.74 → 16.97 |

Every update in these windows is still over the 561,792-cycle budget.

## Unchanged output

- **Host, whole bank.** `make test-audio-bank` decodes all 534 streams
  (16,111,336 samples) with the C decoder and matches the manifest's
  SHA-256. It then plays the bank through the mixer
  (`tests/audio_bank_mix.c`): every clip to its end with one to four
  voices, every track, then 30,000 frames of random requests. Each of
  the 76,783 frames equals a model that decodes bit by bit from the
  format's description and adds samples one at a time. The same test
  built against the previous headers prints the same digest of all
  frames, `e4cc15217e3746a7`.
- **ARM, whole bank.** `make check-audio-arm-probe` builds a ROM that
  runs each hand-written routine against its C reference on the
  ARM7TDMI and judges its result. All 534 streams agree after every
  frame, as do the decoder states; each frame takes the next of ten
  output layouts (packed, and each column of 2, 3 and 4 wide tables).
  Its running sum of the decoded samples, `c18b8cd4 bb236693`, equals
  the one the host test prints. 400 pseudo-random bit streams (704,000
  samples) agree as well; they reach reserved width codes and other
  input the pack tool never writes, and two of them are the longest a
  stream can be. The voice sum agrees on 3,000
  tables, in which 115,514 words hold a sample at either limit. A
  deliberately broken clamp and a broken refill both fail the probe.
- **ARM, the twelve windows.** A capture records, at every VBlank since
  reset, the requests waiting in the queue and the 832 bytes of both
  DMA double buffers after the handler returns.
  `tests/audio_bank_timeline.c` replays such a timeline on the host. A
  replay through the previous mixer and decoder reproduces every byte
  the new ROMs wrote in all twelve windows (401 to 760 VBlanks each),
  and every byte the previous ROMs wrote in theirs. Late battles run
  faster now, so requests reach different VBlanks and the two ROMs'
  captures cannot be compared directly there. On the three title
  screens the timing is the same, and the two ROMs' captures are
  byte-identical for all 474 or 475 VBlanks.

## Other builds and the merged tree

Only ROMs that link the sound bank compile the hand-written routines.
The default original-art ROM is byte-identical with and without this
change (SHA-256 `cc7340a7…5aed177cda`).

The tables above are from `7316734e` with this change. The branch was
then merged with `codex/source-release`, which by then included the
IWRAM placement lists. The three race ROMs built from that merge link
through `common/gba_hot.ld`, pass its stack check (304, 592 and 828
bytes spare; IRQ stack 264 of 416 bytes) and were captured again in the
six late windows: every byte equals the replay through the previous
mixer and decoder, and the means are within 40 cycles of the table in
[With the IWRAM placement lists](#with-the-iwram-placement-lists).

Every other ROM that links the bank was linked from the merge:
backwater, skirmish, races, Zerg 3, 4, 6, 7 and 8, Protoss 3, 6, 7, 8
and 9 and the Showcase. Five of them now end IWRAM above the old
`0x03005000`, so their stack reserve is below the old 11,776 bytes. For
those five, the placement checker was run on a link through
`common/gba_hot.ld` with an empty list:

| ROM | IWRAM ends at | Stack bound | Spare below the new reserve |
| --- | ---: | ---: | ---: |
| Zerg 6 | `0x03005120` | 5,496 | 5,480 |
| Zerg 8 | `0x03005030` | 5,424 | 5,792 |
| Protoss 6 | `0x03005048` | 5,440 | 5,752 |
| Protoss 9 | `0x03005018` | 5,904 | 5,336 |
| Showcase | `0x03005078` | 5,504 | 5,640 |

The checker needs the live activations of each call cycle declared.
Every cycle in these five ROMs is one that the three shipping lists
declare, or part of one: the building/add-on recursion (fingerprint
`c1c7a2ed`, 2), the Nydus recursion (`c6a68339`, 2) and the trigger and
count-cache dispatcher cycles (1). They were declared with those
counts. That carries the lists' reading of the code over; it was not
read again for these ROMs.

Not built here because their private inputs are absent from this
worktree: `verify-sc64` (Wasteland mission data) and `verify-campaign`
(campaign header).

`make test-unified` on the first merge stopped in `test-unified-hud`,
which `codex/source-release` fixed in `53cfd74a`. On the current merge
it passes.

## IWRAM and the stack

The decoder needs 460 bytes of IWRAM. On `7316734e` the three ROMs had
1,160 (Terran), 48 (Zerg) and 248 (Protoss) bytes free below the
`0x03005000` guard, so the guard moves to `0x03005200`. That leaves
11,264 bytes for the main stack, the 11 KiB the guard's message has
always named, instead of 11,776.

| | Terran | Zerg | Protoss |
| --- | ---: | ---: | ---: |
| IWRAM ends at, before | `0x03004B78` | `0x03004FD0` | `0x03004F08` |
| IWRAM ends at, after | `0x03004D48` | `0x03005198` | `0x030050D0` |
| Main stack bound | 7,808 | 6,076 | 5,940 |
| Lowest stack address touched in the two late windows | `0x0300670C` | `0x030067C8` | `0x030068A8` |
| ROM text bytes, before | 30,714,840 | 31,511,620 | 31,513,276 |
| ROM text bytes, after | 30,710,544 | 31,507,324 | 31,508,972 |

The stack bound is the one `tools/gba_iwram_hot.py check` computes,
run on these ROMs' objects. With its 512-byte margin the tightest ROM needs 8,320
bytes. The lowest address touched is the same before and after.

The interrupt has its own 416-byte stack. Its deepest chain was 264
bytes of compiler frames (`audio_irq`, `audio_vblank`,
`audio_decoder_run`, `audio_decoder_run_p1`). It is now 224, through
the third predictor's state machine, and 168 with the bank's own
streams. The same check reports 264 of 416 bytes, where it reported 296.

Three replay probes paint the unused stack from the old guard address
and one verifier checks against it. They now use `0x03005200`:
`tests/sc64_z7_arm_replay.c`, `tests/sc64_p7_normal_input_replay.c`,
`tests/sc64_impact_replay.c` and `tools/verify_sc64_egg_preview.py`
with its test. Their reported stack margins are 512 bytes smaller for
the same stack use.

### With the IWRAM placement lists

The race cartridges now link listed ROM functions into IWRAM through
`common/gba_hot.ld` and check the stack of the result
([placement audit](2026-11-25-iwram-hot-placement.md)). That work was
still on its own branch when these measurements were taken; a private
link of this change with its linker script, tool and lists, which
merged unchanged, passes the check on all three ROMs and leaves 296
(Terran), 364 (Zerg) and 828 (Protoss) bytes spare. The Terran and
Protoss lists already name `audio_mixer_sum` and pick up the new
function. With that link the 0x03005200 guard in `gba.ld` no longer
applies to these three cartridges; it applies to every other ROM that
links the bank, and to the `SC64_IWRAM_HOT=0` comparison builds.

| Window | Lists only | Lists and this change |
| --- | ---: | ---: |
| Terran, Outpost late | 24,567 | 13,853 |
| Terran, Blood Bath late | 13,439 | 7,584 |
| Zerg, Outpost late | 5,861 | 3,097 |
| Zerg, Blood Bath late | 21,973 | 12,516 |
| Protoss, Outpost late | 24,491 | 13,797 |
| Protoss, Blood Bath late | 7,559 | 4,173 |

Paired 300-update runs of those two links from the six saves match in
world state, pixels and saves, and the mean update is 0.99–4.03%
shorter. From IWRAM the sum costs 3.5–4.1 cycles per voice sample.
Adding `audio_mixer_sum` (272 bytes) to the Zerg list as well takes the
Zerg Blood Bath window from 12,516 to 11,706 cycles per VBlank, passes
the check and leaves 76 bytes spare. That list is not part of this
change.

## Tried and rejected

- **Music stored without compression.** It would remove the music
  decode, 2,235–2,608 cycles per VBlank after this change. The music is
  8,231,520 bytes raw against 3,798,692 encoded, 4,432,828 more, and
  the ROMs have 2.84 (Terran) and 2.05 MB (Zerg, Protoss) left of 32
  MiB.
- **The same loop left to the compiler.** GCC 16.2 turns the C
  reference into 876 bytes of ARM with an 80-byte frame and computes
  both predictors for every sample. That is twice the IWRAM for a
  slower loop.
- **The decoder as ARM or Thumb code in ROM.** A ROM fetch costs two
  cycles per Thumb instruction and four per ARM instruction, and a taken
  branch about eight. An earlier trial of the first-order state machine
  as ARM code from ROM
  [cost 2.33% more](2026-11-22-native-custom-game-lobby.md#first-order-audio-on-arm).
  A Thumb version of the compact loop was not built: by instruction
  count it would run no faster from ROM than the state machines it
  replaces.
- **A separate assembly file.** The first working version kept both
  routines in `common/audio_bank.S`. It measured the same, but the
  placement stack check (`tools/gba_iwram_hot.py`) needs the compiler's frame
  record for every linked function. The bodies are now inline assembly
  in C functions, which costs 24 bytes of IWRAM and passes that check.
- **A C helper for clipping words.** Calling one from the middle of the
  assembly left the stack 4 bytes off the 8-byte alignment a C function
  may assume. The helper is assembly with no stack use.
- **A case for width 0.** A silent first-order block was a four-store
  fill. The general block handles it correctly, and removing the test
  saved 36 bytes and two cycles in every other block. Silent blocks
  cost about 70 cycles instead of 20.
- **More speed for more IWRAM.** The per-instruction profile of the
  decoder (about 117 cycles per block of eight samples in the Terran
  Outpost window) splits into 52 for the samples, 19 for word refills,
  19 for width headers and 27 of loop control. By that profile,
  unrolling the two halves of a block would save about 6 cycles per
  block for 160 bytes and one loop per predictor about 3 for 44 bytes.
  Neither was built. Reading a block's residuals as one field and a
  fast path for width changes would save nothing on these streams:
  more than half the fields already end in a word refill, and more
  than half the blocks change width.
- **Decoding music in larger batches.** A decoder call costs about 60
  cycles of setup against 2,300 for a music frame, so there is nothing
  to amortise.

## Release checks with the race cartridges relinked

On the #882 release layout, each race ran a late three-CPU battle in
mGBA for 150 updates after ten warmup updates. A second run cycled
friendly selections during combat to play speech alongside three effects.
Both DMA buffers matched the previous mixer on every captured request
timeline in all six candidate runs. The control ROMs passed the same
check. Control and candidate spent different numbers of VBlanks in a
window, so the cycle counts are workload means.

| Race and map | Neutral IRQ cycles per VBlank, control → candidate | Candidate VBlanks with four voices | Four-voice IRQ cycles per VBlank, control → candidate | Main stack spare beyond 512-byte margin |
| --- | ---: | ---: | ---: | ---: |
| Terran Outpost | 24,567 → 13,848 | 38 | 28,585 → 16,555 | 304 bytes |
| Zerg Blood Bath | 21,999 → 12,486 | 66 | 31,716 → 18,227 | 592 bytes |
| Protoss Outpost | 24,459 → 13,790 | 37 | 29,032 → 16,573 | 828 bytes |

The three build-time IRQ stack bounds fell from 304 to 264 bytes of
416. Showcase retains the standard linker layout. On its title screen,
334 consecutive VBlanks produced byte-identical DMA buffers between
the two ROMs. The mean over 300 VBlanks after warmup fell from 6,505
to 3,410 cycles.

### Whole-game cost on the relinked candidate

After ten warmup updates, the next 300 neutral updates from a late
three-CPU save on each race cartridge gave the following guest cycles.
The orders counter is part of the update cost. These are different
battles and the counters include interrupts.

| Race | Mean whole update and render | 95th percentile whole update and render | Mean orders | Mean draw |
| --- | ---: | ---: | ---: | ---: |
| Terran | 795,178 | 938,620 | 331,967 | 343,500 |
| Zerg | 970,496 | 1,230,420 | 430,019 | 378,323 |
| Protoss | 790,667 | 959,557 | 392,932 | 238,257 |

The two-VBlank budget is 561,792 guest cycles. All three battle windows
exceeded it on average. Across the longer 1,024-update comparisons on
these saves, the audio change reduced total cycles by 1.82%, 0.99% and
2.02%, respectively. Both framebuffer pages, the palette, sampled world
and camera state, and the exported pause save matched each control.
Showcase also matched in an early battle. In the Zerg 7 campaign's
4,901-input battle window, all 72 sampled world and video checkpoints
matched; throughput rose from 8.72 to 8.82 updates/s.

## Limits

- Emulator measurements only. mGBA charges no extra time for CPU access
  to VRAM during display, which hardware does in some cases; the mix
  and music buffers, the table and the decoder states are in VRAM, as
  before.
- The decoder relies on the ARM7TDMI's defined results for shifts by a
  register holding 32 (zero for `LSR`, the sign for `ASR`) and a rotate
  by 0. mGBA implements them and the probe exercises them; no cartridge
  was tested.
- The sound was not listened to. The claim is that the bytes reaching
  the two DMA buffers are the same.
