# Three-opponent battle pacing

On release `815f173e`, load the verified mid-battle save from each
[three-opponent custom match](2026-11-26-n64-gas-pickup.md) on its race ROM.
Resume the original controller inputs for 256 main-loop updates after the
recorded resume checkpoint. The table uses guest cycles, including time spent
presenting. Every window matched its recorded gameplay checkpoints. Sampling
the front buffer produced the same individual cycle deltas and final state
fingerprint as repeating the window with video sampling disabled.

| Human | Map | Loop updates/s | Bitmap changes/s | P95 cycles | Over two VBlanks | Orders cycles | Actor drawing cycles |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Terran | Outpost | 14.59 | 12.19 | 1,818,131 | 253/256 | 395,400 | 297,786 |
| Terran | Blood Bath | 16.73 | 16.41 | 1,480,304 | 254/256 | 343,577 | 215,509 |
| Zerg | Outpost | 11.32 | 10.92 | 1,933,833 | 255/256 | 683,731 | 256,775 |
| Zerg | Blood Bath | 11.29 | 11.20 | 1,988,034 | 256/256 | 612,150 | 369,968 |
| Protoss | Outpost | 14.00 | 8.91 | 1,616,613 | 253/256 | 479,926 | 268,634 |
| Protoss | Blood Bath | 15.27 | 14.14 | 1,498,325 | 255/256 | 435,053 | 141,837 |

The last two columns average only updates that advanced the simulation.
The actor timer includes buildings, effects and unit drawing in the shared
depth renderer. These stages sit inside the simulation and drawing totals.
Bitmap changes count successive changes to the displayed Mode 4 bitmap,
resolved through the active palette, sampled at input boundaries. They do
not count scans of the same bitmap, object overlays or physical LCD response.
The sampler requires BG2 enabled and forced blank off. It reads untransformed
bitmap pixels before any affine, window or blending effects.

In the Zerg Blood Bath window, 256 of 256 updates exceeded the 561,792-cycle
two-VBlank budget. Prioritize orders and actor drawing together. A reduction
in just one stage must still improve total cycles on the recorded controls.
Retain both page images, palette, visible page selection and gameplay checks
when deciding whether a candidate can ship.

## Repeat a measurement

The full-match matrix writes the trace and battle save under the case
directory. Use its matching ROM, ELF and the physical mGBA runner:

```sh
python3 tools/sc64_custom_full_match.py pace \
  --trace "$case_dir/trace.json" --save "$case_dir/replay/pause.sav" \
  --rom "$rom" --elf "$elf" --runner "$runner" \
  --output "$new_output_dir" --updates 256 --video
```

The output includes the ROM, ELF, runner, trace and save hashes, per-update
cycles, simulation frame and bitmap-change observations. A different ELF
can change symbol addresses; always use the ELF linked with the tested ROM.

## N64 observation

A private ares build at `0aafd857` loaded an unmodified USA Blood Bath
state with 130 active unit records. Over 1,000 active VI fields with neutral
input, the native game counter at `0x800B1E84` advanced from 68,224 to
68,523. That is 0.299 native game steps per emulated field. At a nominal
60 fields/s this corresponds to 17.94 native game steps/s. The capture
counted emulated fields independently of host elapsed time and frontend
callbacks. The N64 and GBA battles have different actors and inputs, and the
two engines count simulation steps differently. Keep 30 fps as the GBA
presentation target and measure game-rule timing independently.

A second run waited for each asynchronous image callback before advancing
the next field. It yielded 1,000 progressive 640×240 RGBA images over the
same counter interval (VI serrate off). Every image hash differed. In 852
of 999 successive pairs, no RGB channel changed by more than four levels.
Exact image changes here include small color variation; they cannot identify
the number of distinct world updates or physical display swaps.

## Optimization trials

On Protoss Blood Bath against two CPUs, building the initial actor heap
bottom-up saved 0.046% across 512 updates with recorded controls. Removing
a repeated enemy visibility check saved 0.041% in the same window. Both
matched the five gameplay checks and endpoint fingerprint. In a separate
2,048-update fixed-camera run, both matched both framebuffer pages, palette,
display selection and the pause save at the tested checkpoints. Trimming
transparent edges in the packed sprite compositor *increased* whole-run
cycles by 1.27% on that workload.

On Zerg Blood Bath against three Zerg CPUs, processing two entries per
route-flood iteration changed total time by less than 0.001% over 1,000
neutral updates from the battle save. Packing dense-point fields in aligned
words saved 3.49% inside the flood in an 80-update instruction profile, but
only 0.043% across 1,000 updates and added 344 bytes of IWRAM. Neither
experiment warrants changing the shared engine on this evidence.
