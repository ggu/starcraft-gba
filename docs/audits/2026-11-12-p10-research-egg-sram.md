# P10 skirmish saves with both research and Eggs

The normal P10 build and the Egg preview previously used mutually exclusive
save layouts. This opt-in P10 build recognizes both existing predecessors
and writes a combined custom tag. The outer tag `0x3645505f` combines the
existing Egg feature selector with the research-jobs namespace. The inner
version stays the established Egg version. Egg actors use the same unit
row; completed research uses the same building padding and paid jobs the
same producer row. Campaign saves keep their original pair of tags.

The reader authenticates an exact outer/inner pair in detached scratch
before interpreting either feature. Normal research tags grant no Egg
identity; an Egg-only tag does not grant research. The writer and reader
use the same six accepted outer tags while keeping the existing two SRAM
slots. No fields or SRAM bytes are added.

## Host and cartridge evidence

The sanitized fixture buys a Drone Egg for a Zerg opponent, finishes a paid
Shields level and starts paid Forge weapons research. A separate process
reloads the Egg, level and job; the Egg hatches and the job keeps its
original cancellation refund. Both authentic normal research and Egg-only
HOST writers migrate into the new slot without changing the previous slot.
Three checksum-valid malformed newer records fall back to the older slot
and, without that fallback, reject without changing actors, research,
wallets or SRAM. A separate campaign title/start/save/load fixture retains
the old inner and outer tags. The custom fixture payload remains 16,308
bytes in all three writers.

The normal-menu ARM replay built a Pylon, Assimilator and Forge and paid
for weapons research in a real Outpost match against one Zerg AI. At game
frame 3,005 the Forge had 4,999 ticks left and the AI's Drone Egg had 10.
The cartridge wrote its own 32 KiB SRAM through Start/B. A fresh emulator
process loaded it with L, restored the observed lobby, wallets, actors
and timers, and advanced both timers on the next neutral update. A second
continuation from those same bytes hatched the Drone and completed level
one at frame 8,005 with no loss. The same experimental cartridge also
loaded and migrated a genuine earlier research-only ARM save, preserved
its old 16 KiB slot byte-for-byte, and reloaded the migrated job. These
runs use physical buttons and named reads, with no guest-memory writes.

The experiment ROM is 32 MiB, SHA-256
`aec7e6d10d03020ff1853b53dfd94115b9e320d2179e1f8a9e9d3b6969ac292e`.
EWRAM ends at `0x0203c0c8`; IWRAM ends at `0x03004ba8`. Both original
linker guards pass. The mixed cartridge SRAM SHA-256 is
`7d16e9027495cf04ad7a80c5e4f58692e5235b6a96b66c2fede4ffc42d062b81`.
No ROM, private input tape, saves or derived assets are included here.

```sh
make test-sc64-protoss-research-egg-probe
make protoss-research-egg-probe
```

The separate experimental output is under
`build/protoss-research-egg-probe/`. Keep normal flags off pending wider
custom and carried-campaign checks, older formats beyond the tested
predecessors, stack and busy-frame measurement. This does not extend Forge
research to other cartridge products or qualify Egg/Guardian campaign
activation.
