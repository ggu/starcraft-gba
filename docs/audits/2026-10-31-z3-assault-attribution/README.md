# Carried Z3 assault attribution

This is diagnostic evidence for the Z7 carried profile, not a Z3 win. Engine
source was `197677a`; the ordinary-key controller experiment is retained in
`/tmp/z3-unqualified-policy-20261031.patch`, not committed. Its trace is
`/tmp/z3-local-miners-20261031.json`. State and map contents were used to
choose targets, but game actions were controller inputs; no world, resource,
or UI state was written by that experiment. The controller had restored
mining, protected the Chrysalis and sent approximately 60 non-workers toward
the enemy. Its count included ten unarmed Overlords.

A separate read-only witness, `tests/sc64_z3_assault_witness.c`, replays the
recorded held-key transitions and observes the world; it does not steer or
change state. It uses the full Z7 carried flags and a caller-provided private
`sc64_z3_assault_events.h` defining the `events` array (tick and held fields),
constructed from the trace's input records. That header, trace and generated
inputs are not source artifacts. Witness output
`/tmp/sc64-z3-assault-witness.log` matched all 64 overlapping 600-frame
checkpoints on side-army, enemy units, enemy buildings, minerals and gas.
This is an aggregate comparison, not a complete state equivalence proof.

The trace shows side-army counts 57→41 over frames 33000–36000, 62→45 over
54000–57000, and 62→14 over 66000–69000; enemy buildings fell from 26 to 22
across those windows. The witness recorded respectively 21, 23 and 53
side deaths in those windows, predominantly Hydralisks (20, 22 and 53), all
on attack-move. Enemy raw-30 siege tanks were the nearest surviving enemy to
14, 20 and 41 of those deaths; nearest is not proof of the actual damage
source. At frame 68352 nine Hydralisks died in one update inside a 24×23-pixel
area; other updates saw six and five tightly grouped deaths. The stock siege
splash path was enabled in this profile. Two northern siege tanks at (339,146)
and (280,179) were present before the last wave and gone afterward. This
strongly implicates clustered ground assaults against splash defenses, but
the witness does not instrument the damage callback and does not prove each
individual casualty's cause.

Target selection ranks enemy buildings by straight-line distance from the
Chrysalis, without path or defense evaluation. The trace's strategic targets
included (256,560), (80,544), and (336,448). A click on a fogged enemy does
not resolve to a direct target; the game issues attack-move instead, with
bounded nearby acquisition. The observed attackers moved deep into the map;
this evidence does not establish that the remaining targets are unreachable.
The failed air-tech attempts were not qualified: they lost the Hatchery or
stalled before clearing the base, and their cause requires its own witness.
An authorized next experiment should count armed units, protect both the
Hatchery and Chrysalis, and use ordinary commands for separated attack groups
or protected anti-tank air. No engine, mission objective, damage, actor limit
or scripted AI should be changed to manufacture a win.

The late-air attempt was separately replayed through the same input-only
witness with Hatchery damage reporting (`tests/sc64_z3_hatch_witness.c`,
private trace `/tmp/z3-late-air-20261031.json`). The Hatchery at (752,1368)
lost 7 HP per hit from frame 42988 to its destruction at 58354. One hostile
raw-8 Wraith at (799,1298) was the only enemy within 300 pixels in each
recorded damage snapshot. At frames 42000, 43200, 54000 and 58200, the only
friendly nonworkers within 160 pixels of the Hatchery were unarmed Overlords.
The controller's local combat count included those Overlords, while its main
Hydralisk group remained near the Chrysalis. This is strong spatial and
temporal evidence of an unattended Wraith attack, not a damage-callback trace.
The witness uses aggregate checkpoint matching; private replay inputs and
generated assets are not checked in.

## Focused controller qualification

Controller source `7325b33` on the staged `197677a` engine was built with the
full Z7 carried Z3 flag vector and host ASan/UBSan. A fresh normal-menu run
won at native frame 120346 (host tick 121979), with the Chrysalis at 250 HP,
three enemy units and three enemy buildings remaining, and the source victory
condition and campaign-complete phase set. The frozen-source rerun's trace is
`/tmp/z3-7325b33-frozen-20261031.json`; its SHA-256 is
`c588e33cdf744e782edfdbfbbdef73f56884b017f133d02708c813b58b601114`,
identical to the preceding qualifying run. The test exited zero under both
sanitizers. All 109 pinned private inputs matched the verified receipt before
and after the frozen run.

The 11,622 recorded key transitions have strictly increasing host ticks,
valid ten-bit key masks, changed held states, and pressed masks consistent with
the previous held state. The final logged transition is tick 121474 and the
win checkpoint is tick 121979. These checks validate the host trace structure;
they do not prove it replays on the physical-key backend or fits a legacy ROM
tape. Those qualifications remain separate.

The controller counts armed units, maintains separate ground defenders at the
Hatchery and Chrysalis, builds a Lair and Spire, and directs individually
selected idle Mutalisks against the enemy while keeping ground troops away from
the concentrated siege corridor. It keeps a mineral-worker reserve and
reassigns idle workers with contextual clicks after a normal Move reveals a
map-known field. Target and field choices use the controller's observed world
and map oracle, including information outside current fog. A source audit of the
controller found no assignments to UI cursor/selection, unit/building state,
resources, mission flags or objective state: after normal initialization,
game actions flow through `step` → `game_update` with held/pressed keys. This
is bounded host evidence, not hardware timing, an independent input-only
replay, or proof for another profile. Failed tactical experiments and witness
logs remain in `/tmp` as described above.
