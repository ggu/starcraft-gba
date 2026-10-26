# Tank effects and directional sprites

Runtime source commit: `4e6a32d67e80a97760d5c3140d264ebf9fafd4ca`.
This fixes the campaign's frame-zero-only unit rendering and replaces the
imported Tank's generic shell effect. It adds no missions.

## Rendering and input preparation

The facing importer reads `images.dat` directional metadata. Egg, Archon,
Science Vessel and other non-directional images are not mistaken for heading
banks merely because their GRPs contain many frames. The local bank contains
54 mappings and 407299 indexed pixel bytes, plus 32 pre-mirrored word sidecars.
Canonical Terran/Zerg/Protoss and Showcase enable the packed path.

Facing is a separate byte per actor slot, cleared on initialization, accepted
load, restart and slot reuse. Moving workers also update facing during an old
weapon cooldown. Rejected loads preserve the current state. No actor or save
row fields changed. The generic raw-ID fallback and mirrored scene clipping
are covered by tests.

The Tank importer verifies the USA ROM hash, actor/turret weapon chains and
supported source script sequences. Weapon 11 resolves to impact image 531
(10 frames); weapon 27 resolves to image 533 (14 frames). Directional muzzle
children are images 536/537 (17/119 frames). Siege attachment uses the source
LO offset for the parent pose actually drawn by this engine. The generated
bank contains 491272 frame bytes and a destination-dependent light table.
All generated data stays ignored under `.local/`.

Source light-table selection is inferred; color projection into the reduced
GBA palette is lossy. Every destination index receives a nearest source-color
representative, including indices without a direct remap preimage. Source
waits use the engine's 24-to-30-update conversion. Damage still resolves at
firing, rather than at the native script's damage event. Walking cycles, native
turn rates and independently tracking turrets remain unfinished.

## Local evidence

- `./dev doctor`, `./dev build`, `./dev test`: pass, including 501 Python tests
  and the asset-free host checks.
- `make unified test-unified`: pass in the prepared workspace.
- `./dev sc64 test --profile terran`: pass, including the full Terran
  controller/save/repair regression and all ten mission runs.
- `make race-campaigns showcase test-race-campaigns test-canonical-render
  test-sc64-siege-splash-game`: pass with release defaults. Carried Z3 wins
  at frame 51360; Z4 at 59111; P3 at 47238; P4 at 104346; P5 at 5496.
- `make test-sc64-tank-source-vfx-game test-sc64-showcase test-sc64-t5-facing`:
  pass. The Tank fixture compares actual tracer pixels against the raw source
  lookup and previous destination, with scene clipping both enabled and
  disabled, east/west shots, camera/fog/HUD edges and a normal Marine shot.
- The directional fixtures cover eight movement headings for Marine, Scout
  and SCV, all 32 packed/indexed directions and reset/load/reuse behavior.
- An ordinary-input ARM scene restart restores identical world and framebuffer
  hashes after 51 updates. Eight-scene baseline/indexed/packed replays preserve
  simulation hashes; packed and indexed facing pixels match.
- A normal-menu ARM Siege Line run reaches 270 updates / 181 scene updates,
  rendering source effects in 40 frames with at most six tracers. Private
  captures were inspected. Early purple chunks are existing authored Zerg
  death gibs: they persist with Tank VFX disabled and disappear when death VFX
  is disabled. They are not source-effect or directional-word artifacts.

No Actions run was dispatched. Captures, generated assets and traces are not
part of the public repository. Scoped ARM timer samples show the packed
facing path avoids the indexed path's rendering overhead; these do not
establish 30 fps or physical GBA performance.

## Siege splash gate

Showcase enables `SC64_SIEGE_SPLASH`. Its fixtures cover weapons 27/28,
rectangle-edge distance, inclusive 10/25/40 source-pixel rings, full/half/quarter
packets before armor and explosive size scaling, shield spill, friendly fire,
shooter/air/loaded/invincible exclusions, inner-only burrow hits and normal
death bookkeeping. Bounds are numeric facts for raw IDs 0..227 from pinned
BWAPI revision `d727fed68558c506163048ea889131d8cbb33915`; behavior was checked
against the pinned references listed in `THIRD_PARTY_NOTICES.md`.

Campaign promotion is held. With identical controller inputs, enabling splash
changes the Z3 result from a win at 51360 to LIMIT at 180000. The source roster
contains six enemy siege-mode Tanks and two tank-mode Tanks; player attrition
diverges after combat starts. The current controller does not qualify that
stronger threat. Campaign releases receive both visual fixes while retaining
their prior damage rule. The compact runtime also retains integer HP/shields
and lacks a complete native weapon/defensive-matrix/acid-spore state machine.

## Production memory

All four images are 33554432 bytes. The unchanged strict limits remain IWRAM
below `0x03005000`, BSS below `0x0203e000`, and save payload at most 16368 bytes.

| Profile | IWRAM end | BSS end |
| --- | --- | --- |
| Terran T10 | `0x03004e08` | `0x0203de94` |
| Zerg Z4 | `0x03004e70` | `0x0203c4d0` |
| Protoss P5 | `0x030047c0` | `0x0203da70` |
| Showcase | `0x03004bb0` | `0x0203dfd8` |
