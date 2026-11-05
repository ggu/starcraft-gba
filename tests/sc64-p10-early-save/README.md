# P10 early physical save probe

`physical.py` exercises one early neutral save point in P1–P7. It requires a normal combined P10 ROM and matching ELF, plus a physical runner with read-only access to the modal count at VRAM `0x06013fe0`. Inputs select the mission and acknowledge visible modal pages, advance 30 gameplay frames, save with START/B, and resume. A fresh runner loads exported 32 KiB SRAM with L on HOME. The script compares saved/restored and uninterrupted/resumed states at 120 further gameplay frames. It records scheduled inputs and hashes the runner, ROM, ELF, and SRAM. Use a new output directory per run. Run under the host's native-build lock if other native jobs share resources.

The comparison covers selected live unit and building fields and active queues, mineral fields and resource banks, frame, mission, supply, campaign state, P3–P7 mission persistent structures, and P7 projectile storage when those symbols exist. It does not compare every serialized field. Scratch world contexts, transient UI, path caches, render/audio state, and other actor or effect systems are outside the fingerprint. Neutral continuation and one save point do not prove victory, arbitrary-save coverage, or all source mechanics.

Example (repeat `--mission` from 1 through 7, changing the output path):

```sh
python3 tests/sc64-p10-early-save/physical.py \
  --runner /path/to/physical-runner --rom /path/to/product.gba \
  --elf /path/to/product.elf --out /tmp/p10-p1-evidence --mission 1
```

`migrate.py` accepts a successful `physical.py` output directory as
`--baseline` and a new product's `--runner`, `--rom`, `--elf`, and new `--out`.
It authenticates the saved SRAM and baseline snapshot fingerprints before
loading the unmodified physical SRAM in a fresh process. It compares the
restored and continued states to the old product's recorded snapshots and
checks the exported SRAM remains unchanged. This tests only the baseline's
specific save and continuation; it does not create or alter SRAM records.

The old P10 baseline's exploratory raw world-context comparison differed in
three places at restore. DWARF maps P5 offset 42 to the `u8`
`objective_refresh_requested` field (`sc64_p5_world_adapter.h:21`): opcode 12
sets it at :475, but there is no reader, and the action records objective stage
in `sc64_p5_runtime_adapter.h:506–513`. P6 offset 40 is the `u32`
`elapsed_frames` field (`sc64_p6_world_adapter.h:36`), populated from the
campaign clock each tick (`sc64_p6_runtime_adapter.h:700`). P7 offset 44 is
its corresponding `u32` clock (`sc64_p7_world_adapter.h:50`), populated each
tick (`sc64_p7_runtime_adapter.h:289`). P7 offsets 64–92 are the eight `u32`
`count` entries of `Sc64P7CountSnapshot` (`sc64_p7_world_adapter.h:16,58`),
recomputed for each tick at :68–121 and deactivated at :124–125. The snapshot
active byte did not differ. These facts motivate comparing persistent mission
state and a finite continuation rather than raw contexts; they do not prove
that all excluded effects or other save points are safe.

The required backend source is [tools/sc64_headless_replay.c](../../../tools/sc64_headless_replay.c),
built against pinned mGBA 0.10.5 with debugger support (see its header). Its
physical mode accepts only bounded controller input, stepping and reads, with
SRAM import at boot and export to the explicitly selected output. Palette RAM
and VRAM reads are allowed so the probe can observe modal state; MMIO reads
and physical-mode guest writes remain denied. Use that source rather than an
older backend that rejects VRAM. The source matches the previously qualified
read-only backend used for these results; the backend is not an included
proprietary asset or opaque replacement game ROM.

For an existing pinned mGBA 0.10.5 core-only static build with debugger support,
the macOS compile/link shape used by the qualified backend was:

```sh
clang -std=c11 -O2 -DM_CORE_GBA -DUSE_DEBUGGERS -DBUILD_STATIC \
  -I/path/to/mgba/include -I/path/to/mgba-build/include \
  tools/sc64_headless_replay.c /path/to/mgba-build/libmgba.a \
  -framework CoreFoundation -o /tmp/sc64-physical-runner
```

The source revision used for those results was mGBA
`26b7884bc25a5933960f3cdcd98bac1ae14d42e2`. Platform link dependencies can differ;
this is the recorded macOS shape, not an installer or a request to fetch game
inputs. The asset-free `test_sc64_headless_read_policy.py` compiles the actual
classifier and checks region edges, ROM length, overflow, SRAM and MMIO denial.
It complements the actual read/state/clock observations rather than proving
whole-emulator correctness on its own.
