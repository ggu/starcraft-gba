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
  no CreateUnit (44), GiveUnitsToPlayer (48), or RunAIScript (15/16) actions.
  MoveUnit and SetDoodadState do not create or transfer a producer. The listed create
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
borrow another source's PUNI. Preserve existing old-tag loader and actor
semantics. An unmodified `aa8f43c5` engine produced a genuine, unretagged Z5
old-tag save, SHA-256
`87fb2aff023d7234339f1df74f1ffe2dc179870cd6e7dba79a4c815214596010`.
At `df5da990`, a fresh HOST reader with the Z6 Egg stage selected the old tag
without an Egg token, loaded Z5, and completed 180 advancing ordinary no-input
updates; the 32 KiB save image remained byte-identical. All 594 prepared inputs
and the old save hash were unchanged, with native work serialized under the
shared lock and a 4 GiB reserve. No pause acknowledgment was needed in that
window. This is a bounded initial-world continuation, not a mission completion,
all old saves, an ARM test, or native parity. No Z5 validator, new tag or
normal profile flag was changed.
