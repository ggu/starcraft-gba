# Z5 Egg production applicability: source census

The authenticated Z5 CHK (`008/018`, SHA-256
`070d8e0233f57ee478571cadbb77c25dc13f4c71ad2acabeecc4088adddd5466`)
has no initial or scripted seed for ordinary Zerg larva production in the
current authored campaign path. This is a source and implementation census,
not an ordinary-input win or proof of N64 runtime behavior. The source-only
`tools.sc64_z5_egg_census` repeats the numeric census against a separately
prepared source-map directory; it validates the catalog and complete CHK
manifest and checks input hashes before and after.

- All 149 initial UNIT rows, including seven start markers, contain no Drone
  (raw 41) or Hatchery/Lair/Hive (131/132/133). All 71 THG2 rows include 41
  unit-backed doodads; none is a Drone or one of those producers.
- All 155 retained action rows contain 13 CreateUnitWithProperties actions,
  creating only raw 0 (three), 37 (seven), 101 (two) and 218 (one); there are
  no CreateUnit (44) or GiveUnitsToPlayer (48) actions. MoveUnit and
  SetDoodadState do not create or transfer a producer. The listed create
  actions therefore do not seed a Drone or Hatchery/Lair/Hive either.
- Source slot 3 is the selected human Zerg controller. Its effective PUNI
  denies the inspected ordinary larva products 37, 38, 41, 42, 43, 45, 46
  and 47, as well as Hatchery/Lair/Hive. This PUNI fact is an independent
  admission restriction; it is not the proof that the map has no producers.
- Source slot 0 is Zerg and maps to RESCUABLE in the imported descriptor. Its
  initial actors are two Hunter Killers, not a Drone or producer, and its
  PUNI permits the inspected products. After any control transfer, the
  controlling player's permissions must apply; preserved raw origin is not
  a wallet or permission identity. The raw-origin 2 to SIDE case likewise
  must never inherit source-2 permissions merely from origin.

The authored Z5 campaign branch returns after its scenario tick rather than
running the generic campaign AI. Its normal initial map import skips raw 214
start markers. Without a Drone, Hatchery-family building, scripted source
creation of either, or an ownership transfer action that can provide one,
ordinary Egg production has no admitted source path in this audited
starting/trigger closure. Pinned OpenBW also conditions automatic starting
units on melee setup or an explicit create-melee-units flag; this is PC source
context, not an assertion about N64 startup. The retained CHK actions alone
cannot prove native timing, all possible engine bugs, arbitrary externally
edited state, or whether a prior save was produced by a different build.

Treat Z5 as not applicable to new ordinary Egg admission within this scoped
closure; do not introduce a positive producer fixture by injecting one or
borrow another source's PUNI. Preserve the existing old-tag loader and actor
semantics. A genuine unretagged current-main Z5 save followed by a fresh staged
reader and ordinary continuation is still required before claiming legacy
compatibility; this source-only audit did not run that test, change validators,
or enable any new tag or normal profile flag.
