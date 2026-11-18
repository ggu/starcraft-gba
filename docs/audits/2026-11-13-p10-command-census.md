# Eye of the Storm: trigger census and gameplay stalls

The full ordinary HOME replay of the original P10 cartridge wins at
simulation frame 46,057 after 47,194 calls. A second replay with a read-only
observer matches all 20 gameplay observations, ten briefing reveals,
14 authored message pages, and the timing counters. It takes 90,760,076,029
guest cycles, or 1,923,127 per call (8.72 updates/s at the GBA CPU clock).
This includes menu, presentation, and waits; it is not a hardware frame-rate
measurement. The target is 561,792 cycles per update.

The final result transition is the 76,630,638-cycle global maximum.
The instruction trace attributes 71,802,053 cycles to `race_save_game`
under `game_finish_result`, including 52,008,246 in repeated typed unit
decoding across the save path. The largest ordinary gameplay update is call 11,045, frame 10,803: 45,240,570
cycles by the loop counter. A separate instruction trace attributes
41,944,546 cycles to `sc64_route_facility_next` and 41,926,044 within it
to `sc64_route_facility_slot_for_origin`. Call 9,674 similarly spends
39,618,161 cycles on facility routing. These are workers searching for
reachable depots; the 128-column point-field FIFO does not handle them.
A depot candidate must preserve ordered deposit roots, exact partial
field contents and early-stop behavior, cache effects, and the live
walkability grid. A fill that always completes changes cache behavior.

In sixteen sampled ordinary battle updates across three windows, mean cost
is 1,760,018 cycles. The instruction trace attributes 935,221 cycles to
`game_update` and 768,090 to `game_render`. Selected inclusive work
includes movement orders (490,123), exploration (178,306), the unit draw
pass (156,088), and defense targeting (78,343); the scopes can overlap.
Two other sampled updates spend about 1.7 million cycles each evaluating
authored Command conditions (calls 5,467 and 42,478). The bridge fetches
full actor records and clears one for every slot even though the count
does not use cargo, invincibility, or positions.

## Direct shared Command census

The `SC64_Z8_DIRECT_COMMAND_CENSUS` provider reads the live
unit and building pools in their original order. It omits dead slots and
avoids filling unused actor view fields. It still validates owner, raw
type and geometry for every live actor, even when the actor cannot match
the condition, and sees all earlier trigger mutations. Bring, Kills and
replacement actor providers use the original path. It changes no trigger
schedule, persistent state, or save layout.

The common game-pool binder installs the provider for P8, P9, P10 and
Z8. P8's live-slot wrapper continues to compact Bring only, while P9
retains its location-based path. The late Zerg Z9/Z10 mission provider
special-cases the carried crystal and continues using generic actor
enumeration; their cartridges also carry the eligible Z8 mission.
The standalone P8, P9 and Z8 products have different content/feature
flags; in particular, the Zerg bridge can see an in-progress Egg.
A separate ASan/UBSan differential built against the canonical Z10
configuration compares the Z8 game pools and exercises the Egg branch:
AtMost includes the in-progress Egg, other comparisons exclude it, and
the next query sees the hatched unit. The original ARM comparison below
covered P10; the current-build checks are recorded separately.

The ASan/UBSan differential uses the actual P10 pools and compares 66,816
queries over owners, forces, raw types, group types and comparisons. It
also covers same-slot creation, removal, ownership and building completion
changes, malformed live rows, untouched output on faults, and replacement
provider fallback. Run `make test-sc64-z8-command-census` with prepared
P10 inputs and `make test-sc64-z10-command-census` with prepared Z10 inputs.

The ARM comparison uses the icon-scaling cartridge (runtime source
`cbba1c5`) and the census variant (same runtime on merged source
`cb20848`, plus census commit `344ecaf`). It replays 11,300 calls from
HOME with physical key input and reads guest state without writing it.
Both match ten briefing records, seven authored message pages, and
five ordered host gameplay observations. All 21 sampled displayed pages,
palette, unit/building/mineral pools, fog, camera cursor, frame, campaign
phase and route cache observations match exactly.

| Scope | Control cycles | Census cycles |
| --- | ---: | ---: |
| Mean, 11,300 calls | 1,624,994 | 1,483,602 |
| Call 5,467, complete loop | 3,337,072 | 2,169,986 |
| Call 5,467, P10 condition evaluation | 1,700,111 | 562,688 |
| Call 5,475, complete loop | 3,235,282 | 2,061,171 |
| Call 5,483, complete loop | 3,284,812 | 2,123,080 |
| Call 9,674, depot stall | 42,470,903 | 42,404,672 |
| Call 11,045, depot stall | 45,253,308 | 45,298,148 |

Mean cycle cost decreases 8.70% in the prefix (10.32 to 11.31
emulated updates/s). A full replay against the existing icon-control
receipt then wins at the same frame 46,057 and call 47,194. All ten
briefing records, 14 authored message pages, 20 ordered world
observations and terminal campaign flags match. The full candidate
also reproduces all 21 paired prefix video/state samples. Full mean
cycles decrease from 1,907,674 to 1,766,079 (7.42%, or 8.79 to 9.50
emulated updates/s). The global peak remains the result transition.
This run does not qualify a fresh-process save/load, hardware behavior,
or the other products. The flag defaults off.

IWRAM ends at the same `0x03004ab0`; the BSS end moves eight bytes to
`0x0203c0e8`, within the existing guards. ROM verification passes.

Control ROM SHA-256:
`7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310`.
Candidate ROM SHA-256:
`122eb131a5deeaeeb9ea25b68954e276a936564559caba762348e04c203de682`.
Paired video/state receipt SHA-256:
`e38da1afadb43098c23eb7f9140fee18e643a1d2beeee0d57704f4bab3bbb3b7`.

## Current pointer FIFO cartridges (November 14)

Builds based on `ecc8a6ac` differ from the released Z10 and P10 vectors
only by the census flag. Both normal ARM links pass the unchanged memory
guards: IWRAM stays at `0x03004d50` (Z10) and `0x03004c10` (P10), and
EWRAM grows by eight bytes to `0x02038b08` and `0x0203c0e8` respectively.
Both game-pool differentials passed again under ASan/UBSan. The stock
actor provider remains available with `CANONICAL_COMMAND_CENSUS=0`.
The flag applies where the current ROMs carry these trigger schedulers.
The normal Terran and Showcase compile vectors do not change.

| Mission on normal ROM | Matched world checks | Win frame | Mean guest cycles per call | Effective emulated updates/s |
| --- | ---: | ---: | ---: | ---: |
| Z8 (Z10) | 19 | 45,468 | 1,206,048 → 1,171,831 (−2.84%) | 13.91 → 14.32 |
| P8 (P10) | 41 | 122,809 | 1,784,079 → 1,687,131 (−5.43%) | 9.40 → 9.94 |
| P9 (P10) | 59 | 144,361 | 1,994,520 → 1,888,568 (−5.31%) | 8.41 → 8.88 |
| P10 (P10) | 20 | 46,057 | 1,862,770 → 1,722,799 (−7.51%) | 9.01 → 9.74 |

All four ordinary-controller replays match the no-census controls from
HOME through victory with the same observed gameplay, authored pages and
campaign outcomes. P8 retains both heroes, P9 clears both Cerebrate
objectives, and all four report zero trigger faults. In a separate P10
pair, a pause save at frame 36,761
is byte-identical. A fresh HOME load restores the same sampled world and
matches five continuation checkpoints through the win. The two result
saves are also byte-identical; the result write adds a journal entry and
preserves the earlier bank. Physical Blood Bath runs on both product
pairs match all 21 tested fields, including the lobby, continuation and
full SRAM bytes. These are emulator checks, not physical GBA timing.

The opt-in P10 Egg/Forge preview from #406 was rebuilt with the census.
Its guarded ARM link, ordinary mixed-job save/reload and completion,
research-only ARM migration, and three sanitizer writers pass. The mixed
save is byte-identical to the prior preview. The original-art ROM also
builds to identical bytes with the census setting on or off.

| Local receipt | SHA-256 |
| --- | --- |
| Z10 census ROM | `8e4737c9085dda0ed6f6084ed6f3150ae7dfd722dd97e466f766d6c6bad20efb` |
| P10 census ROM | `6230e726153ed9dea2fd4f5bbcb5278cbcff1f68f0035855af59ee22626dfae8` |
| Z8 census full win | `c70b8e168d51c8807616751b1f519505a5e69cc56b8f12b02f68604eebd2e085` |
| P8 census full win | `5161c803372b3ed03f2bbfc9b12a6038a0465f670429917272a8e3df472bea8f` |
| P9 census full win | `a64fd58cca64911998e5f0a1342aeab83c6cf1838822a68d9a767005a5830fb7` |
| P10 census full win | `51a278871b6d9e5babb899621152d4b701fe3d9d3b2ae2a3300dda4dc847d0a6` |
| P10 census fresh-load continuation | `0b2554a6ee36952c8d4429e0ce508283b6e78601d7e514ff5ca0b3b88276a82b` |
| Two Blood Bath save and continuation comparisons | `5eaf0452ebe9586358d9df51893b7cd8aa4bac4b15d0b3579591dc2f03e16dfd` |
