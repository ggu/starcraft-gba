# Native source weapon effects

This optional pipeline imports weapon and effect artwork metadata from an
authenticated StarCraft 64 ROM. It is separate from the existing authored
weapon-family effects and does not make combat behavior equivalent to the
original game.

## Inputs and generated data

The audit reads the USA ROM's BOLT `units.dat`, `weapons.dat`, `flingy.dat`,
`sprites.dat`, `images.dat`, `images.iscript`, LO tables and referenced GRPs.
It records the pinned OpenBW revision used to interpret field and script
semantics. The source sprite generator then emits bounded C tables for mapped
actor/weapon combinations, projectile images, referenced child overlays,
frames, source animation events and selected image attachment offsets. The
Spider Mine's raw unit 13 / ground weapon 6 is included as a special emitter:
the mine owns the detonation visual when triggered, while its launcher remains
a separate actor. The Reaver's launched Scarab raw unit 85 / weapon 82 is also
kept distinct from the Reaver's own unit row.

Generated files belong under ignored `.local/sc64/weapon-vfx/generated/`.
Regenerate an existing bank after updating this pipeline: the runtime requires
the SPROLUSELO LO capability marker and rejects older banks at compile time.
Prepare them through the profile tooling; do not commit ROMs, extracted art,
generated headers or captures. The generator rejects unauthenticated ROMs,
unknown image modifiers, malformed dimensions and invalid frame/LO offsets
rather than inventing replacement art.

Normal palette sprites are converted to the selected GBA display palette.
Destination-dependent source effects keep their authored strength selector
pixels and use projected lookup tables. Those tables are deterministic
functions of the source RGBA5551 palette, source light-table bytes and GBA
display palette. The source table-to-modifier binding is inferred from
available source metadata and renderer behavior; only the image 211 shift-3
projection currently has a byte-for-byte independent projection oracle. GBA
palette quantization means this is not a claim of pixel-identical N64 output.

## Runtime behavior and limits

The source bank preserves source frame IDs, event opcodes, wait durations,
release events, selected child-image streams and loop-entry PCs. The runtime
uses a 24 Hz source clock mapped to the 30 Hz simulation clock. The extracted
script trace is a linear path: random branches, calls, repeat conditions,
game-state tests and the complete image interpreter are not implemented.
Stored source frames and clocks therefore do not establish complete native
animation parity.

Current combat damage keeps its existing immediate timing. Source projectile
art is a presentation layer; it does not delay or alter hits. Moving effects
use captured launch and target positions and a bounded straight-line preview.
They do not continuously home on a moving target, reproduce source turning or
collision, or execute bounce behavior. In particular, the Scout air weapon has
two source bullets with an alternating initial heading offset; the current
renderer allocates both but gives them coincident captured-endpoint paths.
Its source artwork/count are present, but visibly separated native twin-missile
flight is still missing. Special bullet types that appear at a
source/target location are rendered at that location rather than given a
fabricated flight path. These snapshot paths are not native projectile
simulation.

Bunker attacks remain occupant-driven. The bank does not synthesize a Bunker
weapon profile from a nonexistent independent Bunker firing script; the
existing occupant/fallback route remains responsible for that case. Carrier
production, launch, Interceptor flight and Carrier combat are separate
unfinished work; see [Carrier projectile development](CARRIER-PROJECTILES.md).

The pipeline imports visual data only. It does not add attack eligibility,
weapon damage, target rules, splash damage, or combat timing. Existing authored
effects remain the fallback when a source profile is absent or unsupported.

## Verification

`make test-sc64-weapon-source-sampling` runs asset-free sampler, clipping,
runtime-codec and fixture checks. With private art prepared,
`make test-sc64-native-weapon-game` exercises the integrated game renderer and
source-event path. A successful build or fixture establishes only the behavior
those checks cover; it does not establish N64 pixel parity, full script
execution, or native combat behavior.


The final scoped ARM comparison used a P3 Protoss map-1/three-AI input tape,
measuring its last 120 updates after 9000 total updates. The source and earlier
Tank-only builds produced the same simulation hash. Average total cost was
566198 versus 562531 cycles (+0.65%); the source sample averaged two live
source effects and one attack pose (peaks six and three). This is one emulator
sample, not a 30 fps guarantee or physical hardware qualification. No extra
BSS or save fields were added.
