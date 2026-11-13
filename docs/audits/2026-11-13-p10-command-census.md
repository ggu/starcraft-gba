# Eye of the Storm: trigger census and gameplay stalls

The full ordinary HOME replay of the original P10 cartridge wins at
simulation frame 46,057 after 47,194 calls. A second replay with a read-only
observer matches all 20 gameplay observations, ten briefing reveals,
14 authored message pages, and the timing counters. It takes 90,760,076,029
guest cycles, or 1,923,127 per call (8.72 updates/s at the GBA CPU clock).
This includes menu, presentation, and waits; it is not a hardware frame-rate
measurement. The target is 561,792 cycles per update.

The final result transition is the 76,630,638-cycle global maximum. The
largest ordinary gameplay update is call 11,045, frame 10,803: 45,240,570
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

The opt-in `SC64_Z8_DIRECT_COMMAND_CENSUS` provider reads the live
unit and building pools in their original order. It omits dead slots and
avoids filling unused actor view fields. It still validates owner, raw
type and geometry for every live actor, even when the actor cannot match
the condition, and sees all earlier trigger mutations. Bring, Kills and
replacement actor providers use the original path. It changes no trigger
schedule, persistent state, or save layout.

The ASan/UBSan differential uses the actual P10 pools and compares 66,816
queries over owners, forces, raw types, group types and comparisons. It
also covers same-slot creation, removal, ownership and building completion
changes, malformed live rows, untouched output on faults, and replacement
provider fallback. Run `make test-sc64-z8-command-census` with prepared
P10 inputs.

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

Mean cycle cost decreases 8.70% (10.32 to 11.31 emulated updates/s).
IWRAM ends at the same `0x03004ab0`; the BSS end moves eight bytes to
`0x0203c0e8`, within the existing guards. ROM verification passes.
The depot stalls remain; this is a prefix comparison, not a candidate
win, save/load proof, or hardware qualification. The flag defaults off.

Control ROM SHA-256:
`7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310`.
Candidate ROM SHA-256:
`122eb131a5deeaeeb9ea25b68954e276a936564559caba762348e04c203de682`.
Paired video/state receipt SHA-256:
`e38da1afadb43098c23eb7f9140fee18e643a1d2beeee0d57704f4bab3bbb3b7`.
