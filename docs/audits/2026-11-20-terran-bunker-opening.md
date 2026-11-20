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

## Next checks

1. Qualify a single paid forward Bunker before Factory in the two
   Blood Bath Free for All lobbies. Use the normal SCV construction
   path and the completed production prerequisites observed above.
   Keep a bounded placement search and fall back to ordinary tech if
   a Bunker cannot be completed within the opening. Check spending,
   two completed Barracks, Academy, and the first-offensive gate with
   the prepared `build/test-sc64-skirmish-ai-progress-*` programs.
   `--map 1 --human t --bots t --first-wave` exercises the solo gate.
   Run the spending observer in the same solo lobby and `--bots ztp`
   for the mixed lobby.
2. Reuse the existing boarding order for up to four same-owner combat
   passengers. Count pending boarders against capacity, preserve their
   orders during an army rally, and keep actual cargo counted in supply
   but out of the movable army. Use `make test-sc64-shared-bunker` with
   prepared inputs to check the four profiles' boarding, combat and
   cargo save handling.
3. On each normal ARM profile, run
   `tools/sc64_skirmish_sram_continuation.py` with the profile ROM and ELF,
   the physical backend, `--physical --map 1 --human t --seats t--`,
   and the observed `--play` offset for an active SCV build or a staffed
   Bunker. Use `--continue 600` and repeat the relevant observation with
   `--seats ztp`. Check the saved and restored Bunker slot, passenger
   order and target, resource banks, and uninterrupted continuation.

| Evidence | SHA-256 |
| --- | --- |
| Unmodified native solo census, run 4 | `d725aa4b6c701b0de5c7939ddea7ab279330225acb106df3ade4579650f651b0` |
| Unmodified native mixed census, run 1 | `9ab2e19766f935ad5c30c2c78cb51eda3967c0b6a2724f025ac63e092f5b4ca6` |
| Native keepalive census, run 2 | `860333e92dd842386f19af44acc57d1083979d15666c835144e52807ee519576` |
| Native raw keepalive state, `keep-019444.bs1` | `4db6e4cf342abd2af5b3409d6ad3b9f9298b1a12e529d5484bde09eed7eab7be` |
| GBA Terran ARM control cartridge | `8df962af5e292af798e2fe0934e2356cf929f3853d61c3a6b17e1fffbb86aa92` |
| GBA solo control observer receipt | `5480d710095fb1082363612f0c96fc5b839e2feb2a5b6977b98ff9bcd8ad43d8` |
| GBA mixed control observer receipt | `1be779b75bef20b20ed38a4dfd5f77cca2b831a7154684d89bdfcf44853f2c9d` |
