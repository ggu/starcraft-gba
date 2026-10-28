# Zerg and Protoss campaign source audit

This note inventories the staged source maps for Zerg 5–10 and Protoss 5–10.
It describes numeric source structure and engine requirements; it does not
reproduce trigger tables, prose, dialogue, assets, or source hashes. Map data
remains in the ignored `.local/sc64/` workspace. Pool figures below count
starting UNIT records by source category; neutral start markers are listed
separately. “Create once” sums one execution of every opcode-11 action, so it
is not a lifetime bound when a preserved trigger can fire again.

## Source capacity and trigger inventory

| Mission | Map | Starting units | Buildings | Minerals | Geysers | Start markers | Create once / largest trigger | Preserved triggers | AI script actions / distinct IDs | UPRP IDs used |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Z5 | 128×128 | 135 | 7 | 0 | 0 | 7 | 13 / 10 | 3 | 0 / 0 | none |
| Z6 | 128×128 | 73 | 49 | 60 | 3 | 3 | 3 / 1 | 0 | 1 / 1 | 1–2 |
| Z7 | 128×128 | 113 | 57 | 60 | 1 | 2 | 0 / 0 | 0 | 4 / 3 | none |
| Z8 | 128×128 | 68 | 70 | 51 | 5 | 4 | 16 / 7 | 13 | 8 / 3 | none |
| Z9 | 128×128 | 135 | 150 | 81 | 5 | 6 | 2 / 2 | 0 | 18 / 6 | none |
| Z10 | 96×128 | 99 | 144 | 92 | 2 | 4 | 2 / 2 | 0 | 5 / 5 | none |
| P5 | 128×128 | 110 | 69 | 57 | 3 | 5 | 11 / 11 | 0 | 3 / 3 | none |
| P6 | 128×128 | 200 | 4 | 0 | 0 | 6 | 18 / 9 | 0 | 1 / 1 | 1–13 |
| P7 | 128×128 | 180 | 112 | 64 | 1 | 5 | 3 / 3 | 0 | 3 / 3 | none |
| P8 | 128×128 | 89 | 115 | 50 | 2 | 4 | 24 / 22 | 0 | 3 / 3 | 1–3 |
| P9 | 128×128 | 162 | 114 | 63 | 4 | 5 | 2 / 1 | 2 | 3 / 3 | none |
| P10 | 128×128 | 120 | 67 | 142 | 12 | 4 | 28 / 28 | 0 | 5 / 3 | 1–28 |

The largest starting pools in these chapters are Z9’s 150 buildings and P6’s
200 units. The older chapter-four ZP storage limits (Zerg 192 units; Protoss 240;
96 buildings; 48 minerals; 4 geysers) fail later source maps:
Z9/Z10 exceed buildings, Z6/Z8/Z9/Z10 and P5/P7/P8/P9/P10 exceed minerals,
and Z8/Z9 and P10 exceed geysers. P10 alone needs 142 mineral fields and 12
geysers. These are source placements only; runtime-created actors need extra
capacity. P9’s and Z8’s preserved creation triggers make that additional
capacity potentially recurring, so a hard lifetime reserve needs a mission
rule or an explicit bounded spawn policy.

The opt-in Z5 and P5 full-storage profiles now raise these pools to 240 units,
160 buildings, 144 minerals and 12 geysers using compact runtime/save rows.
It fits these starting-placement counts, but does not establish complete
actor support, repeated-spawn safety or playability for the later maps.

Current staged runtimes cover Z5's unusual actors, doors and traps and P5's
complete actor census, including its Guardian, Shuttle and created
Battlecruiser. Z5's optional source-permitted cloak has controls, energy,
detector and save tests. P5 has source-world callbacks, rescue and persisted
death counters. P5 Shuttle now has a complete ordinary-input host and ARM win
and exact fresh-process cargo save. Z5 remains under controller qualification.
The actor-gap lists below retain the initial source audit; already resolved
Z5/P5 entries are historical requirements, not current import failures.

Raw 150 is the Mature Chrysalis building, while raw 216 is the Chrysalis unit.
Raw 196 is a Protoss beacon building and is included in the building counts.
Raw 214 start markers are not units. The source actor types not mapped by the
initial shared ZP actor/unit/building rules are listed by raw ID below; these
are source occurrences, not a claim that every row is mission-critical.

## Per-mission requirements

Selector values are source values: player slots are 0–7, Current Player is
13, All Players is 17, and Force selectors are 18–21. Condition and action
selector lists below are the distinct values present in that map. Opcode names
use the names in the source importer. “AI” counts opcodes 15/16; each distinct
script identifier is opaque and must be mapped to bounded authored behavior,
not executed as source bytecode.

### Zerg 5–10

| Mission | Condition opcodes / selectors | Action opcodes / owner selectors | AI | Source actor raw IDs lacking a current mapping |
|---|---|---|---:|---|
| Z5 | Command, Bring, CommandsTheMostAt, CommandTheLeastAt, Switch, ElapsedTime; `0,13,18` | CreateUnitwithProperties, MoveUnit, RemoveUnitAtLocation, SetDoodadState, SetSwitch, PreserveTrigger, CenterView, MinimapPing, PauseGame, Wait, Transmission, DisplayTextMessage, PlayWAV, Mute/UnmuteUnitSpeech, objectives, Victory, Defeat; `0,13,17,18,19` | 0 | `1×10, 15×32, 89×6, 90×7, 95×5, 195×7, 218×1` |
| Z6 | Command, Bring, Switch, ElapsedTime; `0,1,13,17,19` | CreateUnitwithProperties, KillUnitAtLocation, MoveLocation, SetResources, SetSwitch, CenterView, Wait, Transmission, DisplayTextMessage, PlayWAV, objectives, Victory, Defeat; `0,1,2,3,18` | 1 action / 1 ID | `44×2, 171×1, 172×1` |
| Z7 | Command, Bring, Switch, ElapsedTime; `0,13,18,19` | SetResources, SetSwitch, Wait, RunAIScriptAtLocation, objectives, Victory, Defeat; `0,18,19` | 4 / 3 IDs | `44×5` |
| Z8 | Command, Bring, Kill, Switch, ElapsedTime; `0,1,2,13,17,19,20` | CreateUnitwithProperties, RunAIScriptAtLocation, SetInvincibility, MoveLocation, SetSwitch, PreserveTrigger, MinimapPing, CenterView, SetResources, Wait, Transmission, DisplayTextMessage, PlayWAV, Victory, Defeat; `0,1,2,3,18,19,20` | 8 / 3 IDs | `84×1, 134×4, 159×2, 163×1, 165×1, 171×2` |
| Z9 | CountdownTimer, Command, Bring, Switch, ElapsedTime; `0,13,17,18,19,20,21` | CreateUnitwithProperties, MoveLocation, RemoveUnitAtLocation, SetCountdownTimer, SetResources, SetSwitch, RunAIScriptAtLocation, CenterView, Wait, Transmission, DisplayTextMessage, PlayWAV, objectives, Victory, Defeat; `0,3,18,19,20,21` | 18 / 6 IDs | `68×4, 69×4, 71×1, 72×2, 84×5, 159×2, 163×3, 165×2, 169×2, 170×2, 171×2, 173×1` |
| Z10 | Command, Bring, Kill, Deaths, Switch, ElapsedTime; `0,13,17,18,19,20,21` | CreateUnitwithProperties, MoveLocation, SetResources, SetSwitch, RunAIScriptAtLocation, CenterView, Wait, Transmission, DisplayTextMessage, PlayWAV, objectives, Victory, Defeat; `0,18,19,20,21` | 5 / 5 IDs | `68×4, 69×7, 71×1, 72×2, 84×2, 159×3, 163×3, 165×3, 169×3, 170×2, 171×4, 172×5, 174×1` |

Z5’s largest one-trigger creation batch is ten actors; it also changes doodad
states 31 times and moves/cleans up selected units. Z6 is a compact trigger
candidate by count, but needs location movement, nine KillUnitAtLocation
actions, and one opaque AI routine. Z7 has no trigger-created units and is
small in trigger count, but still depends on three AI routines. Z8 has six
PreserveTrigger records that also create one raw-74 actor apiece; its create
total of 16 is not a safe upper bound. Z9 and Z10 each have few creates but
many AI actions (18 and 5 respectively) and broad Force selectors.

### Protoss 5–10

| Mission | Condition opcodes / selectors | Action opcodes / owner selectors | AI | Source actor raw IDs lacking a current mapping |
|---|---|---|---:|---|
| P5 | Command, Bring, Deaths, Switch, ElapsedTime; `0,6,13,17` | CreateUnitwithProperties, KillUnit, SetInvincibility, SetResources, SetSwitch, RunAIScript, RunAIScriptAtLocation, CenterView, Wait, Transmission, DisplayTextMessage, PlayWAV, objectives, Victory, Defeat; `0,6,13,20,21` | 3 / 3 IDs | `44×14, 69×1` |
| P6 | Always, Command, Bring, Switch, ElapsedTime; `0,7,13,17,18` | CreateUnitwithProperties, KillUnit, SetDoodadState, SetSwitch, RunAIScript, Wait, Transmission, DisplayTextMessage, PlayWAV, CenterView, Mute/UnmuteUnitSpeech, objectives, Victory, Defeat; `0,6,13,17` | 1 / 1 ID | `1×1, 50×8, 74×8, 75×1, 195×2` |
| P7 | Command, Switch, ElapsedTime; `0,13,17,18,19` | CreateUnitwithProperties, MoveLocation, SetResources, SetSwitch, RunAIScriptAtLocation, Wait, Transmission, DisplayTextMessage, PlayWAV, CenterView, objectives, Victory, Defeat; `0,13,18,19,20` | 3 / 3 IDs | `68×1, 69×8, 71×3, 72×2, 74×4, 75×1, 84×11, 159×2, 163×2, 165×2, 169×2, 170×2, 171×2, 172×2` |
| P8 | Command, Bring, Switch, ElapsedTime; `0,13,17,19,20` | CreateUnitwithProperties, MoveLocation, SetResources, SetSwitch, RunAIScriptAtLocation, PauseGame, Wait, Transmission, DisplayTextMessage, PlayWAV, CenterView, Mute/UnmuteUnitSpeech, objectives, Victory, Defeat; `0,1,2,13,18,19,20` | 3 / 3 IDs | `28×1, 69×3, 71×3, 72×4, 84×1, 159×2, 163×2, 165×2, 168×1, 169×2, 170×1, 171×2, 172×2` |
| P9 | Command, Bring, Switch, ElapsedTime; `0,13,17,19,20,21` | CreateUnitwithProperties, MoveUnit, KillUnitAtLocation, PreserveTrigger, SetResources, SetSwitch, RunAIScriptAtLocation, Wait, Transmission, DisplayTextMessage, PlayWAV, objectives, Victory, Defeat; `0,5,6,7,18,19,20,21` | 3 / 3 IDs | `44×2, 46×2, 71×1, 75×1, 136×2` |
| P10 | Command, Switch, ElapsedTime; `0,3,17` | CreateUnitwithProperties, MoveLocation, SetResources, SetSwitch, RunAIScriptAtLocation, CenterView, Wait, Transmission, DisplayTextMessage, PlayWAV, objectives, Victory, Defeat; `0,1,2,3,13,18` | 5 / 3 IDs | `44×4, 46×2, 74×4, 75×1, 82×1, 136×1, 147×1` |

P5's 14 raw-44 actors now have explicit support; its three opaque AI routines
have a documented bounded replacement policy. P6 fits the current unit bound with no
buildings/resources, but has 20 Bring conditions, 18 doodad changes, 14
property variants, and source actor types not yet mapped. P7/P8/P9 exceed the
older 96-building bound but fit the opt-in 160-building pool. P8 also creates 22 actors
in one trigger and uses four UPRP variants. P9’s two preserved triggers each
create raw-151 actors, so its two-create static count is not a lifetime cap.
P10 has the largest resource demand and uses every property ID 1–28; its
trigger starts with a batch of 28 creates. Its pool/storage and property
decoder requirements make it a poor next integration target despite its
moderate trigger count.

### P8 scheduler scaffold

P8 now has a compact source-order trigger runner and a typed view over its
compact generated trigger rows. The runner accepts condition/action callbacks,
keeps an independent cursor for each of eight trigger records, and bounds each
record to 16 conditions and 64 actions. The caller chooses when to run a pass;
this scaffold does not encode or qualify P8's native scheduler cadence.

An asynchronous action resumes at the same cursor. A callback can return
`BLOCKED` when a timed action cannot start because its owner already has a
shared wait; that action is retried on the next pass without the resume marker,
while later triggers still run in source order. Later actions in that same
trigger remain behind the blocked cursor. As in the existing native-backed Z8
action model, the callback must own the shared timer per source owner and the
armed marker per action. `sc64_p8_timed_service.h` now supplies that bounded
callback policy, but it is not wired into the game loop or a save format.
The pinned native scheduler evidence (ROM SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) shows that source PauseGame completes
immediately, marks its trigger instance, and allows later actions/triggers to
run while its source-pause marker is active. A normal user pause freezes the
trigger/timer clock; source pause does not. At action-list completion, native
dispatches action-table entry 6 at runtime address `0x800C757C`; its ROM word
at file offset `0xC817C` points to the UnpauseGame handler `0x8008086C`, which
clears the source marker. The timed service models this clock distinction
and per-owner/per-trigger lifetime, while caller callbacks must apply the
actual PauseGame and automatic tail UnpauseGame effects. P8's pass cadence is
still caller-owned and unqualified. Terminal results are accumulated across the
source-order pass, with Victory taking precedence over Defeat, and are
accepted only from matching final Victory/Defeat actions. Unknown conditions
or actions fault the run instead of being silently skipped.

P8 is not wired into gameplay, menus or saves. Its three opaque AI identifiers
are not implemented or executed as bytecode; creation, resource, switch,
location, presentation, speech and other effects still need exact world
handlers. No P8 native cadence/replay, runtime callback coverage, save/load or
ordinary-input qualification exists. The scaffold does not establish that the
mission starts, progresses, or reaches a source victory.

`make test-sc64-p8-runtime` is an asset-free synthetic check and is part of
`test-starcraft`. If a P8 generated scenario header already exists locally,
`make test-sc64-p8-source-contract` checks its compact table bounds and
fail-closed callback bridge. That target checks the generated header only; it
does not authenticate its CHK provenance, run the importer, or create generated
data. Source authentication remains a separate packer verification step.
With the existing staged CHK and manifest, that read-only step is
`python3 tools/sc64_zp_scenario_pack.py --mission P8 --verify-staged`.

The terminal rules are compactly source-expressible, but still need exact
runtime tests. Z5 wins on a set switch and loses when selector 18 has no raw
51. Z6 combines a switch and a raw-79 count condition for victory; its defeat
gates check selector 13 against raw 229 and raw 51. Z7’s win checks selector
19 against raw 231. Z8’s win checks raw 154 counts for selectors 1 and 2;
defeat also watches raw 229, raw 51, and three raw-74 Bring conditions. Z9
and Z10 win when selector 13 or Force 1 respectively brings at least one raw
219 to the specified location; Z9 also uses the raw-229 defeat condition.
P5 requires two Bring conditions (at least one raw 79 and at least
two raw 65 for selector 13). P6 requires at least one raw 75 for selector
13. P7 and P9 win on switch state; P8 checks a raw-168 Command condition for
all players. P10 checks raw 147 for selector 3. Protoss defeat gates include
the common raw-229 condition and, depending on mission, zero-count checks for
raw 79, 78, 75, 82, or 28. These are source condition summaries, not
human-readable objectives; use the staged mission model for locations,
comparisons, switch state, and force membership rather than inferring them
from these shorthand clauses.

## Engine work versus import-only work

The existing import pipeline already retains complete numeric source records
and stable map identities. Once a map’s source actor types fit the bounded
runtime and its map pools fit storage, many terrain/placement rows are data
alone. The following are runtime contracts that cannot be solved by adding
more map data:

- **Actor mapping and geometry:** add bounded support for the raw actor IDs
  called out per mission, including selectable/collidable/drawable behavior,
  correct building-vs-unit classification, hit points and map bounds. Z5
  additionally contains raw-195 Terran beacons and raw-218 data-disc actors;
  P6 contains raw-195 beacons. Do not silently convert an unmapped row to a
  default unit.
- **Map-sized bounded pools:** storage must accommodate each starting map and
  reserve capacity for authored creations. A full-campaign storage decision
  must cover the largest per-map demand plus the mission’s create behavior,
  including repeated preserved triggers. Current minerals/geysers/buildings
  chapter-four bounds are not sufficient for later chapters; the new expanded
  profile needs per-mission runtime qualification.
- **Trigger semantics:** condition opcodes represented here need exact
  selector, unit-count and location semantics. Actions include creation with
  UPRP properties, waits/pause, owner-scoped resource and switch changes,
  location moves, selected-unit moves/removals, doodad state and invincibility
  changes, timer control, and terminal outcomes. Existing behavior may be
  reused only where it matches the source opcode’s sentinel and owner rules.
- **Preserved triggers and repeated spawning:** Z8 and P9 have preserved
  creation triggers. Integrations need a state-derived repeat limit or
  unbounded-spawn policy tied to an explicit available-slot check; counting
  each trigger once is insufficient for storage safety.
- **AI scripts:** actions 15/16 are opaque source script identifiers. They
  require mission-authored deterministic, bounded approximations and explicit
  completion criteria. A numeric import is not an AI implementation.
- **UPRP properties:** P6 requires 13 nonzero property records, P8 requires
  three, and P10 requires all 28. The runtime must decode and validate only
  the fields this engine supports, preserve ownership/energy/resource/health
  defaults exactly where relevant, and fail closed on unsupported property
  combinations.
- **Mission objectives and save identity:** the broad mix of Command, Bring,
  Switch, timer, Deaths, and Kill conditions must be translated into
  deterministic mission state. Any staged chapter needs a distinct save
  generation and strict validation of trigger cursors, timers, repeat state,
  created-actor counts, and other persisted fields before normal-frame use.

## Suggested implementation order

1. **Z6** is the most promising next runtime candidate from trigger volume:
   it has only three one-shot creates and no preserved triggers. First close
   the remaining pool gap (60 minerals versus the current 48), map raw 44 and
   building raws 171/172, then implement/test location movement and
   KillUnitAtLocation semantics. Its one opaque AI action remains a
   mission-specific requirement.
2. **P5** is a reasonable parallel candidate if the raw-44 actor can be
   modeled once and reused. It needs only 69 buildings, 57 minerals and three
   geysers, but requires three AI routines and 11 created actors. P5 and Z6
   reuse the engine’s property decoder only for the default/no-property row.
3. **Z7** is data-light in trigger behavior and has no create actions, but
   still requires raw-44 support and three authored AI routines. **P6** is a
   strong follow-on storage test after actor mapping: it exercises the
   240-unit pool and the reusable nontrivial UPRP decoder.
4. Defer **Z8/P9** until repeat-trigger state and safe creation reserves are
   established; defer **P10** until full mineral/geyser storage and a
   28-property validation matrix exist. Z9/Z10 also need substantial
   building-capacity growth and many distinct AI mappings.

For every candidate, qualification should compare the imported source
placements and trigger programs against an authored controller fixture, then
run host sanitizer tests for normal progression, wait/retry timing, save-load
continuation, malformed state, exhausted actor slots, and exact victory and
defeat gates. ARM memory and long replay qualification are separate from
source-import completeness.
