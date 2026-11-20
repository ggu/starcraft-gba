# Terran Bunker opening on Blood Bath

Two unmodified USA StarCraft 64 Free for All runs on Blood Bath have a
passive Terran human. Run 4 seats only a Terran computer. Run 1 seats
Zerg, Terran and Protoss computers. The last absent and first present
samples bracket the Bunker events. [Native capture method](2026-11-19-n64-skirmish-ai.md)

| Observation | Native solo frame | Native mixed frame |
| --- | ---: | ---: |
| Second Barracks first present | 3,564 | 3,818 |
| Academy first present | 4,848 | 5,124 |
| Academy first complete | 6,003 | 6,425 |
| First Bunker foundation, raw 125 | (6,922, 7,151] | (8,002, 8,271] |
| First complete Bunker | (7,360, 7,578] | (8,505, 8,737] |
| First Factory present | 8,834 | 12,807 |

Both owners have two completed Barracks and a completed Academy at
placement. The solo owner has 18 completed SCVs. The mixed owner has
17 completed SCVs and one in production. Neither has a Factory.
The solo Bunker is 793 native pixels from its Command Center and the
mixed Bunker is 551 pixels away, both toward the interior. Using the
existing 5/4 source clock, the first foundation samples are updates
8,938 and 10,338 and the completion samples are 9,472 and 10,921.

## Garrison evidence

The unmodified mixed census has one Marine and one Firebat at the exact
completed Bunker center `(1520,1312)` on frame 10,107. A second Marine
joins by 10,357. All three stay there in the 10,600 and 10,856 samples
with order 5. Those census rows contain positions and orders but omit
passenger flags and pointers. The solo census has no corresponding
center matches by its end at frame 10,025.

Run 2 disabled defeat to keep the mixed lobby running. Four Marines
share the first Bunker center at frame 8,408. The retained game-frame
19,444 state proves later occupancy directly. The same Bunker, actor
172 at `0x802e19e0` and position `(720,768)`, has four encoded passenger
IDs at actor offset `0xac`. The passenger pointers at `+0x80` all name
that Bunker, and status `0x86008810` at `+0xd8` sets passenger bit 25.

| Slot value | Passenger actor index | Raw unit |
| ---: | ---: | --- |
| 2,399 | 350 | Firebat, 32 |
| 2,344 | 295 | Marine, 0 |
| 2,371 | 322 | Marine, 0 |
| 2,267 | 218 | Marine, 0 |

The second Bunker at `0x802e7dd0` also has four linked Marines. The
raw witness confirms the computer uses four Bunker cargo slots. Exact
first boarding time, staffing choices during combat, and a universal
Marine/Firebat ratio remain open. [Native cargo fields and boarding routine](2026-11-13-n64-blood-bath-linked-survivors.md)

## Current ordinary ARM control

The normal Terran control uses the `f6869829` runtime. An input-only, idle
Terran plays both matching lobbies with read-only state samples every
96 updates through the opening. The Terran AI is owner 1 in the solo
lobby and owner 3 in the mixed lobby. Each value is the first sample
where the structure is visible in the requested phase.

| Observation | GBA solo update | GBA mixed update |
| --- | ---: | ---: |
| First Barracks foundation | 2,017 | 2,017 |
| Refinery foundation | 2,785 | 2,785 |
| Second Barracks complete | 5,089 | 5,089 |
| First Academy foundation | 4,513 | 4,705 |
| First Factory foundation | 4,513 | 4,705 |
| Academy complete | 5,953 | 6,241 |
| Last observed sample | 13,464, human defeat | 17,473, match active |

Neither GBA run shows a Bunker foundation or a transported Terran
passenger. The shared engine already supplies Bunker construction,
boarding, firing and cargo save handling. The planner has no Bunker
build step or boarding decision. It starts a Factory before the native
Bunker window and roughly 6,500 to 11,300 updates before the respective
first native Factory samples at the 5/4 clock.

## Paid GBA opening

The shared planner now requests one Bunker in Blood Bath Free for All
after two completed Barracks, one completed Academy, and 18 SCVs
counting production, with at least 17 live SCVs. The
placement search faces the map interior and uses the existing SCV
build and wallet services. It begins at update 8,652 for the standard
opening and 10,002 for the measured Terran versus Zerg, Terran and
Protoss roster. Both are the last native sample without a foundation,
converted using the 5/4 clock.

The ordinary tech plan resumes when the Bunker completes or the
opening ends at 11,042. It also remains available when the home
mineral line runs dry, the worker target is low and there are too few
SCVs, the Bunker record is unavailable, hostile aircraft appear, or a
Factory already exists or is on order. Other maps and game types keep
their build plans.

Once the Bunker is complete, the planner sends one nearby eligible
Marine or Firebat per planning pass through the existing boarding
order. Pending boarders and loaded passengers share the four-slot
limit. The army planner leaves their boarding orders intact. Loaded
units still take up supply and actor slots. The existing save records
hold their orders and targets.

| Observation | Terran host solo | Terran host mixed | Terran ARM mixed, first sample |
| --- | ---: | ---: | ---: |
| First Bunker foundation | 8,898 | 10,258 | 10,261 |
| First complete Bunker | 9,460 | 10,820 | 10,825 |
| First Factory foundation | 9,589 | 11,125 | 11,125 |
| First four loaded infantry | 9,767 | 11,135 | 11,137 |

Both host lobbies place one forward Bunker within the corresponding
native foundation and completion brackets and reconcile spending and
supply. The solo Terran first offensive still leaves at 12,336, with
34 movable fighters. The same host fixture on the other map and the
mixed Protoss Battery and Zerg Sunken opening fixtures also pass.
The Factory still starts earlier than the corresponding native first
sample in both lobbies. Boarding policy stays limited to the first
Bunker, and the native comparison only confirms the time bounds above.

The ordinary-button ARM replay samples the mixed foundation with an
SCV assigned, 18 live SCVs, the two completed Barracks and Academy,
and no Factory. At update 11,137 the Bunker holds three Marines and a
Firebat. Pause-save writes 32 KiB, and HOME+L in a fresh process
restores the Bunker, passenger slots, targets and per-owner banks.
The full observed world matches the uninterrupted replay 600 updates
later, with the same four troops still loaded. The 32 MiB Terran
cartridge retains IWRAM `0x030048a8` and EWRAM `0x02038ddc`.

## Four-profile composition

The shared Bunker opening plus the renderer change links with unchanged
RAM endpoints on Terran, Z10, P10 and Showcase. In the same ordinary
mixed Blood Bath lobby, each build first shows the paid foundation at
10,261, completion at 10,825 and Factory placement at 11,125. The
foundation has an active SCV, two completed Barracks and a completed
Academy. The Factory is absent at that cut.

| Profile | First four loaded infantry | Fresh continuation ends |
| --- | ---: | ---: |
| Terran | 11,137 | 11,737 |
| Z10 | 11,149 | 11,749 |
| P10 | 11,149 | 11,749 |
| Showcase | 11,137 | 11,737 |

Each 32 KiB SRAM image restores its four passenger identities, transport
targets, resource banks and world. The next 600 ordinary updates match
uninterrupted play. Terran also matches its pre-renderer samples and
SRAM byte for byte. The combined source is `0cfbc84d`. The comparison
manifest SHA-256 is
`3ebd3fc0aef8ab7dbd703ff1ecac892b8cd24ae8f22aa0204b2a2acd3d173654`.

## Factory timing

The first Factory (raw 113) is under construction when it first appears
in each census. The lower bound is the last sample without it, and the
upper bound is the first sample showing it. The GBA column applies the
same 5/4 clock conversion, rounded down at each boundary.

| Blood Bath census | Native Factory placement | GBA clock window | Qualified GBA placement |
| --- | ---: | ---: | ---: |
| Unmodified solo Terran, run 4, owner 2 | (8,688, 8,834] | (10,860, 11,042] | 9,589, HOST |
| Unmodified mixed, run 1, owner 0 | (12,550, 12,807] | (15,687, 16,008] | 11,125, normal ARM |
| Unmodified restored mixed lobby, owner 3 | (9,887, 9,970] | (12,358.75, 12,462.5] | Different seat rotation |
| Defeat-suppressed mixed, run 2, owner 2 | (9,865, 10,037] | (12,331, 12,546] | |

All three first-placement samples have 20 completed SCVs and one in
production, two completed Barracks, five completed Supply Depots, a
completed Refinery, an Academy, and a Bunker. Each Bunker and Academy
is complete. No Machine Shop, Starport, Vulture, or Tank appears in
those samples. The first offensive meets the census wave criterion
one sample earlier in runs 4 and 2 and in the same sample in run 1.
These cuts establish order and timing brackets. The production policy
and its relation to the first offensive remain unverified.

The mixed keepalive owner starts at `(128, 240)`, as in the clean solo
run. The clean mixed owner starts at `(1760, 1808)`. Run 2's Factory
placement precedes the sampled loss of the human's final building at
10,984. Its timing is additional evidence from a modified run. Use
the two unmodified windows for the first GBA comparison.

The current GBA Factory starts 1,271 updates before the clean solo
window's lower bound and 4,562 before the clean mixed bound. Compare
delaying the first Factory in the matching Bunker openings while
retaining normal paid placement, garrison, and first-wave behavior.
Keep ordinary tech development available when pressure or losses
interrupt the opening. The [project TODO](../PROJECT-TODO.md#current-integration-priorities)
lists the Factory replay cases.

## Regression cases

With prepared inputs, run
`build/test-sc64-skirmish-ai-progress-terran --map 1 --human t --bots t --opening-bunker`
and repeat with `--bots ztp`. Use `--first-wave` for the solo and the
other-map control. The host pause and 600-update continuation
matches for both an active foundation and a full garrison. Run
`--resume-at 9000` and `--resume-at 9850` with `--bots t`, and
`--resume-at 10300` and `--resume-at 11160` with `--bots ztp`, each
with `--resume-len 600`.

Repeat the ordinary-input continuation on all four normal ARM profiles with
`tools/sc64_skirmish_sram_continuation.py` with the profile ROM and ELF,
the physical backend, `--physical --map 1 --human t --seats ztp`,
and a `--play` offset at the confirmed staffed Bunker. Use
`--continue 600` and compare building slots, passenger orders and
targets, banks and the uninterrupted world. The prepared
`make test-sc64-shared-bunker` suite covers the shared boarding,
combat and cargo codec fixtures.

| Evidence | SHA-256 |
| --- | --- |
| Unmodified native solo census, run 4 | `d725aa4b6c701b0de5c7939ddea7ab279330225acb106df3ade4579650f651b0` |
| Unmodified native mixed census, run 1 | `9ab2e19766f935ad5c30c2c78cb51eda3967c0b6a2724f025ac63e092f5b4ca6` |
| Native keepalive census, run 2 | `860333e92dd842386f19af44acc57d1083979d15666c835144e52807ee519576` |
| Native raw keepalive state, `keep-019444.bs1` | `4db6e4cf342abd2af5b3409d6ad3b9f9298b1a12e529d5484bde09eed7eab7be` |
| GBA Terran ARM control cartridge | `8df962af5e292af798e2fe0934e2356cf929f3853d61c3a6b17e1fffbb86aa92` |
| GBA solo control observer receipt | `5480d710095fb1082363612f0c96fc5b839e2feb2a5b6977b98ff9bcd8ad43d8` |
| GBA mixed control observer receipt | `1be779b75bef20b20ed38a4dfd5f77cca2b831a7154684d89bdfcf44853f2c9d` |
| GBA Terran ARM Bunker cartridge | `1420ec734293ad760b4da5b12feebab5c1c172925863dc2183c25b708b70fb54` |
| GBA Terran mixed ARM opening and fresh garrison continuation | `7cedceaf92800b5dd915297b148c2fea9a0975b42263378eedbcef7efffe44d4` |


The [unmodified restored-lobby census](2026-11-19-n64-skirmish-ai.md#unmodified-restored-lobby-run)
shows Terran owner 3's Bunker foundation in `(7,120, 7,194]` and its first
completion in `(7,619, 7,692]`. At the Factory foundation it has two
complete Barracks and a complete Academy, Bunker and Refinery, five
complete Supply Depots and 19 complete SCVs with none in production.
The two unmodified mixed lobbies share the race roster and differ in
seat rotation and first Factory timing. Use the opening state and
actual opponents reached when qualifying a strategy. The later clean
mixed window is specific to its captured opening.

## Current release with death depth

Composed runtime `eb409168` adds the AI changes to release `cd833c8c`.
Terran, Z10, P10 and Showcase link with the current death-depth bank
and unchanged RAM endpoints. Each normal ROM enters the mixed Free
for All lobby with the same named opponents and reaches both of these
ordinary-input save cuts.

| Profile | Active SCV foundation | Four loaded passengers | Mean cycles per update after foundation / staffing |
| --- | ---: | ---: | ---: |
| Terran | 10,261 | 11,137 | 863,324 / 986,684 |
| Z10 | 10,261 | 11,149 | 913,185 / 1,090,323 |
| P10 | 10,261 | 11,149 | 1,007,523 / 1,177,222 |
| Showcase | 10,261 | 11,137 | 948,006 / 1,121,416 |

Each 32 KiB journal restores the observed world and resource banks
through an ordinary HOME L press in a fresh emulator. The active cut
retains the SCV assignment and the staffed cut restores all four
passengers. Both match the next 600 updates from uninterrupted play.
The staffed observations also match the earlier Bunker replay across
all four products. Cycle counts are for the 600-update emulator
windows after each cut. The build and replay manifest SHA-256 is
`96cb299ef2b3a625420e235f9d4c65e8a6a148afda1d3d844e128afe631dd197`.
