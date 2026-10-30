# Scarab source-art prerequisite

The USA source tables identify the Scarab actor independently of the Reaver's
weapon visual: unit 85 → flingy 48 → sprite 200 → image 147 → GRP index 153
(BOLT `00D/098`). Image 147 uses IScript 160, has directional frames and uses
normal drawing modifier 0. Its 17-frame GRP has a 10×10 canvas; Init selects
frame 0 and has no initial image layers. The source script also names Walking,
WalkingToIdle, SpecialState1 and Death animations. The source GRP SHA-1 is
`92a244acfa924a517a177ef74035ae3bdc8643c3`.

Adding raw 85 to the race sprite importer also makes the directional importer
select it. The shared sprite header exposes `sc64_race_unit_sprite(85)`; the
facing header exposes 17 source frames and the existing mirrored 32-direction
word metadata. This adds art identity only. It does not create a Scarab actor,
production, movement, damage, save support or a playable Reaver service.

For an entitled user with the pinned extractor and an authenticated USA ROM,
the following prepares an isolated base-only audit bank; it does not replace
any existing prepared product inputs:

```sh
python3 tools/sc64_race_sprites.py --rom "$ROM" --remap "$REMAP" \
  --out .local/scarab-art/sprites \
  --header .local/scarab-art/sc64_race_sprites_generated.h --base-only
python3 tools/sc64_unit_facing.py --rom "$ROM" --remap "$REMAP" \
  --out .local/scarab-art/facing
```

Here `ROM` is the verified user-supplied USA ROM and `REMAP` is the existing
256-byte Backwater palette remap. Generated files and previews remain private.
The base-only bank is not a replacement for the per-product prepared banks;
those must retain their own complete feature groups when updated.

Local verification used the pinned ROM and remap to generate new, ignored
outputs. All 17 exported indexed frames and 128 mirrored/phase word arrays
matched the decoded source frame and the existing packer. The prior base header
matched byte for byte after removing only the new Scarab data and lookup.
Existing prepared race, Zerg and Protoss bank hashes were unchanged. Asset-free
DAT-chain, malformed-chain, lookup and directional-contract tests cover the
tracked exporter change; no private pixels or generated header are tracked.
