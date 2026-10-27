# Development

The default StarCraft skirmish builds without ROMs, extracted assets, or external reference checkouts. The required tools are Git, GNU Make, `arm-none-eabi-gcc` with its `objcopy` and `size` tools, Clang, and Python 3.10 or newer. The project uses Python's standard library; no `pip install` step is needed. mGBA is only needed to play or run emulator workflows.

The stage-by-stage campaign sections below preserve dated implementation
history; older profile descriptions are snapshots of those stages.
For the current combined Terran profile and qualification status, see
[Combined Terran T1–T10 campaign](#combined-terran-t1t10-campaign) near the
end of this guide.

## Local-first verification

Routine checks run on your development machine:

```sh
python3 tools/check_source_tree.py
./dev doctor
./dev build
./dev test
```

The source-check workflow is disabled in repository settings and has only a
`workflow_dispatch` trigger. Pushes, PRs and merges do not need an Actions run.
Report the local commands/results and tested commit when reviewing a change.
Prepared imported-profile and ARM qualifications remain separate local checks.

For an explicitly requested Linux compatibility check, an administrator can
enable `source.yml` and dispatch it manually, then disable it again. It uses
the standard Ubuntu runner; public-repository standard runner usage is currently
[free under GitHub's billing rules](https://docs.github.com/en/billing/concepts/product-billing/github-actions).
Do not add paid/larger runners or automatic cloud fallback. Concurrency cancels
superseded manual runs on the same branch.

A self-hosted runner registered to another repository is not automatically
available here. Use local checks on the current machine or a separately
configured trusted machine; do not automatically execute public PR code on
personal runners. Routine verification does not require a new runner service.

## Canonical Zerg unit rendering

Canonical Zerg uses prealigned unit sprite words generated from the same indexed
pixels. Refresh an older prepared workspace before building the new renderer:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 USA ROM" --profile zerg
./dev sc64 build --profile zerg
./dev sc64 test --profile zerg
```

Preparation requests `--prepack-unit-words` for the shared Zerg sprite bank.
Status/build reject stale headers missing the metadata marker or payload.
The extra data stays local; sprite/save layouts and the Z4 filename/identity
are unchanged. Explicit legacy Z3/P3 retain their earlier defaults.
For a renderer comparison, `make -B zerg ZERG_UNIT_WORD_FLAGS=` disables the unit
word path while retaining the other canonical tuning. Canonical Terran also
uses packed unit words; prepare its own bank with `--profile terran`. Its
comparison override is `make -B terran TERRAN_UNIT_WORD_FLAGS=`; force the
rebuild because Make does not track changed command-line flags. The
[Terran audit](audits/2026-10-26-terran-word-promotion/README.md) records the
current stock-combat replay evidence and memory limits. See the
[rendering audit](audits/2026-10-26-unit-word-rendering/README.md) for exact
pixel/state checks, measurements and memory limits.

## Directional unit sprites

Canonical Terran, Zerg, Protoss and Showcase builds select directional frames
from the source unit images instead of always drawing frame zero. Refresh an
older workspace with `./dev sc64 prepare --rom PATH --profile terran` (or
`zerg` / `protoss`). Preparation writes the private bank under
`.local/sc64/facing/generated/`; status/build reject older banks without the
directional metadata and packed-frame payload. The source image's directional
flag determines whether its frames represent headings; frame count alone does
not qualify an image.

Ground units face their movement waypoint, flyers face their destination, and
units aim toward their target when firing. Facing is presentation state: a new
game, scene restart, accepted load or reused actor slot starts at north until
the unit moves or fires. It does not change the save format or combat rules.
This adds directional idle poses, including Tank hull/turret layers; it does
not implement the original walking cycles, turn rates or independent turret
tracking. Canonical campaigns and Showcase use packed directional frames.
`make -B showcase SHOWCASE_UNIT_WORD_FLAGS=` selects the indexed comparison path.

## Source weapon and attack animations

Canonical race ROMs and Showcase now require both the source weapon bank and
full attack-pose bank. Refresh existing inputs with `./dev sc64 prepare --rom
PATH --profile terran` (or `zerg` / `protoss`) before building. See
[Native weapon effects](native-weapon-effects.md) for coverage, local checks
and the remaining differences from the source engine.

Weapon frames, from this bank and from the Tank effect bank below, are hidden
per 16-by-16 map tile. When every tile under a clipped frame is visible, as in
the fully revealed showcase arenas, each row is drawn in one call; otherwise
the renderers split rows at tile boundaries and skip hidden tiles. Both paths
produce the same pixels.

## Tank firing effects and siege splash

Canonical race builds and Showcase use source Tank muzzle and target-impact
frames. Refresh their private inputs with `./dev sc64 prepare --rom PATH
--profile terran` (or `zerg` / `protoss`). The standalone importer is
`python3 tools/sc64_tank_source_vfx.py --out .local/sc64/tank-vfx/generated`.
It verifies the USA ROM, weapon/image chains and supported script sequences;
generated graphics, lookup tables and manifests stay ignored under `.local/`.
`make -B showcase CANONICAL_WEAPON_FLAGS= SHOWCASE_SIEGE_FLAGS=` selects the earlier weapon behavior
for comparison. The same override applies to the three canonical race targets.

Showcase enables the `SC64_SIEGE_SPLASH` prototype. Campaign builds retain
their earlier primary-target damage rule: the stock splash change causes the
Z3 normal-controller completion regression to reach its 180000-update limit,
so campaign promotion requires fresh completion qualification. This does not
block the source firing graphics or directional rendering. In Showcase,
siege-mode shots apply radial damage to eligible nearby ground actors,
including allies, with full/half/quarter packets at inclusive source radii
10/25/40. Distance uses the target's source rectangle. Armor and explosive size
scaling follow the radius split; inner-ring splash can hit burrowed actors.
The shooter, loaded units, air units and invincible actors are excluded.
An unknown nearby actor identity falls back to primary-only damage rather than
guessing its dimensions. HP/shields retain the engine's integer precision.

The effects preserve source strength pixels and use a destination-dependent
light lookup projected into the GBA palette. The N64 light-table binding is
inferred, and palette reduction changes colors; this is not a pixel-identical
N64 rendering claim. Source waits are converted from 24 to 30 simulation
updates per second. The engine still applies damage immediately when firing;
source visual attack windup now plays, while gameplay windup, independently
moving turrets and a complete projectile state machine remain unfinished. The visual pool is cosmetic and carries no
pending damage or new save fields.

## Canonical minimap rendering

Canonical Terran, Zerg and Protoss builds and Showcase batch the same 24-by-24
visibility raster through the existing sprite compositor. No new private inputs
are needed. `make test-sc64-minimap-terrain` checks scalar, batched and
word-compositor paths without a source ROM. To compare a prepared profile with scalar drawing,
use `make -B protoss CANONICAL_MINIMAP_FLAGS=` (replace `protoss` with another
canonical target as needed); force rebuilding when changing flags.

The [minimap audit](audits/2026-10-26-minimap-terrain/README.md) records the
pixel oracle, byte-identical public replay and scoped ARM timing. P3/Z3 legacy
and staged profiles retain their existing flag selections.
Word-sprite builds phase-align the temporary raster with its screen position;
the [alignment audit](audits/2026-10-27-minimap-phase/README.md) records the
clipping oracle and small measured cycle reduction.

## Source geyser art

Imported profiles draw the existing raw-188 geyser sprite instead of the
original green diamond. The painter and static-cache key use the sprite's
actual bounds, including partially visible geysers at the playfield edges.
Existing prepared banks already contain this art; no new private inputs or
save fields are required. The asset-free profile retains its authored marker.
This selects the static source frame; native gas-smoke animation is separate.
With the canonical Protoss inputs prepared, `make test-sc64-geyser-art` checks
source pixels, edge clipping, fog, all three gas buildings and warm-cache reuse.

## Source construction art

Prepared imported race builds now use the source construction art, including
the animated Protoss Warp Anchor during the build timer and source image
underlays/overlays. The importer covers all 50
stock constructible building IDs, including the Greater Spire morph. `./dev sc64
prepare` generates the private header for race profiles; older prepared
workspaces should regenerate it using the command below. `SC64_CONSTRUCTION_ART=0`
selects the older authored fallback for diagnostic comparisons.

When updating an existing prepared workspace, rerun `./dev sc64 prepare` with
your ROM and profile to refresh its sprite banks as well as construction art.

```sh
python3 tools/sc64_construction_sprites.py
make test-sc64-construction-source construction-visuals
```

The importer defaults to the already-supplied verified USA ROM at
`.local/sc64/rom/StarCraft64-USA.z64` and the prepared palette remap at
`.local/sc64/backwater/generated/palette-remap.bin`. Use `--rom` and
`--palette-remap` for other local paths. `--header` and `--out` can select
another ignored output directory; add its include path when compiling.
It requires the same pinned extractor used by ordinary preparation, and
does not fetch a ROM or reference checkout. Generated art and its audit
manifest stay under `.local/sc64/construction/generated/`.

The ROM is `build/StarCraft-Construction-Preview-GBA.gba`; its menus and
mission set match the canonical unified profile. It adds no campaign entries.
Canonical imported race profiles enable `SC64_CONSTRUCTION_SOURCE_ART` and
depend on the generated construction header. Original-art builds do not use
this flag or require the header.

Warp Anchor image 211 preserves its source intensity bytes and uses a
40-row destination-color lookup for its blue-white glow. The importer reads the
actual UI palette, world palette transform, and UI overrides from the runtime
sources, then projects the source lookup through those displayed GBA colors.
Regenerate the bank after changing those palette sources; preparation and build
checks reject older banks without the modifier payload. The N64 BFire table
association is inferred from its structure and reference light-table ordering,
not proven by an executable cross-reference. GBA color quantization remains an
approximation. The bright core and saturated blue/cyan halo are consistent with
[the N64 longplay at 5:03](https://www.youtube.com/watch?v=rkyLf7lELXE&t=303s).

With prepared inputs, `make test-sc64-protoss-warp-color` checks early, middle,
and late source frames through the production building overlay, including
map/HUD clipping with and without the scene-clip API. Asset-free Python tests
check palette parsing against the C runtime and synthetic lookup projection.

The bounded sampler uses existing saved build progress and adds no per-actor
state or save-generation change. Stage timing is an adaptation: Terran uses
progress thresholds instead of native HP/order states; Zerg uses progress
quarters and halves. Animation waits use a local 24-Hz source clock derived
from the 30-Hz simulation. Paused progress freezes the sampled visual, and a
load resumes the same frame. Child layers retain their source spawn clocks and
follow-main frame links. This bounded visual sampler does not reproduce the
complete native order/IScript machine: worker animation, finish signals,
dynamic resource-gas overlays and Protoss post-timer warp states remain
unfinished. See
[Validation](VALIDATION.md#source-construction-art-preview) for coverage.

## Death animations

Units and buildings no longer vanish when they die. Every build, including
the asset-free original-art skirmish, plays a short death effect in the
family StarCraft 64 uses for that actor:

- Marines, Ghosts, Medics and critters collapse and leave remains.
- Zerg ground units burst into gore and leave remains.
- Zerg air units splash.
- Vehicles, aircraft, SCVs and Firebats explode in orange, with a larger
  blast for tanks, Science Vessels and Battlecruisers.
- Protoss units release a blue energy burst. In the source, Zealots,
  Dragoons and Templar collapse instead; canonical source-art builds show
  that, while the authored fallback uses the burst for them too.
- Buildings explode in their race's colors and leave rubble for 30 seconds.

The families and their timings come from the source IScript Death
animations: frames of three source updates, 7–14 frames per animation, and
remnants that appear when the collapse ends, or under the explosion for
buildings. Asset-free builds draw authored approximations with GBA
primitives (`starcraft/sc64_death_vfx.h`, `sc64_death_vfx_render.h`).

The effects are presentation only. At the start of every other update,
and on the first result-screen update, `death_vfx_update()` looks for actor
rows that changed from alive to dead with HP at or below zero. A death
therefore appears one or two updates late, and the final deaths of a mission
are always seen. It reads those rows and writes nothing back. The hook is a
single bare call at the top of `game_update`: in some P7 probes, any larger
addition to `game_update` changes GCC's inlining and overflows IWRAM.
Scripted removals keep their HP, so they play no death. Units inside
transports or Carrier hangars are not on the battlefield, so they play none
either. New games and loads clear the pool and re-prime the alive shadow, so
dead rows already in a loaded world never replay. The effects use no RNG,
add no actor or save fields, and are clipped to visible battlefield cells
above the cached lower HUD. The pool holds 16 effects in race profiles and
12 in original-art builds (20 bytes each, in EWRAM). When it is full, a new
death replaces the oldest rubble-only effect first. Build with
`-DSC64_DEATH_VFX_DISABLED` to compile the feature out.

```sh
make test-sc64-death-vfx test-sc64-death-vfx-default
```

These asset-free checks run in `./dev test`. With the unified inputs
prepared, `make test-sc64-death-vfx-unified` (part of `make test-unified`)
runs the same game fixture on the race engine. With the full Terran, Z7 and
P7 inputs prepared, `make test-sc64-death-vfx-profiles` also covers those
actor sets. Each game fixture is built twice, once with the feature compiled
out. Both builds run the same combat workload, rendering every frame, and
must print the same field-wise state digest and the same SRAM digest after
saving.

### End-of-mission outro

The simulation still stops on the update that decides victory or defeat.
The result screen no longer replaces the battlefield at once. The final
deaths keep animating over the frozen battlefield on a presentation clock,
then the result panel grows open over 0.2 seconds, much as StarCraft 64
shows its defeat or victory dialog after the last structures die. The outro
lasts at least 0.8 seconds and ends when every collapse or explosion has
finished, capped at 3.2 seconds. Destroying a final building takes about
1.8 seconds.

Input on the result screen is unchanged: A and B act on the first press,
even during the outro, so controller replays and showcase restarts behave as
before. The outro counter is not saved, and a world loaded on its result
screen shows the panel immediately. `test-sc64-death-vfx-default` and
`test-sc64-death-vfx-unified` check these result transitions:

- the outcome arrives on the same update;
- the final deaths are picked up on the first result-screen update;
- the simulation stays frozen;
- the panel is held back until the final explosions end;
- the panel opens over 0.2 seconds;
- input works during the outro;
- a load shows the panel at once.

The P3 frame-coverage oracle also renders a mid-outro result frame over
poisoned buffers with `SC64_NO_FRAME_CLEAR`. Both frame-coverage oracles now
render the result screen through the production `draw_result_screen()`.

The feature-on/off workload digests include the outcome frame and 120
frozen result updates.

### Source death and corpse art

Canonical Terran, Zerg, Protoss and Showcase ROMs use the extracted death and
corpse frames. This replaces the authored purple Zerg puddles that formed
segmented health-bar-like rows in Siege Line. Refresh older prepared inputs
with `./dev sc64 prepare --rom PATH --profile terran` (or `zerg` / `protoss`)
before building. The source bank must have version 2 timing metadata and a
nonempty pixel payload; setup/build reject stale banks. Keep generated banks local to
each checkout when branches use different formats; parallel preparation in
another worktree must not overwrite this checkout's generated header.

The same art is available in the separate unified preview:

```sh
python3 tools/sc64_death_sprites.py
make test-sc64-death-source death-visuals
```

The importer authenticates the same USA ROM and BOLT tables as the
construction importer. For each source unit or building id, it follows
`units.dat` → `flingy.dat` → `sprites.dat` → `images.dat` to the actor image.
It then compiles that image's Death animation into at most eight independent
layers:

- collapse frames played on the actor's own GRP;
- explosion overlays, plus any overlays they spawn (`imgol`/`imgul`);
- remnant and rubble sprites (`lowsprul`).

Each layer keeps its source frame order; a fixed `wait N` spans N source
updates. Random waits use a deterministic midpoint, without an extra update. IScript `end` removes the
layer, as in the source. Remnant streams are capped at 720 source updates
(30 seconds). Only the frames that the animations reference are imported.

The compiler is a whitelist. It ignores sounds, flip and direction
instructions, and follows the grounded path of `liftoffcondjmp`. Anything
else makes the actor keep the authored fallback. Remapped fire images
(draw function 9: the orange Terran and blue Protoss explosions) store blend
intensities rather than colors. GBA mode 4 cannot blend, so these map to
fixed UI-palette ramps: the faintest band is dropped and the next is
checkerboard-dithered. Generated art and its manifest stay under
`.local/sc64/death/generated/`. Use `--rom`, `--palette-remap`, `--header`,
`--out` and `--upstream` for other local paths.

The importer currently compiles 167 actors from 61 image banks (about 770 KB
of pixels). Canonical builds also prepack combat-unit collapse and remnant frames
for the existing word compositor (about 891 KB of ROM data, no new RAM).
Neutral actors and buildings use the same source pixels through indexed drawing
to fit the complete Terran ROM. The indexed fallback handles HUD-edge clipping. Adding
`-DSC64_DEATH_INDEXED_REMNANTS` to the source death flags selects identical
indexed pixels for a performance comparison. The preview ROM is `build/StarCraft-Death-Preview-GBA.gba`; its
menus and missions match the unified profile. Canonical race builds and
Showcase select this bank by default. `CANONICAL_DEATH_FLAGS=` selects the
authored comparison path when forcing a rebuild; it leaves gameplay unchanged.
The original-art build and explicit older profiles retain authored effects.
Unsupported source actors also keep their bounded authored fallback.
`make test-sc64-showcase-death-source` exercises actual Siege Line deaths
and compares rendered layers to source-frame composites. See
[Validation](VALIDATION.md#canonical-source-death-art-2026-10-26) for current
coverage and limits; older preview measurements remain historical.

## Install the tools

### macOS

Install Apple's Xcode Command Line Tools for Clang, Make, and Git, then install the ARM toolchain and Python with Homebrew:

```sh
xcode-select --install
brew install arm-none-eabi-gcc python
```

Homebrew's [ARM GCC formula](https://formulae.brew.sh/formula/arm-none-eabi-gcc) provides the `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, and `arm-none-eabi-size` commands. If you want to play, install the optional [mGBA app](https://formulae.brew.sh/cask/mgba-app):

```sh
brew install --cask mgba-app
```

The wrapper checks `PATH` and also looks for `/Applications/mGBA.app/Contents/MacOS/mGBA`.

### Ubuntu 24.04

Install the build and test tools with the system package manager:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends gcc-arm-none-eabi binutils-arm-none-eabi clang make python3 git
```

To play, install the optional mGBA SDL package from Ubuntu's [package archive](https://packages.ubuntu.com/search?keywords=mgba):

```sh
sudo apt-get install mgba-sdl
```

On Ubuntu, the emulator executable may be `/usr/games/mgba`; if it is not on `PATH`, set `MGBA=/usr/games/mgba` when running the play command. If the package is unavailable for your Ubuntu release, use the package search page to check the release and enabled repositories.

Other systems need equivalent versions of these tools. `./dev doctor` reports any missing required command. To check the optional emulator too, use `./dev doctor --emulator`.

## Asset-free workflow

After cloning as described in the [README](../README.md), run:

```sh
./dev doctor
./dev build
./dev test
```

Doctor should print `OK` for ARM GCC, ARM objcopy, ARM size, Make, Clang, Python 3, and Git. Build creates `build/StarCraft-GBA.gba` and verifies the GBA ROM. Test runs the asset-free host checks and exits with status 0 when they pass. `make build/StarCraft-GBA.gba` builds the ROM without the wrapper; `make verify` builds and verifies it.

Playing is optional and requires mGBA:

```sh
./dev doctor --emulator
./dev play starcraft
```

The wrapper launches the built ROM and keeps local emulator state in `.local/play/starcraft/`. The game code is in `starcraft/`, including `starcraft/game.c`; GBA display and input support is in `common/`. Host-side behavioral tests are in `tests/`, and build, conversion, validation, and emulator helper scripts are in `tools/`.

## Optional unified profile

The unified campaign/custom-game profile requires a user-supplied StarCraft 64 USA ROM that you are entitled to use. The supported input is a `.z64`, `.v64`, or `.n64` file, or a ZIP containing exactly one 32 MiB N64 image. Preparation normalizes byte order and verifies SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. It does not download a ROM. The pinned `sc64-maps` extractor is GPL-3.0-or-later and is not vendored; the first preparation clones it from its public upstream into ignored `references/`, which requires Git and network access.

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile unified
make unified
make test-unified
./dev play unified
```

Preparation writes extracted and generated inputs under ignored `.local/`; ROM builds and reports go under `build/`. Do not commit or distribute the ROM, imported maps or art, generated headers, or captures containing game graphics. Default-profile contributors do not need this workflow.

See [SC64 unified](SC64-UNIFIED.md) for implemented missions and limitations, and [SC64 development notes](SC64.md) for import details.

The default `./dev test` includes the asset-free weapon-effect renderer check. After preparing the unified inputs, `make test-unified` also runs actual-attack visual checks and pixel/cache checks for Protoss shield bars; those checks keep damage application immediate and compare cached HUD output with a full redraw.

## Optional StarCraft 64 sound effects and music

`./dev sc64 prepare` also runs `tools/sc64_audio_pack.py`, which converts the
ROM's sound effects and music into a private bank under ignored
`.local/sc64/audio/generated/`: `sc64_audio_generated.bin` (signed 8-bit PCM),
`sc64_audio_generated.h` (sample and per-unit index tables) and a manifest
with counts and hashes. To refresh only the bank:

```sh
python3 tools/sc64_audio_pack.py
python3 tools/sc64_audio_pack.py --music 0,4,5   # fewer music tracks, smaller ROM
```

Every imported (`SC64_*`) ARM profile links the bank automatically when the
header exists. `make <target> SC64_AUDIO=0` builds the same profile silent, for
example for performance baselines; `SC64_AUDIO=1` fails if the bank is absent.
The default original-art ROM never uses it: it keeps short PSG placeholder
tones for the same events. The bank is about 15.6 MB, so imported ROMs grow
to the 32 MiB cartridge size. Like other imported data, the bank and any audio
captured from it are game content: do not commit, attach or distribute them.

What is imported:

- StarCraft 64 keeps its sounds in the BOLT archive. `014/000` is a
  1144-entry table indexed like PC `sfxdata` ids, pointing into an unlisted
  5.6 MB blob at the end of the archive; `015/000`-`00F` are 16 music streams.
  Both use mono IMA ADPCM (high nibble first) at 11025 Hz. The N64 mixes them
  on the CPU to 11025 Hz stereo.
- The converter decodes and linearly resamples them to 10512 Hz, the closest
  GBA rate that consumes exactly 176 samples per video frame. Sounds are chosen
  from the ROM's own `units.dat` (ready, selection and command responses, up
  to four variants each) and IScript (`playsnd`, `playsndrand`,
  `playsndbtwn`, `attackmelee` in attack and death animations, following
  one level of spawned explosion sprites): 501 distinct clips. Annoyed
  responses are not imported.
- Music roles come from the resident N64 selector, which loads resource
  `0x1500 + track`: 0 is the menu theme; Terran, Zerg and Protoss play
  4/5, 8/9 and 12/13 in game, alternating as each ends; 2/3, 6/7 and 10/11 are
  the mission-end stingers chosen by the result flag. Which stinger is victory
  is inferred from that flag and has not been checked by ear. Tracks 1, 14 and
  15 are not imported.

Engine design (`common/audio_mixer.h`, `common/gba.c`):

- Timer 0 runs at 10512 Hz and feeds both Direct Sound FIFOs. DMA1 plays a
  352-byte double buffer of sound effects on channel A; DMA2 streams the
  current music track on channel B (50% volume) straight from ROM, so music
  costs no mixing. A 64-byte zero guard follows the double buffer: when the
  interrupt is delayed (for example behind a long DMA3 blit) the FIFO reads a
  few samples of silence instead of unrelated memory.
- A VBlank interrupt drains an 8-entry request ring, assigns requests to four
  fixed voices and mixes 176 samples with saturation. Voice 0 is reserved for
  unit responses, which interrupt each other as in StarCraft; effects take a
  free voice or steal a lower-priority, older one, and the same effect is not
  retriggered within four frames. Samples are word-aligned, so the mixer
  reads four samples per ROM word and adds them in two 16-bit lanes, clamping
  only lanes that overflow. It runs as Thumb code from ROM: IWRAM has no room
  for it.
- The mix buffer and the event snapshot live in the unused VRAM gap between
  the two mode-4 pages (`0x06009600`-`0x06009FFF`), so EWRAM grows only by the
  mixer state.
- `starcraft/game_audio.h` derives events at the start of each rendered frame
  from read-only comparisons with the previous frame: a raised weapon
  cooldown on a visible unit (a shot), alive 1->0 (a death), a new player unit
  (ready), a cleared construction flag (completed building, which plays the
  building's selection sound because StarCraft has none), and a new selection
  or changed orders after input (selection and command responses). It never
  writes simulation state or uses the game RNG, at most two effects start per
  frame, and it ignores any tick discontinuity such as a load or restart.

Check the engine with `make test-audio` (asset-free, part of `./dev test`) and
`make test-game-audio-unified` (part of `make test-unified`). Measured costs
and limits are in [Validation](VALIDATION.md#sampled-audio-2026-10-25).

## T4 internal actor/world checks

The Jacobs Installation data and actor rules have an internal, gated host-test
path. This is not a playable campaign entry: the frontend catalog remains
closed to T4, and no frame-rate result is claimed. After preparing the local
unified inputs, run:

```sh
make test-sc64-t4-actors
make test-sc64-t4-storage sc64-t4-storage-probe
```

The actor tests load the 95 source placements, check supported actor roles,
sight, combat and bounded rescue behavior, and validate the special Beacon's
save representation. Storage tests cover the T4-only 128×128 layout. Both
targets use private generated inputs under ignored `.local/` storage. The
initial actor/storage stage did not implement doors, traps, teleports, trigger
objectives or completion behavior. Later gated runtime checks are described
below. See [Validation](VALIDATION.md) for the exact test coverage and limits.

The private numeric scenario import and the asset-free trigger/query core have
separate checks:

```sh
make sc64-t4-scenario test-sc64-t4-scenario
make test-sc64-t4-triggers test-sc64-t4-queries test-sc64-t4-actions
```

The first line needs the retained records produced by local race preparation.
Re-run preparation after updating an older checkout: T4 retention now includes
FORC, OWNR, SIDE, UPRP and UPUS, in addition to the original eight sections.
It reconstructs the full source sections from the generated C schema and
checks their canonical digests. The second line needs no ROM or generated
data and is included in `./dev test`. These commands document the earlier
numeric-import and callback-only stage; see the gated runtime status below for
subsequent integration.

World-effect callbacks have a separate internal fixture:

```sh
make test-sc64-t4-effects
```

This requires the same private prepared inputs. It runs queries, removals,
location movement and bounded group teleport effects against synthetic engine
states, with creation and doodad calls stubbed at their callback boundaries.
It does not start the T4 trigger scheduler, save that state or prove mission
completion. This callback-only fixture remains useful for the standalone
effect API; the later runtime fixture also exercises those effects in context.

## Gated T4 scenario runtime

The internal Jacobs Installation runtime binds all 95 imported UNIT rows and
26 unit-backed THG2 props into the fixed pools: 121 initial actors total
(87 units and 34 buildings). It evaluates the 20 retained doodad-state
actions, handles supported creation and map-revealer actions, updates door
collision/path blocking, and runs the bounded trigger contexts. Standard
profiles still exclude T4; the combined Terran experiment described below is
the opt-in normal-menu build.

Door presentation uses an authored immediate-state policy: enabled/closed
doors draw source frame 0, disabled/open doors draw frame 9; the paired right
door types are mirrored. Traps draw static frame 0 when enabled and are hidden
when disabled. Door animation timing and trap animation are not reproduced.
Disabling a trap suppresses firing and drawing but leaves its actor targetable;
native T4 initial targetability has not been established. This is an authored
compatibility policy, alongside the immediate door endpoints.
These visual choices are bounded compatibility policies, not proof of native
N64 IScript behavior. The runtime save generation is version 10 and persists
the trigger and doodad state.

T4 distinguishes raw-20 Marine Raynor from the older raw-19 Vulture alias.
Marine Raynor has the pinned source weapon and armor rules; older T4 saves
are rejected after this compact actor-type change. To check acquisition and
damage against trap, ground and air targets, run
`make test-sc64-t4-raynor-combat` after local preparation.

After preparing the local T4 inputs, the focused host checks and ARM link probe
are:

```sh
make sc64-t4-scenario \
  test-sc64-t4-effects test-sc64-t4-creation test-sc64-t4-traps \
  test-sc64-t4-prop-art test-sc64-t4-scenario-save test-sc64-t4-runtime \
  sc64-t4-scenario-probe
```

The runtime test checks the full opening roster, 20 doodad toggles, door
collision/path-grid changes, pause/save/load continuity, and trigger execution.
Its win/loss fixtures directly place or remove scenario actors; they are
synthetic rule-integration checks, not controller-played mission completion.
The link probe checks the complete opt-in engine profile. No claim is made for
full native trigger eligibility, original animation behavior, full campaign
playability, or near-30-fps operation.

For controller/replay development, build the separate internal ROM:

```sh
make sc64-t4-replay test-sc64-t4-frontend-entry test-sc64-t4-ping \
  build/test-sc64-t4-controller
```

`build/StarCraft-T4-Internal-GBA.gba` has an explicit
`SC64_T4_REPLAY_ENTRY` flag. On HOME, press START+SELECT together to open the
T4 briefing, then A to deploy. Standard Terran builds still list only their
first two missions. The on/off frontend fixtures exercise this distinction without
private inputs. The ping fixture checks the live minimap marker, pulse,
clipping and expiry. The controller runner records input attempts under ignored
`.local/` storage and exits successfully only on a win; a limit/failed attempt
is useful diagnostic evidence but does not establish completion.

Replay a recorded attempt on ARM, or measure a bounded prefix:

```sh
python3 tools/sc64_t4_controller_replay.py --trace .local/sc64/t4-controller-attempt.json
python3 tools/sc64_t4_controller_replay.py --trace .local/sc64/t4-controller-attempt.json --ticks 5000
```

The helper validates launch inputs and checkpoint fields before starting
mGBA. It writes controller scheduler entries, observes gameplay through ELF
symbols/DWARF, and saves private checkpoint/progress reports with ROM and trace
hashes. A replay that reaches a recorded limit or loss verifies that attempt;
only a matching source-trigger win can report `verified_win`.

For changes to cached terrain or prop presentation, run all eight combinations
of scrolling, clear suppression and static-key culling:

```sh
make test-sc64-t4-frame-coverage
```

The fixture compares both poisoned alternating framebuffers against an
independently cleared, freshly rendered full frame. It covers door/trap state
and binding changes, camera edges, fog, pause/results and restart/save/load.
`T4_OPTIMIZATION_FLAGS` enables count memoization, scrolling, clear suppression
and static-key culling for this internal profile. For a comparison build, use
`make -B sc64-t4-replay T4_OPTIMIZATION_FLAGS=` and freeze its ROM/ELF pair
before rebuilding the optimized default.
Diagnostic builds with `SC64_DIAGNOSTIC_UPDATE` expose separate rescue, trigger
and trap cycle counters, which the controller replay report records when present.

## Combined Terran T1–T4 experimental campaign

The combined target builds Terran missions 1–4 together, including T3 and
Jacobs Installation (global mission ID 7). It requires the same local Terran
preparation and private generated T4 data as the internal runtime:

```sh
make terran-t4
make test-sc64-t3t4-runtime test-sc64-t3t4-save \
  test-sc64-t3t4-skirmish test-sc64-t3t4-controller \
  test-sc64-t4-campaign-frontend
```

The combined runtime suite also covers the T3 survival win, save/resume and
HUD behavior. The skirmish fixture exercises the map/race/opponent setup
matrix for this profile.

This produces `build/StarCraft-Terran-T4-GBA.gba`. T4 is reached from the
ordinary campaign menu: press A from HOME, move RIGHT to page two, move DOWN
to Jacobs Installation, press A for its briefing, then A again to deploy.
The distinct `sc64-t4-replay` build still uses START+SELECT on HOME as an
internal development shortcut. The standard campaign catalog remains
unchanged; the internal replay target retains its separate entry and save
identity.

The combined ROM has save generation 11. It rejects saves from older
generations, including generation 10 used by the standalone internal T4
target, so use a separate save file. Its host and ARM controller runs both
complete the mission through the normal campaign entry. On the earlier
`181a5063` mGBA run, the win occurs at frame 9,331 (host tick 9,378); 160 input transitions
peak at 62 per scheduler page, and 11 observations match. The run averages
23.15 effective fps over 7,971 counted live frames, with a worst observed
interval of 20.71 fps. This remains below the 30-fps goal and is emulator
timing, not a physical GBA result. The ROM is 2 MiB; ROM SHA-256 is
`181a5063515a362970ce5902561ddbb61410ad43d91c248160d03439244c78bb`, and ELF
SHA-256 is
`5fc7296475ac58c2f20f624ca4bcc7a127b0bb27e01dbeabd6858e7f3a6da5cd`.
The ELF has 1,857,188 bytes of text, 388 bytes of data and 241,872 bytes of
BSS; BSS ends at `0x0203b258` and IWRAM at `0x03004ec8`, within the existing
guards.

A separate 10,000-tick neutral skirmish run on Blood Bath with three opponents
of independently selected Zerg, Protoss and Terran races on the earlier
`181a5063` build averaged 29.48 fps (observed intervals 28.58–29.86). This is
historical bounded-scenario evidence, not a measurement of the current ROM, a
full-battle worst case or a campaign performance result. Across current
experimental progression builds, Terran 1–5, Zerg 1–3 and Protoss 1–3 are
playable; the remaining 19 catalogued missions are unavailable.
`SC64_DIAGNOSTIC_UPDATE` exposes rescue, trigger and trap counters;
`SC64_DIAGNOSTIC_TERRAIN` exposes terrain reuse/event and minimap counters.
These are instrumentation, not separate performance results.

The current combined default enables `SC64_UNARMED_QUERY_SKIP`. Its frozen
2 MiB ROM SHA-256 is
`d867886700a4c0a7351165f135ab720f37a6442327d529074775f4ac29d45077`; the
matching ELF SHA-256 is
`3788c697cafe3f5693ee9b752013a6dee9cdc9a1c6673a90d838d920985d1202`. The
same controller win, 11 gameplay checkpoints and three captures match the
earlier combined build byte-for-byte for reported state and images. This
version averages 23.53 effective fps over 7,971 counted frames, with a worst
interval of 21.17 fps. It remains below the 30-fps target. The experimental
576-byte minimap-interior buffer was removed after an exact 5,000-tick
comparison showed no measurable gain.
A fresh 10,000-tick Blood Bath skirmish on this current ROM, with a Terran
human and Zerg, Protoss and Terran AIs, averages 29.49 fps (intervals
28.58–29.86). Every observed checkpoint remains in play; this is a bounded
neutral-input scenario, not a full-battle performance guarantee.

To replay the recorded normal-menu controller trace against the combined ROM,
pass the matching ROM and ELF explicitly because the helper defaults to the
separate internal T4 target:

```sh
python3 tools/sc64_t4_controller_replay.py \
  --trace .local/sc64/t3t4-controller-smoke.json \
  --rom build/StarCraft-Terran-T4-GBA.gba \
  --elf build/starcraft-t4-combined.elf
```

## T5 source import checks (historical stage)

Antigan Revolt (Terran global mission ID 8) has an import-only data path. The
private importer retains its 96×96 Badlands map and all 168 non-start UNIT
rows: 79 units, 48 buildings and 41 mineral fields. Four start markers are
excluded from the live placement rows. At this import-only stage, these checks
did not add a mission to the catalog or establish playability; later combined
campaign completion is recorded in the current summary at the end of this
guide.

After preparing the local ROM-derived input, regenerate the race tables, then
the T5 scenario tables and run the focused source/import checks:

```sh
python3 tools/sc64_races_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_t4_scenario_pack.py --profile t5
make test-sc64-t5-import test-sc64-t5-scenario \
  test-sc64-t5-actor-rules test-sc64-resource-actions
make sc64-t5-availability test-sc64-unit-availability test-sc64-t5-availability-engine
```

Current T5 preparation also retains the exact PUNI record and generates a
342-byte availability bitmap in ignored storage. Re-run preparation after
updating an older checkout whose T5 records lack `PUNI.bin`. The internal
runtime uses this source table to reject disabled production before payment;
existing placed units remain usable. It also checks queued products before
accepting a saved T5 game.

`make test-sc64-t5-actors test-sc64-t5-ai` also runs asset-free sanitizer
checks for distinct Kerrigan, Raynor Vulture and Spider Mine actor rules,
source bounding boxes, weapon upgrades, and the opaque AI-script dispatch
boundary. Dispatch records accepted requests; it does not execute the original
script bodies. The mine remains excluded from ordinary weapon targeting.

### Internal T5 runtime probe

The separate internal runtime profile now binds the retained trigger program
to the engine. With both T4 and T5 private scenario headers prepared, run:

```sh
make test-sc64-t5-engine test-sc64-t5-world test-sc64-t5-combat test-sc64-t5-save
make test-sc64-transport-rules test-sc64-t5-tech
make test-sc64-transport-engine test-sc64-transport-queries
make test-sc64-t5-render
make sc64-t5-engine-probe
python3 tools/sc64_t5_engine_probe.py --ticks 3000
python3 tools/sc64_t5_save_smoke.py
```

The first command runs sanitizer fixtures against the real engine. They use
direct world setup to check the full roster, region-gated enemy resource
grant, rescue conversion, mine and hero combat, and mission outcome rules.
They do not demonstrate ordinary-controller completion.

The probe ROM automatically enters global mission 8 for a bounded neutral-input
ARM run. Its internal entry remains separate from the combined normal-menu
campaign build described below.
The probe uses save generation 14; the normal T1–T4 build keeps generation 11.
Use separate save files for these profiles. T5 shares the existing 16,328-byte
SRAM payload with T4: four active trigger contexts are stored, and omitted
contexts are checked against their initial state and reconstructed on load.
The save checks cover trigger waits, mine continuation, executed resource/AI
actions, invalid-state rejection before world mutation, and other mission saves.
Reports and captures remain under ignored local output directories.

`make sc64-t5-controller-probe` runs a bounded 60,000-tick host controller
attempt and writes its private trace under `.local/sc64/t5/`. It exercises
the rescue/base sequence, paid tech and supply construction, safe hero
positioning, and grouped Wraith attacks. Its input budget is enforced at each
step. Selection helpers cancel stale command modes, use the actual cursor
lattice for construction, and isolate a clickable group seed when necessary.
Private checkpoints include live Marine/Wraith counts and hero survival;
the replay reads those optional counts from the ARM actor pool. Queue/supply
diagnostics in a host trace are not independently verified by the ARM helper.
Ending at its limit is an unfinished attempt, not a mission-completion test.
For a longer bounded attempt, use `./build/sc64-t5-controller-probe --limit
75000 --trace .local/sc64/t5/controller-probe.json` after building the target.
The earlier ferry policy and evidence are retained in prior source revisions.
The separate
`SC64_T5_CAMPAIGN_ENTRY` menu flag is covered by asset-free frontend checks
and enabled only in the internal replay target:

```sh
make sc64-t5-controller-replay
python3 tools/sc64_t5_controller_replay.py --trace /path/to/private-controller-trace.json
```

That diagnostic ROM starts at HOME and uses the ordinary campaign menus.
Standard race builds and the combined Terran T1–T4 build keep T5 unavailable.
The replay helper writes controller/scheduler inputs and reads mission state;
its timing uses the GBA hardware clock. Private traces, reports and captures
stay local.

For a diagnostic update-stage breakdown, build `make sc64-t5-profile` and pass
`--rom build/StarCraft-T5-Profile.gba --elf build/starcraft-t5-profile.elf`
to the same replay helper. It adds update timing counters, including rescue
contact, mines, and trigger/AI work. These counters add measurement overhead;
use the ordinary replay ROM for final performance measurements. `--ticks`
limits a run to a prefix, which does not establish a complete mission win.
The renderer fixture compares T5 cached frames with cleared fresh renders
across timer boundaries, visible buildings/minerals, depletion, fog and camera
movement. Its key reuse adds no persistent runtime storage.

The internal T5 profile now supports paid Starport/Control Tower/Dropship
production and eight-point cargo capacity. Select ground troops and press A
on a friendly Dropship to board. Select the ship, open B commands, choose
UNLOAD CARGO and press A on the destination. Factory, Starport and Control
Tower entries follow STOP / HOLD in the scrolling command list. Select a
Factory or Starport and press R to cycle its production types, then A to queue.
The Control Tower command requires a selected, completed Starport and builds
its attached add-on automatically. Cargo stays in the existing actor/save
records; one derived activity word avoids cargo scans when no transport
orders are active. Generation 14 rejects generation-13 internal saves.
These controls remain experimental and do not enable the mission catalog.

Opaque AI scripts use explicit GBA policies: rescue conversion, passive guards,
clear-combat orders, paid production and small periodic attack groups. Hero
art reuses existing Ghost/Vulture frames. Cloaking, full mine visibility and
animation timing, THG2 sprite decorations, and original AI bytecode are still
absent. The mine's
source-based targeting and splash retain documented integer/timing/movement
approximations; ordinary hero movement and sight still use the compact engine
policies. At that stage, T5 was still an experimental profile; its normal-menu
ARM win and fresh-process save check had passed. The campaign count then stood
at 11 of 30.

The import-only T5 checks cover retained rows, metadata and digests, bounded source actor
rules, and isolated SetResources arithmetic. The actor facts do not establish
cloak or mine-arming behavior, and the resource helper intentionally does not
interpret player-group selection. Those isolated fixtures do not claim T5 runtime,
mission completion, or playability.

### Combined Terran T1–T5 experimental stage (historical)

With the local Terran and T5 private inputs prepared, build the experimental
combined profile and run its focused host checks:

```sh
make terran-t5 test-sc64-t5-combined
```

The ROM is `build/StarCraft-Terran-T5-GBA.gba`; its matching ELF is
`build/starcraft-t5-combined.elf`. It exposes Antigan Revolt through the normal
campaign menu. The ROM uses save generation 14, distinct from combined T1–T4
generation 11. Use separate save files.

The full controller-driven host trace
`.local/sc64/t5/controller-wave20-visible-clusters-200k.json` wins at input
tick 121,547 and game frame 121,261. Repeating the run produces the same
trace; both heroes and the command center survive, all enemy buildings are
destroyed, and no trigger faults are reported. This validates the host
controller scenario. The frozen normal-menu ARM replay also wins at frame
121,261 with both heroes and the command center alive, no enemy units or
buildings, and no trigger faults. It averages 20.85 effective fps over 120,714
counted frames; the worst counted observer interval is 11.95 fps. This is
below the 30-fps target and is emulator timing, not physical GBA validation.

Generate the private winning trace, then repeat the ordinary-menu ARM replay
and fresh-process save check with:

```sh
make build/sc64-t5-controller-probe
./build/sc64-t5-controller-probe --limit 200000 \
  --trace .local/sc64/t5/controller-wave20-visible-clusters-200k.json
python3 tools/sc64_t5_controller_replay.py \
  --trace .local/sc64/t5/controller-wave20-visible-clusters-200k.json \
  --rom build/StarCraft-Terran-T5-GBA.gba \
  --elf build/starcraft-t5-combined.elf
python3 tools/sc64_t5_save_smoke.py \
  --rom build/StarCraft-Terran-T5-GBA.gba \
  --elf build/starcraft-t5-combined.elf \
  --menu-entry --output .local/sc64/t5/combined-save-menu
```

The fresh-process save check passes with generation 14 and the 16,328-byte
payload. It resumes frame 123 at frame 123 and continues to frame 183 without
player-context faults. The ROM SHA-256 is
`9344a66258c3a37e24c61eca04f3b9e95414999d8496f9ea3865f1416ad4ba7b`; ELF
SHA-256 for the frozen replay ELF is
`1a9d2b9a6a8bebdd144502aa57c445f4a150202558a694db4a16b53143141f3a`.
Rebuilding produces the same ROM; GCC's temporary object filename can change
the ELF's nonloaded string table and hash without changing code or layouts.
At that historical stage, the build was opt-in and did not change the standard
race ROMs or combined T1–T4 target. The experimental builds then had 11 of 30
missions qualified as playable; 19 remained unavailable.

## T6 packed storage foundation (internal only)

The T6 storage probe exercises capacity for the retained Norad II source map;
it does not add Norad II to the campaign catalog or implement its mission.
First prepare the usual local Terran inputs with `./dev sc64 prepare --rom
"/path/to/StarCraft 64 ROM or ZIP" --profile terran`, then run:

```sh
make sc64-t6-prepare
make sc64-t6-storage-probe
make test-sc64-t6-import test-sc64-t6-scenario test-sc64-t6-actors
make test-sc64-t6-availability
make test-sc64-compact-building-save test-sc64-t6-storage test-sc64-t6-combined
```

The dedicated preparation command retains the T6 records, generates its
source availability bitmap, and adds its gated source sprites. The optional
availability check compares all 2,736 player/type values with the retained
raw PUNI record. That source permits the human Dropship, Starport and Control
Tower; this check does not exercise acquiring two Dropships in the game.
Run preparation again after ordinary profile preparation, which omits
T6 sprites. A storage build rejects incomplete T6 preparation.

The import retains the 96×96 map and all 216 non-start UNIT placements,
including 35 mineral fields and three geysers. The source parser classifies 105 UNIT rows as
units and 73 as buildings; runtime mapping moves raw type 195 from the source
UNIT category into the building pool, yielding 104 ordinary units and 74
buildings. The storage probe keeps pools of 160 units, 80 buildings, and 48
minerals. Packed building rows reduce the save record to 15,384 bytes and use
save generation 15, separate from T5 generation 14.

To repeat the ARM persistence check through ordinary campaign controls:

```sh
python3 tools/sc64_t5_save_smoke.py \
  --rom build/StarCraft-T6-Storage-Probe.gba \
  --elf build/starcraft-t6-storage-probe.elf \
  --menu-entry --generation 15 --output .local/sc64/t6/storage-arm-save
```

The frozen ARM storage probe passes the existing memory guards: text/data/BSS
are 2,474,724/388/245,632 bytes, IWRAM ends at `0x03004f90`, and BSS ends at
`0x0203c108`. A fresh-process ordinary-menu T5 save test on that build saves at
frame 123, reloads at frame 123, and continues to frame 183 with all eight
source-player contexts fault-free and campaign/AI clocks aligned. This proves
the new save layout on the existing T5 scenario only. At that stage, T6
mission-world integration, controller completion, and performance
qualification remained future work; the campaign count was 11 of 30.

The standalone T6 trigger adapter has separate host checks:

```sh
make test-sc64-t6-runtime-synthetic  # Asset-free typed callback/core checks
make test-sc64-t6-runtime            # Requires the prepared private T6 records
```

It reuses the bounded T4 runner and retains all eight source-player contexts,
including inactive controller slots enabled by source Force masks. Its 1,184
bytes hold the contexts and shared switches. World queries, creation,
resources, AI requests and presentation use typed callbacks; they are not yet
bound to the playable GBA world. Waits use `ceil(milliseconds * 30 / 1000)`
ticks and elapsed conditions use 30 ticks per second. These are GBA adapter
policies. The checked entry validates the program and state; a separate
prevalidated entry avoids rescanning immutable tables each tick after
init/load validation. The structural state validator is not a save execution
provenance validator, and this stage does not change any ROM or SRAM format.

## Internal Terran mission and capacity probes

Norad II has a separate live-world probe, still unavailable in the normal
campaign menu until a controller completion run is qualified:

```sh
make sc64-t6-prepare sc64-t6-engine-probe
make test-sc64-t6-engine test-sc64-t6-world test-sc64-t6-save test-sc64-t6-tech
make test-sc64-t6-repair
```

The probe imports the entire starting roster, executes the source trigger
contexts, and supports paid Starport/Control Tower/Dropship construction.
Select SCVs, then press A over a damaged friendly completed Terran structure
to repair it. Repair spends resources and uses the reference construction HP
rate at 24 source steps per 30 GBA ticks; mobile mechanical repair is pending.
Original AI script bodies are still replaced by bounded authored behavior.

The late-campaign storage probe tests a distinct generation-17 layout:

```sh
make sc64-terran-late-storage test-sc64-late-terran-storage
make test-sc64-compact-unit-save test-sc64-terran-late-actors
make test-sc64-terran-late-runtime
python3 tools/sc64_t5_save_smoke.py --mission 6 --generation 17 \
  --rom build/StarCraft-Terran-Late-Storage.gba \
  --elf build/starcraft-terran-late-storage.elf \
  --output .local/sc64/terran-late/arm-save --ticks 300
```

It holds 192 units, 96 buildings and 80 mineral fields. Its lossless 38-byte
unit save rows and 56-byte runtime units preserve all named fields, including
dead slots; the payload is 15,312 bytes. Combined target IDs use 16 bits and
visibility has exact nine-bit overlap counts. Keep its SRAM separate from
earlier profiles. These commands exercise storage and the internal Norad II
probe; they do not qualify Terran missions 6–10 or establish performance.

## Troubleshooting

- If `doctor` reports a missing required tool, install that tool and confirm its command is on `PATH`. ARM GCC also needs the matching `arm-none-eabi-objcopy` and `arm-none-eabi-size` commands. On a custom toolchain prefix, set `PREFIX` to the prefix used by all three commands.
- Cloning the public repository over HTTPS needs no GitHub login. If cloning fails, check the repository URL and network access. Pushing changes requires write access and authentication; for HTTPS, use `gh auth login` and `gh auth setup-git`, or configure a GitHub SSH key for SSH.
- If `./dev sc64 prepare` rejects the ROM, make sure it is the supported USA image and that a ZIP contains exactly one 32 MiB N64 image. The normalized SHA-1 must match the value above. The project cannot supply or identify a ROM for you.
- If an optional profile build says generated headers are missing, prepare that profile first with `./dev sc64 prepare --rom PATH --profile PROFILE`. These generated inputs are private local data and are not checked into the repository.
- If `./dev play` cannot find mGBA, install it or set `MGBA` to the executable path. For example, on Ubuntu: `MGBA=/usr/games/mgba ./dev play starcraft`.

## References and scope

Reference revisions, files consulted, and reduced-mechanics notes are in [REFERENCES.md](REFERENCES.md). License status and provenance are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Build and test coverage is summarized in [VALIDATION.md](VALIDATION.md). The engine uses bounded, fixed-size data for GBA memory. Campaign triggers, AI, unit systems, and presentation remain intentionally incomplete.

### T6 ordinary-menu qualification build

With the private T6 inputs prepared, `make sc64-terran-t6-controller` builds an
explicit six-mission development cartridge. `make
build/test-sc64-t6-campaign-run` builds its host controller runner. The ignored
trace can be replayed with `tools/sc64_terran_controller_replay.py`, supplying
that ROM and ELF through `--rom` and `--elf`; `--validate-only` checks the trace
without starting mGBA. The ordinary T6 menu route is A, RIGHT, RIGHT, DOWN, A,
A. The source-only repository deliberately excludes controller traces that
encode private map data, generated headers, ROMs, and emulator output.

The late Terran runtime adds LIFT / LAND to Terran field commands. Select a
completed liftable building, choose LIFT / LAND to lift off, use MOVE ORDER to
fly it, then choose LIFT / LAND and a clear destination to land. Airborne
buildings cannot produce units or receive worker resource returns. Accepted
landing goals are reserved against overlap and persist in generation-18
saves. This support is required by The Big Push's imported airborne opening;
that earlier stage note predates the completed host-controller runs recorded
below.

## Combined Terran T1–T10 campaign

The canonical Terran profile integrates all ten campaign missions and the
existing Outpost/Blood Bath skirmish setup in one ROM.
Controller-driven host runs and normal-menu ARM replays have completed all ten
missions across the recorded frozen builds. T7, T8 and T10 have canonical-ROM
ARM wins; T9's ARM win is on the earlier frozen ROM listed below. On the
canonical ROM, T7 wins at frame 12,151 (15.76 effective fps), T8 at frame
86,596 (14.37 fps), and T10 at frame 38,491 (11.78 fps). T10 ends with Raynor
alive, the Ion Cannon destroyed and zero trigger faults. These emulator rates
are below the 30-fps target; physical-GBA qualification is not claimed.
All six fresh-process generation-18 saves for missions T5–T10 also pass on
the canonical ROM: each loads exactly at frame 123, continues to frame 183
with zero faults, and retains the mission-specific late state exactly for
T7–T10.

Prepare the Terran inputs, then build, test and play the normal-menu ROM:

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile terran
./dev sc64 build --profile terran
./dev sc64 test --profile terran
./dev sc64 play --profile terran
```

This produces `build/StarCraft-Terran-GBA.gba`, an 8 MiB ROM. To build all
three race campaigns, first prepare the shared inputs with the Terran profile,
then run `make race-campaigns test-race-campaigns`. Preparing Zerg or Protoss
alone is not sufficient for that aggregate target. The original 32 MiB N64
image, imported content and generated headers remain under ignored local
storage and must not be committed or distributed.

The integrated profile uses 192 unit slots, 96 building slots and 80 mineral
fields, with 128×128 terrain support for the largest retained maps. Its save
generation is 18, so keep its SRAM separate from earlier campaign builds.
The map-select skirmish still supports any human race and one to three
opponents whose races are chosen independently. Current adaptations include
bounded trigger execution, authored AI fallbacks and simplified presentation;
they do not reproduce the original N64 engine or every original behavior.

The canonical ROM SHA-256 is
`00f0939878a51320b8fc252d69b2947ffc442e6d3cc39efa12d0d118511bf3ee`.
T9's ARM replay used earlier frozen ROM
`329793a4707e0d3c4b33ed169391b2b887374d5eb788943ceef8828604c7ce44`; it
wins at frame 358,906 with all 43 Protoss Force2 buildings destroyed, all 21
Zerg Force3 buildings intact, Kerrigan alive, all eight trigger clocks at frame 358,906,
zero faults, and 16.94 effective fps. T6's separately qualified ARM replay
averaged 17.26 fps. All reported campaign rates are below target; physical
GBA testing remains pending.

### Repeatable Terran performance comparison

For a repeatable T10 sample, prepare and build the Terran profile, then run
`./dev sc64 test --profile terran`. This runs the private controller profile
and writes its T10 trace to the ignored local path
`.local/sc64/terran-late/full-t10-controller.json`. Replay that trace through
the normal menu using a frozen ROM and its matching ELF for each build:

```sh
python3 tools/sc64_terran_controller_replay.py \
  --trace .local/sc64/terran-late/full-t10-controller.json \
  --rom /path/to/frozen-terran.gba --elf /path/to/matching-terran.elf \
  --ticks 4000
```

This bounded prefix reports progress and performance without claiming a win.
Omit `--ticks` to run the complete trace through the mission win. Keep
immutable, matching ROM/ELF copies for the before and after builds, and record
their SHA-256 hashes, compiler/toolchain versions, replay command, and observed
mission state with each result. Compare runs only when they use the same trace,
emulator setup, and observed state/frame range, with a matching ROM/ELF pair
for each build. The replay's
effective fps uses the emulated GBA hardware timer and completed simulation
frames, not host wall time. Diagnostic counters describe only the final tick;
collect them from a diagnostic build separately from the uninstrumented FPS
comparison. `tools/sc64_compare_campaign_replays.py` is Protoss-only and does not
apply to Terran measurements.

### Zerg 4 source staging

The opt-in Z4 importer prepares the complete source for **Agent of the Swarm**
without changing the canonical race pack or enabling a playable mission. Use
the verified USA ROM and the existing pinned `references/sc64-maps` checkout:

```sh
python3 tools/sc64_z4_pack.py --rom /path/to/StarCraft64-USA.z64 \
  --out .local/sc64/z4
python3 tools/sc64_z4_pack.py --verify-staged --out .local/sc64/z4
python3 tests/test_sc64_z4_import.py
```

Packing verifies the normalized ROM, extractor revision and exact extractor
source bytes, and the authenticated Z4 CHK. The ignored output contains
`source.chk`, 16 framed source records, `manifest.json`, and `source-audit.json`.
Standalone verification checks retained files against the pinned CHK without
requiring the original ROM or running the extractor. Keep all output private.

The audit preserves all 184 placement rows, seven trigger records, source
properties, ownership, locations and other CHK sections. It reports runtime
mapping gaps and pool requirements rather than truncating source rows. Z4
still needs gameplay mappings, larger building/mineral pools, its countdown
and hatch sequence, victory/defeat behavior, persistence and controller/ARM
qualification. Source staging alone does not establish a playable mission.

### Canonical Zerg and Protoss builds

The standard Zerg build includes its first six missions; the standard
Protoss build includes its first five. Prepare each profile from your verified ROM, then use the standard
commands; no extra staged-profile selection is needed:

```sh
./dev sc64 prepare --rom /path/to/StarCraft64-USA.z64 --profile zerg
./dev sc64 build --profile zerg
./dev sc64 test --profile zerg
./dev sc64 play --profile zerg
```

Replace `zerg` with `protoss` for the Protoss campaign. The canonical output
pairs are `build/StarCraft-Zerg-Z6-GBA.gba` / `build/starcraft-zerg-z6.elf`
and `build/StarCraft-Protoss-P6-GBA.gba` / `build/starcraft-protoss-p6.elf`.
Explicit `zerg-z4` (`--profile zerg-z4`) preserves the earlier four-entry
`build/StarCraft-Zerg-Z4-GBA.gba` / `build/starcraft-zerg-z4.elf` pair, its
`SCZ4` generation-20 saves and `make test-zerg-z4`. Explicit `zerg-z3`
preserves `build/StarCraft-Zerg-GBA.gba` / `build/starcraft-zerg.elf` and its
save identity; versioned `-Z3` names are aliases. `protoss-p3` preserves the
legacy P3 real files, code and save path. Retain matching
frozen ROM/ELF pairs for replay.

The canonical Zerg test target runs ordinary-input controllers for all six
chapters under the canonical flags, normal-menu startup/save fixtures for all
six campaign entries, and all 234 combinations
of map, human race and independently selected AI races. The setup/save matrix
does not prove a completed match for every combination. Build guards still
enforce the same IWRAM and EWRAM reserves.

Canonical Zerg uses `SCZ6`, save generation 32, the Z6 storage/runtime and
query flags, a word-packed six-chapter sprite bank under
`.local/sc64/zerg-z6/generated`, and the canonical drawing flags. Preparing
`zerg` also generates the Z5/Z6 inputs and that bank. Its save directory,
`.local/play/zerg-z6`, is separate from the legacy Z4 and Z3 profiles. Canonical Protoss uses `SCP6`, generation 30 and
`.local/play/protoss-p6`; it carries P1–P5 including Shuttle transport, then P6
with Storm and the source rescue/extraction mission. Its separate packed sprite
bank is `.local/sc64/protoss-p6/generated`; the internal probe bank is unchanged.
Explicit `protoss-p5-shuttle` retains `SCPB`, generation 26 and
`.local/play/protoss-p5-shuttle`.
Explicit `protoss-p4-storm` retains `SCP5`, generation 21 and its P4 save path;
`protoss-p3` retains `SCP3`, generation 7. Older standard two-mission generation-2
saves are incompatible and are rejected. The asset-free default build
continues to require no imported inputs.

Older prepared Protoss workspaces must prepare P6 inputs before building:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 USA ROM" --profile protoss
./dev sc64 build --profile protoss
./dev sc64 test --profile protoss
```

P6 preparation requires authenticated P4/P5/P6 map/scenario gates, P6 abilities
and doodads, and its complete packed sprite bank. `protoss-p6` is an explicit
alias for the canonical product. `protoss-p5-shuttle` preserves the earlier
five-entry ROM and its separate saves. No P3/P4/P5 save conversion is performed.
The P6 runtime retains generation 30; its ROM title/code are `SC PROTOSS6` /
`SCP6`. See the [P6 audit](audits/2026-10-27-p6-promotion/README.md) for current
host, ARM, save, timing and memory evidence. P5's earlier evidence remains in
the [P5 audit](audits/2026-10-26-p5-promotion/README.md).

### Staged Zerg and Protoss chapter 4

Zerg 4 is enabled in the canonical profile, which now continues through Zerg 6.
The explicit `zerg-z4` profile builds the earlier four-entry Z4 ROM. Canonical Protoss includes **The Hunt for Tassadar** and
P5 through the Shuttle runtime and P6. Explicit `protoss-p4-storm` keeps the
four-entry Storm profile and its own real files/save format. The plain `protoss-p4`
build remains a separate internal experiment. Preparing later map data does
not enable unqualified later missions in the frontend.

The wrapper can prepare, build, test and launch the Storm-enabled P4 profile
without running the individual packers manually:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 USA ROM" --profile protoss-p4-storm
./dev sc64 build --profile protoss-p4-storm
./dev sc64 test --profile protoss-p4-storm
./dev sc64 play --profile protoss-p4-storm
```

This route checks both P3/P4 map gates, source mission identity and packed
sprite metadata. It uses `.local/play/protoss-p4-storm` for emulator saves.
The explicit `protoss-p3` wrapper retains the older P3 ROM and
`.local/play/protoss` saves. Their generations and game codes are distinct;
the wrapper does not convert saves between them. See the
[P4 audit](audits/2026-10-26-p4-promotion/README.md) for current-source ARM
mission and live-save qualification, separate from preparation.

P4 Storm, preserved P5 and canonical P6 enable the generated P4 create-action prefix and hot validator.
An older prepared workspace must regenerate its numeric scenario header before
building:

```sh
python3 tools/sc64_p4_scenario_pack.py
make protoss test-protoss
```

The generator uses the already authenticated local P4 source; it does not fetch
assets. Full `./dev sc64 prepare --profile protoss` also prepares this version.
For an explicit comparison build, `make -B protoss P4_CREATE_PREFIX_FLAGS=`
retains the old action-row recount. Force rebuilding is necessary when changing
flags. These flags do not change save generation, actor limits or the legacy P3
build. See the [prefix audit](audits/2026-10-26-p4-create-prefix/README.md) for
the measured scope and unchanged worst-frame limitation.

After the canonical profile preparation has supplied the common assets,
prepare the additional ignored Z4 and P4 inputs from the same verified source
ROM (`./dev sc64 prepare --profile zerg` prepares Z4 automatically):

```sh
python3 tools/sc64_races_pack.py --rom /path/to/StarCraft64-USA.z64 --out .local/sc64/zp-stage/generated/sc64_races_generated.h
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --base-only --include-zp-campaign-actors --prepack-unit-words --out .local/sc64/zp-stage/generated/sprites --header .local/sc64/zp-stage/generated/sc64_race_sprites_generated.h
python3 tools/sc64_z4_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_z4_scenario_pack.py
python3 tools/sc64_p4_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_p4_scenario_pack.py
make zerg-z4 protoss-p4
make test-sc64-z4-runtime test-sc64-p4-runtime test-sc64-zp-storage
```

Outputs are `build/StarCraft-Zerg-Z4-GBA.gba` and
`build/StarCraft-Protoss-P4-GBA.gba`, with matching
`build/starcraft-zerg-z4.elf` and `build/starcraft-protoss-p4.elf` files.
Use the matching pairs for emulator qualification. All imported data, ROMs,
saves and graphical captures must remain local and ignored.

The Z4 layout uses 192 Zerg unit slots, 96 buildings, 48 mineral fields, four
geysers, compact save rows and 128×128 geometry. Its generation-20 saves are
incompatible with the legacy Z3 generation-5 saves. The P4 layout uses 240
Protoss unit slots and its own generation-20 saves. Protoss production reserves
the remaining 87 source-script creations, including
queued products, instead of allowing those later actors to be dropped.
These bounds still do not fit every later campaign map; see
[the remaining-chapter audit](ZP-CAMPAIGN-IMPLEMENTATION.md).

The runtime tests use real engine/menu/update/save paths under sanitizers.
Their controlled actor deaths, position changes and protected worlds are
synthetic predicate tests. They do not establish a normal controller win or
hardware frame rate. The imported opaque AI routines use bounded authored
pressure policies; text/audio actions use documented presentation holds.
Original N64 AI, animation, abilities and trigger scheduling are not fully
reproduced. Controller and ARM qualification remain separate requirements.

The staged Zerg field menu adds **Upgrade to Lair** for a selected, idle
Hatchery and **Spire** for a selected Drone. Select the completed Hatchery or
Lair and press **R** to cycle to **Mutalisk**, then **A** to train. The Lair
requires a Spawning Pool; the Spire requires a completed Lair; Mutalisks
require a completed Spire. This makes the source-permitted air route
accessible through ordinary controls. The Lair upgrade uses the existing
construction policy: its larva timer pauses during the upgrade and completion
fills its larva count to three. Native equivalence of that timing has not been
established.

An additional P4 ability experiment is isolated from the normal P4 build:

```sh
python3 tools/sc64_p4_scenario_pack.py
make protoss-p4-storm test-sc64-p4-storm
```

This writes `build/StarCraft-Protoss-P4-Storm-GBA.gba` and its matching
`build/starcraft-protoss-p4-storm.elf`. Its generation-21 save persists four
bounded Psionic Storm effects and caster energy. In P4, select one High
Templar or Tassadar, open **B** commands, choose **STORM**, place the cursor
within cast range and press **A**; **B** cancels targeting. The row appears
only when the selected caster has enough energy. Storm affects allied units
too. Its pulse cadence and visual effect use documented authored policies;
see the frozen development-image qualification in [Validation](VALIDATION.md).

The P4 controller now has a repeatable host win. Run its full input-only
playthrough with `make test-sc64-p4-campaign`; matching-image ARM replay and
fresh-process save checks are separate. In this profile, **B+R** selects the combat escort
without Tassadar and Raynor, so army orders can clear their route first.
Ordinary **R** still selects the heroes with the army.

For later-chapter implementation, authenticate and generate the retained
numeric scenario metadata separately. For example:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission Z5 --verify-staged
python3 tools/sc64_zp_scenario_pack.py --mission Z5
```

The tool accepts Z5–Z10 and P5–P10 and reads the staged source-map directory
created by `sc64_races_pack.py`. Its default output is an ignored header under
`.local/sc64/zp-stage/generated/scenarios/`. It preserves numeric trigger,
location, player, property and availability records after authenticating the
complete CHK and section hashes. Generation does not implement or enable a
mission; actor support, runtime actions, save rules and controller
qualification remain required.

The isolated Z5/P5 schedulers have asset-free checks:

```sh
make test-sc64-zp-chapter-five-models
```

After generating both numeric headers, run their adapter checks with
`make test-sc64-z5-adapter test-sc64-p5-adapter`. The adapters preserve source
operands, sequence waits and switches, and reject failed world callbacks.
They still require actual engine implementations of source conditions and
effects; these fixtures do not establish playable fifth missions. Neither
model or adapter enables a frontend entry.

### Staged Zerg chapter 5 and expanded storage

After preparing the chapter-four inputs above, generate the additional Z5
numeric records and isolated sprite bank:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission Z5
python3 tools/sc64_z5_doodad_pack.py
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --base-only --include-zp-campaign-actors --include-z5-actors --include-z5-doodads --out .local/sc64/z5/generated/sprites --header .local/sc64/z5/generated/sc64_race_sprites_generated.h
make zerg-z5 test-sc64-z5-runtime test-sc64-z5-world
```

`build/StarCraft-Zerg-Z5-GBA.gba` and `build/starcraft-zerg-z5.elf` are an
internal five-entry Zerg build. Its complete source placements include all
41 unit-backed doors and traps; the other 30 THG2 rows are sprite-only
decoration and have no world actor. The runtime tests cover import, trigger
effects, door collision/cache changes, save/load and malformed-state
rejection. They do not prove a controller win. The source PauseGame action
requires an authored **A** acknowledgment; **B** saves while that pause is
shown. Native speech, cinematic and AI-script behavior remain approximations.

This profile uses generation 23 and an opt-in full-storage layout: 240 units,
160 buildings, 144 mineral fields and 12 geysers. Compact 48-byte runtime
buildings, 35-byte unit save rows and 5-byte mineral save rows keep the
unchanged SRAM and RAM guards. Older save generations are incompatible.
These are fixed pools, not a guarantee that unlimited repeated triggers or
production in every later mission can fit. `make sc64-zp-full-storage-probe
test-sc64-zp-full-storage` separately checks the storage layout with P4.

### Optional Z5 cloak and staged Protoss chapter 5

Z5's optional ability profile authenticates PTEC and the hero's UNIT energy
before enabling Personnel Cloaking. With the Z5 inputs above prepared, run:

```sh
python3 tools/sc64_z5_ability_pack.py --verify-staged
python3 tools/sc64_z5_ability_pack.py
make zerg-z5-cloak test-sc64-z5-cloak-runtime
```

Select Kerrigan, press **B**, and choose **CLOAK** or **UNCLOAK** in the command
list. Her HUD shows energy and cloak state. STOP preserves energy. Live,
enabled hostile detector traps reveal her within their source bounding-box
sight range; tying detector activity to the authored trap-enabled state is a
GBA policy, not verified native doodad-order behavior. Source PTEC disables
Psionic Storm here. This profile uses generation 25 and rejects older saves;
it does not establish a completed Z5 playthrough.

P5 has a separate five-entry Protoss build. After the chapter-four staging
and P4 inputs are ready, prepare its numeric descriptor and sprite bank:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission P5
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --base-only --include-zp-campaign-actors --include-z6-actors --out .local/sc64/p5/generated/sprites --header .local/sc64/p5/generated/sc64_race_sprites_generated.h
make protoss-p5 test-sc64-p5-runtime test-sc64-p5-world
```

`build/StarCraft-Protoss-P5-GBA.gba` uses save generation 24. Choosing Sides
retains every starting placement, separate hostile forces, source-player
death counters, rescue, and all 11 source creates. Production reserves slots
for unexecuted creates. The reinforcements wait for a source event: entering
one of the choice regions or killing a source-player-6 actor sets switch 0.
Bring Tassadar and two Zealots to the extraction point to satisfy the source
victory predicates. The three opaque AI script actions use a bounded authored
paid-pressure policy; native AI equivalence is unverified. Startup, source
actions, saves and render checks do not establish a normal-controller win.
The extraction terrain requires Shuttle transport, which this generation-24
profile does not enable. To test the separate generation-26 cargo profile:

```sh
make protoss-p5-shuttle test-sc64-p5-shuttle-runtime
```

Select passengers and press **A** on the friendly Shuttle to board. It has
eight space points: Probe 1, Zealot/High Templar/Tassadar 2, Dragoon 4. Select
the loaded Shuttle, press **B**, choose **UNLOAD CARGO**, then press **A** on
a clear landing area. Passengers retain HP and supply while aboard and
cannot be selected, attacked or counted in a Bring objective until unloaded.
Cargo dies if the Shuttle is destroyed. These bounded GBA orders reuse
existing unit/save fields; generation-24 saves are incompatible. Lifecycle
checks cover the transport lifecycle. A complete normal-controller P5 win, ARM replay,
and fresh-process cargo save are now qualified (see `docs/VALIDATION.md`).
Run `make test-sc64-p5-campaign-run` to repeat the host win with sanitizers.

`make test-sc64-z5-ability-models test-sc64-z6-model` requires no imported data.
After generating the Z6 numeric descriptor with the scenario packer,
`make test-sc64-z6-adapter` checks its isolated callback scheduler. Z6 is not
an enabled runtime mission.

`make test-sc64-z6-world` checks its engine-backed world callback foundation,
including selectors, locations, resources, both MoveLocation targets,
KillUnitAtLocation and source properties. Its mutable location state still
needs save integration. Source hallucination, energy and opaque AI actions
require explicit integration callbacks and fail closed when absent; this
fixture does not establish a playable Z6 mission.

### Z5 Consume and remaining ability models

The optional Z5 Consume profile shares Kerrigan's source-checked Q8 energy and
cloak state. Prepare the existing Z5 inputs, regenerate the authenticated
ability facts, then build and test:

```sh
python3 tools/sc64_z5_ability_pack.py
make zerg-z5-consume test-sc64-z5-consume-runtime
```

Select Kerrigan, press **B**, choose **CONSUME** after the cloak command, then
press **A** on a friendly Zergling. She approaches using ordinary movement and
consumes it for 50 energy, capped at 250, with no energy cost. Hallucinated food
is killed without giving energy. This adapter supports the authenticated Z5
human target roster (source-owner-3 raw37 Zerglings); the standalone model
records the broader native target filters without inventing an organic-only
restriction. Native spell animation, projectile delay and inertia are not
emulated. The range uses native collision-box geometry at the GBA's 2:1 scale
with zero movement allowance.

`build/StarCraft-Zerg-Z5-Consume-GBA.gba` has distinct identity SCZD and save
generation 27, rejecting earlier generations. Pending Consume orders preserve
validated caster/target identities through SRAM; invalid links reject before
world mutation or SRAM replacement. The unused circle fallback is kept in ROM
for this profile so the unchanged IWRAM stack reserve remains intact.

The source audit also authenticates Ensnare permission. Its standalone model
covers friendly-fire area filters, energy cost and the 75-status-decrement
lifetime (600 native frames, nominally 750 GBA updates). Ensnare is **not yet
integrated into the playable runtime**. Ability fixtures and startup/save
coverage do not establish a complete Z5 controller win.

### P6 numeric foundation

P6 has isolated source-program and actor-fact checks:

```sh
make test-sc64-p6-model test-sc64-p6-actors
python3 tools/sc64_zp_scenario_pack.py --mission P6
make test-sc64-p6-adapter test-sc64-p6-world
```

The numeric adapter validates 26 one-shot records, 99 actions, 29 conditions,
source selectors, waits, switches and the referenced UPRP identities. The isolated world adapter tests all source rows with synthetic callbacks. World
operations still require live engine bindings and explicit capabilities; the opaque AI row fails closed
without an authored policy. P6 is not yet a selectable playable mission. Its
remaining runtime work includes property-driven enemy orders, permanent-cloak
detection for Dark Templar/Zeratul, Infested Terran suicide damage, THG2 props,
and full normal-input/ARM/save qualification. Source door types 205–208 are
invincible non-buildings; placement in the GBA Building pool is a prop policy.


The opt-in P6 sprite importer prepares the missing actor and wall-trap art
without changing earlier sprite banks:

```sh
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --include-p6-actors --include-z5-actors --include-z6-actors --out .local/sc64/p6/generated/sprites --header .local/sc64/p6/generated/sc64_race_sprites_generated.h
python3 tools/sc64_p6_doodad_pack.py --verify-staged
```

Keep T4 imports enabled (omit `--base-only`) for the inherited props. The P6
bundle reuses ZP beacons and Z5 indexed doors/traps and adds raw50/74/75 unit
images plus all seven raw210 wall-missile-trap frames. This prepares art only;
new raw actor behavior and the raw210 prop policy remain unimplemented. The
THG2 audit preserves all 37 rows: 27 unit-backed props and ten sprite-only
rows. The sprite-only type IDs are SpriteType IDs, not unit IDs. The two raw210
unit-backed placements explicitly prevent runtime-ready prop generation until
their mapping is implemented. `test-sc64-p6-actor-source` audits source UPRP
values separately from the asset-free actor-fact checks.


### Z5 complete controller and shared query checks

The generation-27 Consume build now has a full ordinary-input host win, a
complete ARM replay, and an exact fresh-process mid-mission save restore.
Repeat the host run with `make test-sc64-z5-campaign-run`. It launches through
HOME, retains Kerrigan and uses movement, cloak and Consume through the normal
controls; victory follows the source gate and extraction program.

`make test-sc64-z5-count-queries` checks the optional started-trigger pruning
and phase-local Command/Bring snapshot against their full-query oracles. The
three preserved source triggers continue to poll; copied or unsupported rows
fall back to full queries. Neither optimization changes SRAM. See Validation
for frozen replay comparisons and measured performance.

### P6 internal live probe

After preparing the P6 sprite and numeric THG2 inputs above, run:

```sh
make protoss-p6-probe
make test-sc64-p6-game test-sc64-p6-runtime test-sc64-p6-save
make test-sc64-p6-combat test-sc64-p6-profile test-sc64-p6-frame-coverage
```

`build/StarCraft-Protoss-P6-Probe-GBA.gba` has an internal sixth menu entry and
separate generation-30 identity SCPC. It imports 200 units, four beacons and
27 unit-backed THG2 props. The source 26-record program drives gates, creates
and the Zeratul extraction objective. The bounded GBA actor policy includes
permanent-cloak detection, property-driven burrow/release, Infested Terran
suicide splash and raw210 trap attacks. Ten sprite-only decorations remain
metadata rather than rendered actors. The opaque Suic script is an authored
single AttackMove for source Force 2; native AI, animation timing and full
combat equivalence are not claimed.

The probe passes live startup, candidate-only save validation, combat and
framebuffer checks, including source door changes. These checks do not yet
establish a complete controller win, ARM mission replay or hardware viability.
The standard Protoss output remains the separately qualified P1–P3 build.


The Consume build enables the qualified Z5 query options by default. For a
comparison build, use `make -B zerg-z5-consume Z5_QUERY_FLAGS=` and retain that
ROM/ELF separately, then restore with `make -B zerg-z5-consume`. The complete
optimized replay averages 17.73 effective fps versus 8.78 for the baseline,
with matching observed state/captures and exact fresh-process save restore.


### Z6 live probe and seventh-chapter foundations

After the staged map and Z5 inputs are prepared, import Z6's complete optional
actor/building bank and build the internal sixth-entry ROM:

```sh
python3 tools/sc64_race_sprites.py --rom "/path/to/StarCraft 64.z64" --base-only --include-zp-campaign-actors --include-z5-actors --include-z5-doodads --include-z6-actors --include-z6-buildings --out .local/sc64/z6/generated/sprites --header .local/sc64/z6/generated/sc64_race_sprites_generated.h
python3 tools/sc64_zp_scenario_pack.py --mission Z6
make zerg-z6-probe
make test-sc64-z6-world test-sc64-z6-runtime test-sc64-z6-save
make test-sc64-z6-profile test-sc64-z6-frame-coverage
```

This produces `build/StarCraft-Zerg-Z6-Probe-GBA.gba` (SCZE, generation 32).
It retains the complete 185-row source placement, adds distinct Robotics
Support Bay and Shield Battery identities, and runs Z6's source trigger
program. Hallucinated Tassadar actors have a bounded lifetime, cannot deal
damage, and take doubled damage before armor and shields. The Zer6 script
uses an explicit paid-production/order policy rather than native AI bytecode.
Kerrigan now starts at the authored full energy and has Cloak and Storm in
her field menu. Her source disables Consume. Ensnare and active Shield
Battery recharge remain incomplete. Do not count this as a completed sixth
mission.

Z7 and P7 source schedulers can be checked independently:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission Z7
python3 tools/sc64_zp_scenario_pack.py --mission P7
make test-sc64-z7-model test-sc64-z7-adapter test-sc64-z7-runtime
make test-sc64-p7-model test-sc64-p7-adapter
```

Z7's world/runtime fixture uses all 231 canonical rows and a bounded paid
Zerg AI policy. Its opt-in shared-engine profile now includes a seventh menu
entry, source objective outcomes and generation-35 saves:

```sh
make zerg-z7-probe
make test-sc64-z7-profile test-sc64-z7-game-save test-sc64-z7-frame-coverage test-sc64-z7-colonies
```

This produces `build/StarCraft-Zerg-Z7-Probe-GBA.gba` (SCZF). It requires the
complete Z6 inputs above and the Z7 scenario descriptor. It retains the source
starting roster and resources; build a base and destroy the hostile source
owner's buildings while preserving at least one human actor. Menu/startup,
save rejection and render checks establish integration readiness, not a
completed playthrough or full-mission performance result. Its save generation
differs from the Z6 probe and earlier generation-33 Z7 probe; older saves are
rejected rather than converted. With only Drones selected, scroll below SPIRE
to CREEP COLONY and place a 75-mineral colony on creep. After it completes,
select it and choose SUNKEN from the contextual row. A completed Spawning
Pool and 50 minerals are required; this morph consumes no additional Drone.
It preserves damage through the 375-tick morph and applies the reference's
max-HP adjustment at completion. The explicit colony-defense gate is enabled
only in the Z7 development profile.

The optional full-render candidate can be built and host-checked with:

```sh
make zerg-z7-full-render-probe
make test-sc64-z7-full-render
```

It produces a generation-36 SCZR title/profile candidate. The full-render
audit records are in [the Z7 full-render audit](audits/2026-10-27-z7-full-render/README.md).
The fresh-process ARM restart confirmed byte-identical 16,296-byte SRAM
payloads with six worker depot caches retained after reload. The paired
12,000-tick ARM prefix had 34 zero-mismatch projection checks; it does not
compare the full world or pixels or complete the mission. Mean full-frame
cost was 1,069,633 cycles versus 1,026,851 for the reference (+4.17%). These
are emulator measurements, not hardware or 30-fps qualifications. The Z6
canonical profile remains unchanged, and staged campaign qualification stays
at 22/30.

`make test-sc64-z7-reachable-formation` checks the default-off
`SC64_REACHABLE_FORMATION` group-order fallback against the original slot
policy. Canonical flags leave it disabled. See the
[formation audit](audits/2026-10-27-reachable-formation/README.md) for the
synthetic input fixture, ARM link evidence and remaining qualification gates.
The newer [group command cost audit](audits/2026-10-27-group-command-cost/README.md)
records the unconditional selection-count hoist and the component-based
reachability implementation. The optional target also checks retained route
hints against an independent graph oracle. The formation feature remains off
in canonical builds; the selection-count hoist applies to all builds.

The separate default-off `SC64_Z7_PRUNE_STARTED` experiment skips census work
for authenticated one-shot triggers that have already started, while retaining
per-tick live-world validation and pending action clocks. With the existing
private Z7 inputs, `make test-sc64-z7-prune-started` runs paired ASan/UBSan
fixtures. See the [started-trigger pruning audit](audits/2026-10-27-z7-started-pruning/README.md)
for the opening and real-save ARM digest comparisons and measured limits.
It does not promote Z7 or enable the option in canonical build flags.

`make zerg-z7-steering-probe` builds a separate full-render generation-36
`SCZS` development ROM with the collision-class cache and bounded local
steering list enabled. `make test-sc64-z7-steering` checks list answers against
the original step-clear query, capacity/worker fallback, slot reuse and SRAM
cache rebuilding under ASan/UBSan. The Z7 list uses a 388-byte caller-owned
stack context and a ROM builder to fit the unchanged product guards. See the
[Z7 steering audit](audits/2026-10-27-z7-steering-list/README.md) for the actual
standalone link, paired ARM digests, stack canary and cycle measurements.
Canonical race flags and the earlier full-render probe stay unchanged.
The [Z7 raw-density preflight audit](audits/2026-10-27-z7-steering-preflight/README.md)
records a later exact-fallback optimization within that same optional steering
profile, its boundary tests and measured continuation costs.

P7's separate seven-mission composition has host completion and bounded ARM
completion/save evidence, while native AI, Carrier combat, and performance
qualification remain unfinished; see [the P7 controller report](P7-CONTROLLER.md).
The indexed Z7 controller has a [reproduced host win](audits/2026-10-27-z7-controller-win/README.md)
via `make test-sc64-z7-controller`. A separate full-render ARM replay now
[matches all 365 checkpoints through victory](audits/2026-10-27-z7-full-win/README.md),
but its instrumented mean is about 2.85 rendered updates/second. Performance,
carried-mission regression checks, and fresh-process ARM SRAM continuation
remain required before campaign promotion.


The sixth-mission probes require their authenticated spell metadata:

```sh
python3 tools/sc64_p6_ability_pack.py
python3 tools/sc64_z6_ability_pack.py
make test-sc64-campaign-spells test-sc64-p6-storm
make test-sc64-six-query-options test-sc64-p7-runtime
```

P6's Tassadar has Storm at `B` → `STORM`; select him alone before casting.
Z6's Kerrigan has `CLOAK`/`UNCLOAK` and `STORM` in the field menu. Stop retains
spell energy. Active effects and energy survive SRAM restore; the new probes
reject their older generation-28/29 saves. P6 uses generation 30 and Z6 uses
32, while the separately qualified P5/Z5 outputs keep their existing formats.

Optional query experiments remain off by default. Compare a frozen baseline
with `make -B protoss-p6-probe P6_QUERY_FLAGS='-DSC64_P6_PRUNE_STARTED -DSC64_P6_PREVALIDATED_TICK'`.
The analogous Z6 experiment uses `Z6_QUERY_FLAGS=-DSC64_Z6_PRUNE_STARTED`.
An additional default-off Z6 count experiment is available after preparing
the private numeric scenario inputs:

```sh
make test-sc64-z6-count-queries
make -B zerg-z6-probe Z6_QUERY_FLAGS=-DSC64_Z6_COUNT_SNAPSHOT
```

It caches the eight authenticated Command/Bring counts for one condition
phase, with full-scan fallback and invalidation before actions. It retains
the current center-based location predicate; source bounding-box/elevation
fidelity remains a separate correction. Host equivalence and a full host win
are established; ARM timing and replay equivalence remain pending. Restore
the standard build with `make -B zerg-z6-probe` after retaining any experiment.
Restore a default build with `make -B protoss-p6-probe` or
`make -B zerg-z6-probe`; Make does not track command-line flag changes.
`tools/sc64_zp_start_profile.py --help` describes a bounded P6 startup
comparison driven by ordinary menu inputs. It does not qualify mission wins.

### Z6 transport integration and P6 query experiments

The internal Z6 probe now requires authenticated Ventral Sacs facts:

```sh
python3 tools/sc64_z6_transport_pack.py
make test-sc64-z6-transport test-sc64-z6-save test-sc64-campaign-spells
make zerg-z6-probe
```

Z6's objective island is disconnected from the opening ground component in
native tile walkability. Morph the starting Hatchery to a Lair, select the
completed Lair, then use `B` → `VENTRAL SACS`. Research costs 200 minerals and
200 gas and takes 3,000 active 30-Hz updates; pausing freezes it. It begins
unresearched. Once complete, select ground passengers and press `A` on a
friendly Overlord to board. Select the loaded Overlord and use `B` →
`UNLOAD CARGO`, then `A` on traversable ground. Capacity is eight source space
points, including one for Kerrigan and two for each Hydralisk. Research,
embarked actors and source identities survive saves. Destroying the carrier
kills its passengers and records their source deaths. Boarding and landing
remain bounded GBA adaptations of the native order machine.

The Z6 save format is generation 32. It also persists the bounded Drone-morph
census used to distinguish construction from killed starting Drones, and
accepts the imported Hatchery's in-place Lair morph. Older internal Z6 saves
are intentionally rejected; the qualified standalone Z5 output is unchanged.

P6 and Z6 heroes can enter Storm targeting with `B+R` when selected alone.
Insufficient energy keeps that selection and reports failure. Successful casts
stop the caster's previous movement order. P6 Storm also damages native
NonBuilding traps held in the engine's Building pool.

The optional P6 query experiments include conservative trap range rejection,
one-shot trigger polling removal, preflight-backed immutable metadata checks,
and a transient Command/Bring snapshot. The snapshot groups repeated source
selectors and has room for every condition's query rectangle. It is built
before predicates and discarded before any action or opaque AI callback;
saves still use full candidate validation.

```sh
make test-sc64-p6-query-experiments
make -B protoss-p6-probe P6_QUERY_FLAGS='-DSC64_P6_PRUNE_STARTED -DSC64_P6_PREVALIDATED_TICK -DSC64_P6_TRAP_CULL -DSC64_P6_COUNT_SNAPSHOT'
```

These flags remain off by default while full playthrough comparisons are in
progress. The sixth missions remain internal probes until normal-input wins,
fresh emulator replay, save continuity and performance are qualified. The
completed staged campaign count remains Terran 10, Zerg 5 and Protoss 5.


Both sixth probes now support the bounded startup observer. Z6 also has a
full-bank replay observer, ready for a genuinely winning controller trace:

```sh
python3 tools/sc64_zp_start_profile.py --profile zerg-z6 --rom build/StarCraft-Zerg-Z6-Probe-GBA.gba --elf build/starcraft-zerg-z6-probe.elf --live-frames 1000 --checkpoint 100
```

The observer reads model, source deaths, mutable locations, ability/research
state, Drone morph history and complete compact actor banks through the
matching ELF's DWARF offsets. `--profile protoss-p6` remains the default.

With the Z6 private inputs prepared, `make test-sc64-z6-controller` runs the
full normal-input mainland-clearance and Ventral Sacs ferry playthrough under
ASan/UBSan. It writes the private replay trace to `build/z6-controller.json`.
The controller also accepts `--trace PATH` when invoked directly. A host win
does not qualify the ARM replay, SRAM handoff or physical GBA performance.

The qualified P6 defaults now include both: `SC64_P6_RESCUE_GROUPED`
groups live friendly rescue candidates while preserving in-frame rescue
chains; `SC64_P6_STORM_POSTCHECK_LOCAL` skips the unchanged mission-model
post-validation after a successful Storm tick. It retains full entry checks
and the live raw-79 energy scan. Run their private-data host checks with
`make test-sc64-p6-rescue-grouped test-sc64-p6-storm-validation`.
Append either flag to an explicit `P6_QUERY_FLAGS` experiment and use `-B`
when changing flags. Neither option has a qualified full-mission FPS result.
Startup reports are explicitly separate from mission-win evidence.

In the Z6 probe, **R** selects the combat group including Kerrigan. **B+R**
selects the army while leaving Kerrigan out, so group orders can keep the
mission-critical hero safe. With only Kerrigan selected, **B+R** instead
opens Storm targeting; an exhausted Kerrigan keeps her selection. This is
an authored GBA control, not an original N64 binding.

P6 burrow-release options are included in the qualified defaults. To run their
four-combination differential checks after preparing the private probe inputs:

```sh
make test-sc64-p6-burrow-cull
```

Add `-DSC64_P6_BURROW_CULL -DSC64_P6_BURROW_GROUPED` to the existing
`P6_QUERY_FLAGS` experiment to compare the grouped candidate. Retain separate
ROM/ELF pairs and use the startup profiler's `--compare` option. These flags
are not enabled in a qualified full-mission build yet; current evidence and
frozen hashes are in [Validation](VALIDATION.md#p6-burrow-release-experiments).

### Complete skirmish controller matches

After preparing the three canonical race profiles, run:

```sh
make test-sc64-skirmish-matches
```

This compiles an input-only controller against each canonical profile and runs
both available maps, all three human races and one to three AI opponents.
Opponent races rotate from the human race; this is 18 configurations per ROM,
not every possible opponent-race permutation. The matrix allows 120,000 input
ticks and fails on an unfinished match, launch failure or sanitizer error.
Human defeats are valid completed matches. Private traces, logs and matrix
reports are written under `.local/sc64/skirmish/{terran,zerg,protoss}-matches/`.
To run one case directly:

```sh
build/test-sc64-skirmish-match-protoss 1 0 2 .local/sc64/skirmish/example.json 120000
```

The arguments are map (0 built-in arena, 1 imported), human race (0 Terran,
1 Zerg, 2 Protoss), opponent count, output trace and optional input-tick cap.
The controller uses ordinary HOME/menu navigation and `game_update` inputs;
its world reads guide decisions without modifying resources, actors or AI.

### Dense input replay without a larger queue

Some full controller routes exceed 256 transitions in a 1,000-tick window.
`SC64_DENSE_SCHEDULE` reduces replay pages to 250 ticks while retaining the
existing 256-entry queue and RAM layout. It changes the debugger observation
cadence; ordinary play continues to use the same game rules. For the P6 probe:

```sh
make -B protoss-p6-probe P6_EXTRA_FLAGS=-DSC64_DENSE_SCHEDULE
python3 tools/sc64_zp_controller_replay.py --help
```

The Z3 and staged Zerg/Protoss replay tools discover the shorter period from
the ELF. Other existing profilers still expect their documented 1,000-tick
builds. Rebuild with the original flags before using those profilers.
For a dense skirmish ROM/ELF, use the dedicated observer:

```sh
python3 tools/sc64_skirmish_controller_replay.py \
  --trace .local/sc64/skirmish/example.json \
  --rom /absolute/path/to/matching.gba \
  --elf /absolute/path/to/matching.elf
```

Supply the matching build used to generate the trace; profile-specific actor
capacities can change the outcome. The tool verifies a fresh HOME start,
feeds only scheduler input fields, and checks the final configuration, frame,
result and live actor counts. Its FPS mean uses GBA timer deltas between live
PLAY checkpoints, excluding HOME/start and final RESULT intervals.

### Prepare the qualified staged Zerg and Protoss builds

The wrapper also supports the six-mission Zerg development profile and the
five-mission Protoss Shuttle profile. From a fresh checkout, use your supported
StarCraft 64 USA ROM:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft64-USA.z64" --profile zerg-z6
./dev sc64 build --profile zerg-z6
./dev sc64 test --profile zerg-z6
./dev sc64 prepare --rom "/path/to/StarCraft64-USA.z64" --profile protoss-p5-shuttle
./dev sc64 build --profile protoss-p5-shuttle
./dev sc64 test --profile protoss-p5-shuttle
```

These generate the required maps, art, scenario, ability and transport headers
under `.local/`; Z6 also generates its inherited Z5 sprite bank. Inspect
readiness with `./dev sc64 status --profile zerg-z6` or the corresponding
Protoss profile. The output ROMs are `build/StarCraft-Zerg-Z6-Probe-GBA.gba`
and `build/StarCraft-Protoss-P5-Shuttle-GBA.gba`. The ordinary `zerg` and `protoss` names select the canonical six-mission
products; these explicit staged names keep their own files.

### Reproduce the canonical P6 host win

After preparing `--profile protoss`, the normal test command runs the full
canonical feature set and all carried missions:

```sh
./dev sc64 test --profile protoss
make test-sc64-p6-canonical PROTOSS_P6_CONTROLLER_TRACE=.local/build/p6-my-run.json
```

The canonical product pins validated P5/P6 query choices. `P5_QUERY_FLAGS`,
`P6_QUERY_FLAGS` and `P6_EXTRA_FLAGS` are for their internal experiments and do
not change this product. Keep canonical P6, internal P6 probe and legacy P5
ROM/ELF/save files separate.

### Reproduce the internal P6 probe host win

After preparing the P6 probe inputs described above, run:

```sh
make test-sc64-p6-controller
```

The host test enters the probe through ordinary HOME/menu controls, then
steers from the live actors. It recruits the two early Marine groups,
regroups them, and sends their escort ahead of Tassadar before completing
the Zeratul rescue and extraction. The earlier 102,005-tick opening fixture
is historical regression data; it is no longer the default route.
Game assets still come from local preparation, and no private opening input
file is required. The default 180,000-gameplay-frame limit covers the full
win. To write a separate trace without replacing the default proof file:

```sh
make test-sc64-p6-controller P6_CONTROLLER_TRACE=.local/build/p6-my-run.json
```

Full trace replay on ARM requires a dense probe build because some input
windows exceed the standard queue's 1,000-tick capacity; see the dense replay
section. This target is a deterministic regression route, with exact source
hero/extraction/completion assertions at victory.
The current live route wins at frame 49,599; an input-only ARM replay matches
all 57 observed mission/hero checkpoints. Its measured throughput remains
about 10.5 fps equivalent, and this is still the P6 probe rather than the
canonical Protoss product. The newer canonical P6 qualification is recorded
separately in the [promotion audit](audits/2026-10-27-p6-promotion/README.md). See the
[live-route qualification](audits/2026-10-27-p6-live-route/README.md).

The expanded Z/P collision lookup has a dedicated
sanitizer differential target after Z6 inputs are prepared:

```sh
make test-sc64-z6-collision-facts
```

`SC64_COLLISION_FACT_HOIST` changes only repeated collision fact lookups.
The target explicitly builds its reference and hoisted variants independently
of the profile's enabled query defaults.

`SC64_ZP_FACT_RANGE_CULL` rejects ordinary roster
types before checking optional campaign actor facts. Its differential test
needs no imported assets and compares complete resolver/wrapper answers
across 16 actor-gate combinations, including P7 and the late Terran enum prefix:

```sh
make test-sc64-zp-fact-range
make -B zerg-z6-probe Z6_QUERY_FLAGS='-DSC64_Z6_COUNT_SNAPSHOT -DSC64_COLLISION_FACT_HOIST -DSC64_ZP_FACT_RANGE_CULL'
```

The combined build reproduces the complete winning Z6 host route and the
12,000-input-tick ARM prefix. The range cull improves that prefix from 10.47
to 10.97 fps. The full ARM route wins at 19.01 average effective fps, with
both opening and late saves restoring exactly in fresh processes. The three
options now form the Z6 defaults; the staged Z7 profile inherits them.
`Z6_QUERY_FLAGS=` builds the reference path, and `Z6_EXTRA_FLAGS` appends
diagnostic or experimental options without replacing those defaults.

The separate collision-class cache remains disabled by default:

```sh
make test-sc64-z6-collision-classes
make -B zerg-z6-probe Z6_EXTRA_FLAGS=-DSC64_COLLISION_CLASS_CACHE
```

It uses one transient byte per unit to retain type-derived collision facts
within a bucket phase. Its 12,000-input opening improves from 10.97 to
11.29 fps, with matching reported states and captures. A full ARM route and
save qualification are still required for that candidate. It uses the legacy
per-frame bucket lifecycle and rejects late-Terran persistent storage.

P6's eight query options are enabled by default after the full ARM win and
fresh-process save checks. `P6_QUERY_FLAGS=` explicitly builds the reference
path; assigning a subset replaces the defaults for a component comparison.
`P6_EXTRA_FLAGS` appends replay/diagnostic options without replacing the query
options. Differential Make fixtures explicitly select their on/off variants.
The qualified dense P6 run averages 12.75 fps; 30 fps remains unfinished.

The Probe-only command menu includes CANNON after NEXUS. The existing Forge,
power, cost and placement rules apply; the new row provides access to them.
`make test-sc64-probe-cannon test-sc64-probe-expansion` checks both commands
in basic unified, Z6 and P6 builds.

P3 has an optional bounded production-exit experiment:

```sh
make test-sc64-production-exit test-sc64-p3-production-exit
make -B protoss-p5-shuttle P3_EXTRA_FLAGS=-DSC64_P3_PRODUCTION_EXIT
```

It is active only during P3 and remains disabled by default. Clear exits keep
the existing location; blocked exits search for reachable space, and completed
orders wait when no space is available. This is a bounded GBA placement model,
with the limitations recorded in VALIDATION. Use a clean rebuild without the
extra flag to return to the normal P5 build.

Expansion resource returns have a separate opt-in experiment:

```sh
make test-sc64-nearest-resource-depot
make -B protoss-p5-shuttle P3_EXTRA_FLAGS='-DSC64_P3_PRODUCTION_EXIT -DSC64_NEAREST_RESOURCE_DEPOT'
```

`SC64_NEAREST_RESOURCE_DEPOT` selects a same-owner, reachable active depot
instead of the first base slot, and caches it for a resource trip. Morphing
Lairs/Hives remain active depots; a new unfinished Hatchery does not. Missing
depots preserve the worker's resource order and cargo until a topology or
depot-completion event allows another selection. This needs the prepared
P5/Z6/P6 private inputs for its actual-engine sanitizer matrix.

The experiment remains disabled by default. Its focused behavior/save tests
and ARM reserve checks pass, but the existing Z6 controller stalls with the
changed economy and does not qualify a complete route. The Z6 controller
target now exits nonzero for a loss or bounded attempt without a victory.
Worker resource handling stays in ROM so compiler inlining cannot consume
the IWRAM stack reserve; movement and combat retain their existing routines.

The larger P5 bank has a separate P3 expansion controller using both options:

```sh
make test-sc64-p3-expansion-controller
make -B protoss-p5-shuttle P3_EXTRA_FLAGS='-DSC64_P3_PRODUCTION_EXIT -DSC64_NEAREST_RESOURCE_DEPOT'
python3 tools/sc64_p3_controller_replay.py --route expansion --trace build/p3-expansion-controller.json
```

The host target writes `build/p3-expansion-controller.json` and fails unless
the source objective reaches a verified win. It uses paid expansions,
production and normal controller commands; it preserves the canonical P3
controller. The additive replay route validates the launch, final source
state and windowed input capacity before starting mGBA. This larger-bank
route has a host win at frame 123916. Its ARM replay and fresh-process saves
are separate qualification checks; do not promote it from the host result.

Adding `--ticks 1000 --save-at-end` to the replay command creates a bounded
opening save. Run `python3 tools/sc64_p3_resume_smoke.py --report
/path/to/prefix/profile.json` against the same ROM/ELF to load that SRAM from
HOME in a fresh process. It compares the reported persisted state, explicitly
reports the building-selection reset on load, and verifies one neutral
handoff frame followed by 60 live frames. Unit selection remains checked.

After Z7 inputs are prepared, `make test-sc64-z7-gas-restore` checks destroyed
source Extractors reopening their geyser locations and the corresponding
transactional save validation. This does not establish a full Z7 win.

Construction hit-point progression has a separate default-off
`SC64_CONSTRUCTION_HP` gate requiring race rules. It starts ordinary buildings
at one tenth HP, grows HP and Protoss shields with construction, and retains
damage on completion. Zerg upgrades use their separate morph HP rules. The
asset-free integer checks are included in `./dev test` and can run alone:

```sh
make test-sc64-construction-hp
```

With unified inputs available, `make test-sc64-construction-hp-game` runs
actual worker/construction lifecycle fixtures. With the existing Z7 private
inputs prepared, `make test-sc64-construction-hp-z7` also checks Lair and
damaged Sunken/Spore morphs, including packed-save continuity. These test
targets enable the gate explicitly; standard ROM flags leave it off pending
complete mission and fresh-process SRAM qualification. See
[Validation](VALIDATION.md#experimental-construction-hit-point-progression)
for the integer-timing and pre-arrival-foundation limits.

The default-off `SC64_RENDER_KEY_ONCE` option requires an imported profile.
It computes the static terrain/object key once per draw and passes it through
cache lookup and capture, extending the existing T5 keyed path. It adds no
global storage or save fields. Asset-free cache checks are included in
`./dev test` and can run with `make test-terrain-key-once`. With the existing
P5 Shuttle, P6 and Z6 inputs prepared, `make test-sc64-render-key-once` compares
the actual world renderer with full-clear output under all three profiles.
Standard flags leave this experiment off pending ARM timing and replay checks.

P7 has a gated source-fact foundation, independently testable without assets:

```sh
make test-sc64-p7-actor-rules test-sc64-zp-fact-range
make test-sc64-p7-geometry
```

`SC64_P7_ACTORS` supplies the missing Archon, Arbiter, Carrier and Observer
identities plus seven Protoss structure types. They remain scenario-only;
this does not expose new training commands or a playable P7 campaign entry.
The P7 header records missing splash, interceptor, cloak/detection, Arbiter
field and Shield Battery mechanics. Enum capacity fits the current packed
save fields. Separate experimental gates now exercise Archon splash,
Observer detection and Battery recharge; a complete P7 playthrough remains
unqualified.

The asset-free geometry fixture checks source-pixel actor rectangles, including
inclusive source extents and malformed coordinate rejection. Runtime centers
are doubled before applying the source extents. With P7's private scenario
prepared, `make test-sc64-p7-runtime` checks both actor-gate configurations;
the enabled fixture checks the complete placement census and location edges.
These are isolated integration checks, without a P7 playthrough qualification.

P7's sprite importer is also opt-in and writes to a separate private directory:

```sh
python3 tools/sc64_race_sprites.py --rom "/path/to/StarCraft 64.z64" --include-p6-actors --include-z5-actors --include-z6-actors --include-z6-buildings --include-p7-actors --out .local/sc64/p7/generated/sprites --header .local/sc64/p7/generated/sc64_race_sprites_generated.h
```

It adds Arbiter art and complete source frame banks for seven Protoss
structures, reusing the shared Archon, Carrier and Observer images. The
generated completeness marker requires that shared art as well. This prepares
source images only; native animation selection and P7 renderer/profile
integration remain unfinished. Generated art and previews stay local.

Shield Battery starting properties can be prepared separately from the same
authenticated P7 source CHK:

```sh
python3 tools/sc64_p7_battery_pack.py
```

The packer uses the shared scenario authenticator, rereads and hashes the CHK,
then derives the original Battery placements and HP/shield/energy percentages.
It writes `sc64_p7_battery_properties_generated.h` under the ignored scenario
directory. Use `--source-root` and `--output` for other local staging paths.
The public source contains no copied map coordinates or generated properties.
This preparation does not enable recharge or expose P7 in a campaign build.

The default-off `SC64_P7_BATTERY_ENABLED` integration requires the P7 runtime,
actor facts and full Protoss campaign storage. With the existing private P6
inputs, the P7 scenario, properties and sprite bank prepared, run:

```sh
make test-sc64-p7-battery
make protoss-p7-battery-probe
```

The internal ARM probe combines Battery, Observer and Archon gates. It retains
the qualified campaign menu; it is not a completed P7 release. In a P7 test
world, a selected Probe exposes SHIELD BATTERY after a completed Gateway;
placement requires a completed same-source-owner Pylon's field and pays 100
minerals. The Battery checks the pinned source psi mask at the represented
half-resolution actor centers, including its asymmetric cell boundaries.
A selected completed, powered
Battery exposes RECHARGE: click a friendly unit or a point to order eligible
units in the source-size square to approach it and recharge shields.

Losing power cancels Recharge on the actor's next service update, before any
transfer. Saving immediately after a supporting Pylon dies preserves the
pending order; loading cancels it on its next update. Saving does not advance
the simulation. Battery energy regeneration and construction progress continue
without power, as in the pinned reference. Native disabled statuses that the
adapter does not represent remain unsupported.

The service uses existing fields for Q8 energy, regeneration phase and shield
fractions. P7 state shares the existing Protoss save union; internal generation
37 rejects older experimental formats. No actor fields or reserve increases
are introduced. The fixed-point and psi-mask geometry fixtures join the
asset-free default suite; the full target also checks actual
construction/recharge saves, immediate power-loss pause/save/load, source
bindings, recycled targets, and existing P5/Shuttle/P6 save fixtures with the
gate enabled. See [Validation](VALIDATION.md#experimental-p7-shield-battery)
for limits and scoped ARM evidence.

The experimental `SC64_P7_ARCHON_SPLASH` combat path is separately gated and
requires `SC64_P7_ACTORS`, `SC64_ZP_CAMPAIGN_ACTORS` and `SC64_RACES`:

```sh
make test-sc64-p7-archon-rules
# Requires the existing private P6 inputs for the shared engine fixture:
make test-sc64-p7-archon-splash
```

It uses source collision rectangles for reach and radial damage, the source
30+3 weapon upgrade, and owner-based Enemy Splash exclusions. Q8 damage is
adapted to the engine's integer HP/shields. Impact is immediate at the target
center; projectile/windup and native burrow handling remain unfinished. This
does not enable P7 in a campaign build or qualify its missing abilities.

Splash compares known source-player identities even when two P7 source
players share one runtime opponent slot. Actors with unknown source owners
fall back to runtime ownership; future P7 production integration must assign
that source provenance to newly created units and buildings.

`SC64_ROUTE_VISIT_GRID` is an independent, default-off packed-route
experiment. It requires `SC64_RACES` and `SC64_ROUTE_PACK3` and borrows the
walkability grid to mark cells during a reverse flood. The existing BFS queue
restores those bits before returning, without extra persistent storage.
Callers with a custom grid getter must also supply its matching setter.

```sh
python3 -m unittest discover -s tests -p 'test_sc64_routes.py'
```

The complete P3 host trace and state fingerprints match the reference with
this gate enabled. ARM timing and full replay/save checks are still pending;
it is not part of any qualified profile's defaults.


### Experimental direct attack orders

`SC64_DIRECT_ATTACK` is a default-off race-runtime gate. In the ordinary
ATTACK command, clicking a visible hostile unit or building gives capable
selected units a direct order against that actor. Clicking ground retains
Attack-Move. The order keeps its target through cooldown and movement instead
of choosing a nearer enemy on every update. It uses the existing packed order,
target and last-visible destination fields; it adds no save fields or globals.

With the local race and Z7 inputs already prepared, run:

```sh
make test-sc64-direct-attack
```

This runs actual-engine ASan/UBSan fixtures for the plain race, unified and
Z7-plus-construction-HP configurations. The feature remains off in standard
ROMs pending normal-controller mission and fresh-process SRAM qualification.
See [Validation](VALIDATION.md#experimental-direct-attack-orders) for the
bounded visibility and targeting policy.


### Experimental packed route-field initialization

`SC64_ROUTE_WORD_CLEAR` requires race rules and `SC64_ROUTE_PACK3`. It replaces
the byte-at-a-time all-unreachable field initialization with aligned,
alias-qualified 32-bit writes. A byte prefix and tail handle arbitrary row
sizes and unaligned later slots; the path adds no persistent or scratch state.
It remains off in standard profiles.

```sh
make test-sc64-route-word-clear
```

The asset-free sanitizer fixture runs 128×128, 128×96 and 10×10 maps, including
unaligned 38-byte rows, canaries and independent BFS field comparisons. It is
part of the default test suite. [Validation](VALIDATION.md#experimental-packed-route-field-initialization)
records the full host parity and limited ARM timing evidence.


### Experimental clear-corridor routing

`SC64_ROUTE_L_FAST_PATH` requires race rules and `SC64_ROUTE_PACK3`. Before
requesting a cached route field, it checks the reverse search's preferred
horizontal-then-vertical corridor from destination to origin. If the entire
corridor and both endpoints are walkable, its first step is identical to the
existing left/right/up/down BFS tie order. Otherwise the ordinary route cache
handles the request. Blocked origins and destinations retain their existing
fallback behavior.

```sh
make test-sc64-route-l-fast
```

The asset-free ASan/UBSan fixtures cover 64×64, 128×96 and 128×128 grids, plus
128×96 with partial flooding. They compare exact first steps and complete
fallback fields with an independent BFS, including obstacles outside a clear
corridor, blocked endpoints and cache eviction/reset. This command is part of
the default test suite. The gate adds no route, actor or save storage and
remains off in standard profiles pending broader qualification. See
[Validation](VALIDATION.md#experimental-clear-corridor-routing) for the
full host/ARM replay, timing and opening-save evidence and remaining limits.


### Experimental route fields sized to the active map

`SC64_ROUTE_ACTIVE_STRIDE` requires race rules, `SC64_ROUTE_PACK3`,
`SC64_ZP_CAMPAIGN_STORAGE` and `SC64_LARGE_MAPS`. It partitions the existing
86,016-byte route bank using the active map's packed field size. A 128×96
map fits 18 fields instead of 14; a 128×128 map retains 14. Metadata and field
pointers reset when map dimensions change. No field-data bank or save bytes
are added; the pointer/configuration metadata uses 112 additional ARM BSS bytes.
The flag remains off in standard profiles pending broader qualification.

```sh
make test-sc64-route-active-stride
```

The standalone asset-free sanitizer fixture checks exact BFS first steps and
complete fallback fields, both word and byte initialization, runtime shape
changes, field boundaries, unused pool bytes and invalid dimensions. Existing
route fixtures remain unchanged. See
[Validation](VALIDATION.md#experimental-route-fields-sized-to-the-active-map)
for the measured intervals and qualification limits.


### Experimental independent Z7 Egg production

`SC64_Z7_EGG_PRODUCTION` requires the race and Z7 runtimes and is active only
in Z7. Ordinary larva products become independently ticking raw-36 Egg actors
instead of serialized producer queue entries. The existing live-larva cap is
three; it does not cap Eggs. Started Eggs survive Hatchery death and slot reuse.
The authored enemy policy still buys Zerglings at its existing pulse cadence.

With the private Z7 inputs already prepared:

```sh
make test-sc64-z7-egg-production
make test-sc64-z7-egg-save
```

Select your Egg, open the command menu and choose **CANCEL EGG** to receive the
full product mineral/gas refund. STOP and movement commands leave its paid
product intact. Both Zergling and Scourge Eggs reserve supply for two outputs.
If no slot exists for the second unit at hatch, the first completes and the
second is attempted only once, matching the pinned reference's allocation
failure behavior.

Existing larvae also remain usable while an owned Hatchery upgrades to Lair
or Hive: the normal R/A production controls can start paid Eggs during the
morph, and the existing 428-update larva counter continues toward its live
cap of three. An unfinished newly built Hatchery remains unavailable. This
models larvae through the town-hall proxy; native individual-larva movement
and spawning timing remain separate limitations.

During those upgrades, the active Z7 build also retains the preceding hall's
tech tier and the existing 160-pixel creep proxy. A Lair morph retains Hatchery
requirements; a Hive morph retains Lair requirements. It grants the new tier
only when complete. Extractors follow the source's Drone-and-geyser requirement
while keeping normal placement checks and the 50-mineral cost.

This gate uses save generation 36; generation-35 internal Z7 records are
rejected. Standard profiles keep the gate off. See
[Validation](VALIDATION.md#experimental-independent-z7-egg-production) for
runtime, save and fidelity limits.


### Experimental Z7 source sight

`SC64_Z7_SOURCE_SIGHT` uses the pinned base sight ranges for supported Zerg
units and buildings only while Z7 is active. It replaces generic worker/army
radii with the type's source range, while retaining the existing GBA rasterizer.
Ordinary unfinished buildings use four source tiles; upgrades retain their
predecessor sight until completion. The existing visibility snapshot encodes
that derived state without adding storage or changing saves.

```sh
make test-sc64-z7-source-sight
```

The fixture runs with and without independent Egg production. It compares
incremental visibility with a fresh scan through movement, transport, removal,
construction and real morph transitions. Standard profiles leave both gates
off until mission and fresh-process SRAM qualification.

### Experimental Z7 Scourge combat

`SC64_Z7_SCOURGE_COMBAT` requires the Z7 runtime and independent Egg production.
With the gate enabled, select a Hatchery or Lair and press R to cycle through
the Zerg products, including **SCOURGE**. A completed Spire is required. A buys
one Egg for 25 minerals and 75 gas; its normal timer produces two Scourge and
reserves both outputs' supply. Each Scourge attacks hostile aircraft once for
110 base damage, then is consumed. Weapon upgrades do not increase that damage.

```sh
make test-sc64-z7-scourge
```

The focused actual-engine fixture checks real R/A dispatch, paid production,
mid-Egg save/load, both births and combat. Standard profiles leave this gate
off pending mission, fresh-process SRAM and performance qualification.

The Z7 trigger geometry also recognizes an ordinary Spire's source footprint.
This correction applies independently of the Scourge gate: previously a Spire
could make a Bring condition fault and pause the mission.

### Experimental Z7 derived save rows

`SC64_Z7_DERIVED_SAVE` is a default-off internal save-format experiment for the
complete Zerg ZP/Z5/Z6/Z7 Egg profile. With its private inputs already prepared:

```sh
make test-sc64-derived-save
make test-sc64-z7-derived-save
make zerg-z7-derived-save-probe
```

The probe combines the current construction-HP, direct-attack, Egg, source-sight,
Scourge and nearest-depot gates. It writes
`build/StarCraft-Zerg-Z7-Derived-Save-Probe-GBA.gba`. Standard profiles retain
their existing save format. The probe uses generation 38 and rejects older
internal saves; it is not a qualified Z7 campaign release.

Unit rows derive maximum HP and armor from the same profile facts used by
constructors; Building rows derive maximum HP. The unused `Unit.weapon` field
is restored as zero, so this is semantic preservation rather than a bitwise
round trip of that field. Changing those facts or giving that field gameplay
meaning requires a new generation or an explicit compatible migration.

At 240 Unit and 160 Building slots, rows shrink from 35/28 to 29/25 bytes,
reducing RaceSave from 16,296 to 14,376 bytes. This creates 1,920 bytes of save
space without changing actor sizes or increasing RAM reserves. It does not
allocate a projectile pool or establish any frame-rate improvement. See
[Validation](VALIDATION.md#experimental-z7-derived-save-rows) for checks and
remaining qualification work.


## Z8 authenticated source descriptor

`tools/sc64_z8_pack.py` authenticates previously retained Z8 scenario data
and writes a complete numeric descriptor to a new ignored
`.local/build/z8-*` path. Its `verify` command compares all rows with the
authenticated source, rather than trusting a self-reported fingerprint.
It does not regenerate shared banks or enable Z8 gameplay. See the
[Z8 source foundation audit](audits/2026-10-27-z8-source-foundation/README.md)
for explicit commands, provenance, supported data and remaining runtime work.

## Experimental P7 menu and prevalidated tick

After preparing the current private P6/P7 inputs, build separate Homeland
reference and optimized probes with:

```sh
make test-sc64-p7-prevalidated
make protoss-p7-runtime-reference protoss-p7-runtime-probe
```

The ROMs are `build/StarCraft-Protoss-P7-Runtime-Reference-GBA.gba` and
`build/StarCraft-Protoss-P7-Runtime-Probe-GBA.gba`. Both expose P7 through the
normal single-race mission menu under `SC64_P7_CAMPAIGN_ENTRY`. This is an
internal candidate entry: Carrier/Arbiter mechanics, production/AI fidelity,
a full normal-input win and campaign performance remain unfinished. Canonical
race ROMs do not enable this entry and qualified staged progress stays 22/30.

The optimized probe additionally enables `SC64_P7_PREVALIDATED_TICK`. It
validates immutable mission data at initialization and load, then checks
mutable trigger, timer, wallet, location and binding state on each tick.
Save candidates retain the full validators. It skips the repeated map census
and immutable table scans without changing the persistent state format.
The host fixture compares both profiles through menu launch, pause/save/load,
hero-death conditions and a synthetic human-force defeat. The fast profile
also checks full/fast validator agreement for 31,744 individual byte mutations.
See [Validation](VALIDATION.md#p7-prevalidated-runtime-opening) for the scoped
ARM measurement; neither probe is a near-30-fps campaign release.

An additional default-off `SC64_P7_COUNT_SNAPSHOT` probe counts the eight
authenticated ownership/type conditions in one actor census per runtime tick:

```sh
make test-sc64-p7-count-snapshot
make protoss-p7-count-probe
```

It produces `build/StarCraft-Protoss-P7-Count-Probe-GBA.gba` and requires the
prevalidated tick gate. The counts exist only during condition evaluation;
invalid census data falls back to the checked callback. The differential
fixture also exercises all three scripted actor creations, invalid ownership,
unfinished structures and the Nexus-absence conditions. These synthetic
condition stimuli do not qualify a mission win. See
[Validation](VALIDATION.md#p7-count-snapshot-opening) for the opening measurement.

A separate default-off movement probe adds the existing
`SC64_ZP_FACT_RANGE_CULL` and `SC64_COLLISION_CLASS_CACHE` options and the new
`SC64_COLLISION_STEER_LIST` to the count-snapshot flags:

```sh
make test-sc64-p7-move
make protoss-p7-move-probe
```

It produces `build/StarCraft-Protoss-P7-Move-Probe-GBA.gba`. When a ground
mover's next step is blocked, the steering list resolves nearby collision
candidates once, then answers the eight one-step side checks from that short
list. Each answer matches `unit_step_clear()`; workers that are gathering, an
inactive bucket index or more than 48 candidates use the original query.
`test-sc64-p7-move` compares reference and candidate traces over a neutral
opening and a controller-only attack-move. It checks 619,216 list answers
against the original query, then repeats the menu/save-load fixture. See
[Validation](VALIDATION.md#p7-movement-steering-list) for ARM measurements.

An experimental mode-0 renderer builds on that movement probe:

```sh
make test-sc64-p7-tile
make protoss-p7-tile-probe
```

`tools/sc64_tile8_pack.py` first derives
`.local/sc64/tile8/generated/sc64_races_generated.h` from the existing
imported header without modifying it. Each map's 16x16 terrain tiles become
8x8 tiles that are flip-deduplicated across maps compiled together, plus four
GBA-style entries per source tile. `SC64_TILE_RENDERER` then shows imported
race-campaign play screens in mode 0:

- BG3: terrain, streamed through a 464-slot VRAM tile cache.
- BG2: fog.
- BG1: static art (minerals, geysers and building art).
- BG0: the HUD band.
- Eight 64x64 sprites carry units, overlays, the cursor and text from the
  unchanged software primitives.

Menus and skirmish maps keep mode 4. `test-sc64-p7-tile` composes the emulated
layers on the host and requires every rendered frame to match the mode-4
build, pixel for pixel. See
[Validation](VALIDATION.md#p7-mode-0-tile-renderer) for its limits.

A second default-off probe adds `SC64_FACT_TABLES` to the movement-probe flags:

```sh
make test-sc64-fact-tables
make test-sc64-p7-fact-tables
make protoss-p7-fact-probe
```

It produces `build/StarCraft-Protoss-P7-Fact-Probe-GBA.gba`. Before compiling,
`tools/sc64_fact_tables_gen.c` is built on the host with the probe's exact
flags. It writes packed per-type ROM tables to
`build/generated/p7-fact-tables/sc64_fact_tables_generated.h`. The tables hold
unit actor flags, collision radius and flight, plus building actor flags. They
come from the unchanged functions, which stay available as `*_reference`. The
generated header records the actor gates it was built with, and a build with
different gates fails to compile. Types outside the roster still use the
reference functions. `test-sc64-fact-tables` needs no game data. For every
actor-gate configuration, with and without `SC64_ZP_FACT_RANGE_CULL`, it
compares each table entry with the reference functions. It also checks that
every fact-selecting macro is recorded and that a table built for other gates
is rejected. `test-sc64-p7-fact-tables` repeats the table check under the P7
flags. It also compares the P7 movement trace and the menu/save-load fixture
with the reference builds. See
[Validation](VALIDATION.md#p7-build-time-fact-tables) for ARM measurements.

## Mineral gathering before a base exists

The shared first-depot worker path lets an empty mineral worker gather before
a friendly completed base exists. A loaded worker keeps its cargo and mineral
order, then uses the existing return/delivery path when a base becomes ready.
This fixes Z7's base-less opening without changing gas orders, depot selection
or the save format. The separate nearest-depot experiment remains opt-in.

```sh
make test-sc64-no-depot-harvest
make test-sc64-no-depot-harvest-z7
```

The first check is included in `./dev test` and needs no game data. The second
uses the prepared private Z7 Scourge profile and covers SCVs, Probes and Drones.
See [Validation](VALIDATION.md#mineral-gathering-without-a-completed-base) for the
native-reference limits and shared-profile regression results.

## Active Zerg resource depots

The standard first-depot worker path accepts Lairs and Hives while they are
under construction, as the source rules do. The GBA Hatchery-to-Lair command
retags the building immediately, so upgrading no longer suspends mineral or
gas deliveries. Fresh unfinished Hatcheries remain inactive. Existing
first-slot selection, owner checks and airborne rejection are preserved.

With the private Z6/Z7 inputs prepared, run:

```sh
make test-sc64-active-hall-depots
```

This checks the actual Lair command, cargo delivery during its upgrade,
unfinished Lair/Hive eligibility, fresh Hatchery rejection, slot reuse and
packed building roundtrips. The Hive fixture supplies an unfinished Hive
state; it does not establish a Lair-to-Hive command. See
[Validation](VALIDATION.md#active-lair-and-hive-resource-depots) for scope.


## Experimental P7 Arbiter cloak and cached detection

The default-off `SC64_P7_ARBITER_CLOAK_CACHE_ENABLED` profile joins P7 Observer
visibility, intrinsic cloak for Observers and the two authenticated Dark
Templar identities, and same-source-owner Arbiter aura. Its host matrix is
explicitly opt-in:

```sh
make test-sc64-p7-arbiter-host
```

The matrix builds full, prevalidated and count-snapshot versions of the existing
runtime fixture, then compares their output. It also covers cache policy,
Battery save/load with the cache enabled, and Observer/Battery compatibility
with the cache gate disabled. The optional ARM link target is
`make protoss-p7-arbiter-cache-probe`, which includes the prevalidated and
count-snapshot runtime options.

The cache follows the source's 24 Hz update cadence against the GBA's 30 Hz
simulation: four source updates are spread over five GBA frames. The first
refresh occurs on the first ordinary play tick after map setup. A target newly
entering an Arbiter aura or intrinsic cloak is refreshed at the next source
update; it does not force a detector refresh on the exact GBA frame the cloak
begins. Aura and detection ownership use the authenticated source player
identity, including raw owner, and current live-owner mapping. The cache stores
its target mask, countdown, validity, aura and tracked bits in the existing
`Unit.weapon` word. The source refresh stagger and phase occupy P7's existing
reserved bytes. The feature selects save generation 39; the default profile
remains cache-off.

These checks cover engine integration and save behavior. They do not establish
mission completion, frame rate, or physical hardware behavior. No added Unit
field is introduced. The opt-in ARM build checks the existing memory limits;
runtime timing still needs separate qualification.

The cache clock is zero before the first source update. Saving immediately
after launch preserves that pending update; loading this initial save follows
the same first-tick path as resuming without a reload. A separate normal-menu
fixture pauses and saves before any play tick, clears mission/cache process
state, loads from the title, and compares the first-tick units, buildings,
mission state, frame and RNG with the uninterrupted path. Running saves keep
their existing detection state and phase until the saved clock advances.

## Experimental P7 typed save rows

`SC64_P7_TYPED_COMPACT_ENABLED` is an opt-in generation-40 codec for the
complete P7 Arbiter/cache profile. It uses 28-byte Unit and 25-byte Building
rows, reconstructing checked canonical max HP and unit armor from the current
type catalog. It preserves every bit of the 16-bit Unit weapon/cache word,
source identities, and the P7 clock/phase supplement. Invalid noncanonical
fields reject instead of being truncated. Generation-39 saves are a separate
format; no migration is provided by this experiment. Standard profiles keep
their existing codecs and save generations.

Asset-free packing and malformed-profile checks run in `./dev test`. With
private P7 data prepared, use:

```sh
make test-sc64-p7-typed-compact test-sc64-p7-typed-compact-matrix
make protoss-p7-typed-probe
```

The first target checks actual type facts and campaign/skirmish save roundtrips;
the matrix checks full/prevalidated/count parity, active cache/recharge saves
and initial-launch save timing. The ARM probe includes prevalidation and the
count snapshot. Its format frees save space for future projectile state; it
does not implement Carrier Interceptors or prove that a complete projectile
and child-actor pool will fit. See [Validation](VALIDATION.md#experimental-p7-typed-save-rows).

The composed generation-40 movement probe is also opt-in:
`make test-sc64-p7-typed-move` compares typed count-snapshot and combined
runtime traces, compares the steering-list fixture against the typed count
baseline while checking each list answer against `unit_step_clear()`, and runs
typed Battery and initial-save fixtures. `make protoss-p7-typed-move-probe`
links the combined typed/Arbiter/movement profile as code `SC7M` (title
`SC P7 MOVE40`). It keeps generation 40, the 56-byte Unit, 48-byte Building,
14,188-byte RaceSave and the existing source clock. The combined host matrix,
ARM memory checks and a matching 600-update emulator opening pass; measured
effective fps is 7.880 for typed count and 9.045 for typed movement. See
[Validation](VALIDATION.md#generation-40-movement-composition) for the exact
comparison and limits. It is not part of the canonical or default build.

The opt-in rectangle-reuse probe layers
`SC64_P7_GEOMETRY_RECT_HOIST_ENABLED` on that typed/Arbiter/movement profile:

```sh
make test-sc64-p7-rect
make protoss-p7-rect-probe
```

It validates an Arbiter's source rectangle once per source tick and reuses
target rectangles for aura finder and distance checks and detector scans.
Ownership, range thresholds, clipped finder edges and detection cadence are
unchanged. The host target compares runtime/steering traces, checks geometry
against the original helpers, and runs Battery and initial-save fixtures.
The ARM output is `build/StarCraft-Protoss-P7-Rect-Probe-GBA.gba`, code `SC7R`,
title `SC P7 RECT40`; save generation, actor sizes and RAM ends are unchanged.
See [Validation](VALIDATION.md#p7-source-rectangle-reuse) for the measured
opening/attack workload and its limits. This probe does not enable Carriers
or qualify P7 for campaign promotion.

Carrier pool, codec and motion checks are asset-free and included in
`./dev test`. The separate generation-41 P7 projectile probe reserves the
Mode 4 arena and integrates game updates, damage consumption and saves.
The separate generation-42 training probe adds paid Interceptor queues and
saved hidden hangar children. Carrier production, launch/flight and the
weapon-event caller remain unfinished; neither enables Carrier combat.
See [Carrier projectile development](CARRIER-PROJECTILES.md)
for commands, memory ownership, reference evidence and promotion limits.

Ordinary attacks use authored weapon-family effects: muzzle flashes, needles,
missiles, plasma, flame, melee, acid, shells, psionic arcs, explosions, colony
spines and laser pulses. Ground/air weapon variants have separate profiles;
defenses and enabled traps use the same clipped, fog-aware renderer. These
effects preserve the adaptation's existing instantaneous damage timing and
are not imported native IScript animations. They do not enable weapons on
actors whose combat is still unsupported.

`make test-sc64-weapon-vfx` checks asset-free rendering geometry and clipping.
With the existing private inputs, `make test-sc64-weapon-vfx-profiles` checks
the unified, full Terran, Z7 and P7 actor sets and actual firing events.
`make showcase` includes the effects in the filming ROM; the canonical race
builds include them as well. The transient pool holds 32 effects in race
profiles and retains 12 in the original-art save format. Overflow replaces
the oldest cosmetic effect and never changes damage or RNG. Actor and save
rows retain their existing layouts.
Deaths use a separate transient pool; see
[Death animations](#death-animations).

For repeatable filming setups, `make showcase` builds the separate
`StarCraft-Showcase-GBA.gba` with a hidden scene menu. On HOME hold R, add L,
then tap Select. `make test-sc64-showcase` checks the menu, scene resets,
last-stand timing and save isolation with the existing private P7 inputs.
The frontend input model also runs in the asset-free host suite. See
[Showcase scenes](SHOWCASE.md) for all eight setups, recording controls,
Carrier preview limits and the controller-only emulator replay.
