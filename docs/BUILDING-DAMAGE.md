# Building damage and burning

Every `SC64_RACES` engine product uses the same building burn lifecycle. The
common race renderer imports orange fire, blue fire and bleeding damage
images from the verified local N64 ROM. No game data is distributed here.

## Mechanics

Completed Terran buildings burn when their displayed health percentage,
rounded down, is at most 33. This is not exactly one third: 50/150 burns,
51/150 does not. Each source update removes 20/256 HP. Four source updates
per five 30 Hz GBA updates preserve the reference rate of 1.875 HP per second.
Lifted buildings keep burning. Construction, Zerg/Protoss buildings and
special non-Terran objects do not use this damage path. Burn bypasses armor,
shields and combat invincibility, matching the inspected PC reference path.
These mechanics come from pinned OpenBW behavioral research; the native N64
burn handler has not been traced. No reference implementation code is copied.

Burn destruction shares weapon destruction cleanup: supply, path blocking,
mission notifications, Battery/Nydus references and alive state. The last
weapon-damage owner is retained for existing GBA mission death bookkeeping;
neutral is used when none is known. Exact N64 score attribution is unverified.

Terran buildings have no shields, so their existing serialized
`shield_remainder` word stores a fractional HP deficit (low byte) and last
weapon owner (upper bits, 0–4). Values remain within the existing codec limit
0–1279. This excludes special objects, including the P6 missile trap whose
word is a cooldown. Actor size and static RAM do not grow. Whole HP remains
the ceiling of fractional health, preserving the displayed burn threshold.
A new marker in unused save state word 25 distinguishes the encoding. Older
completed worlds initialize it with zero deficit and neutral credit. Unknown
markers reject before applying state. The construction-compatibility policy
in [SHARED-ENGINE.md](SHARED-ENGINE.md) still applies to unfinished old saves.
Both Terran repair adapters clear the fractional deficit upon full repair.
Finishing fraction-only damage uses one existing HP work/payment step; this
is a documented approximation to source fractional repair accounting.

## Presentation

The importer authenticates image-to-GRP association, IScript frame timing and
frame-zero LO damage offsets from the local ROM. Small damage overlays
progress to large ones using the source count of attachment slots and health
stages. Orange and blue loops have 12 frames; bleeding loops have six, each
at two source updates per frame. The same bank and renderer serve every race
product. Visual damage is separate from Terran burn damage: Protoss and Zerg
can show their damage effects without taking Terran burn damage.

GBA palette ramps approximate source translucent blending. Slot activation
follows deterministic LO order, rather than persisting the source engine's
random allocation/repair history. Attachment offsets use the source's first
pose, so alternate/flying pose offsets are not claimed. Current integer HP
presentation approximates fractional stage boundaries. These limits prevent
an exact N64 visual-equivalence claim.

Prepare the private bank only with an already available verified ROM and
extractor:

```sh
python3 tools/sc64_building_fire_pack.py --rom /absolute/path/to/StarCraft64.z64
```

Its default output is ignored `.local/sc64/building-fire/generated/`.
`RACES_FLAGS` and `RACES_INPUTS` include it for every derived product.

## Verification

Asset-free commands:

```sh
make test-sc64-building-damage test-sc64-building-fire-vfx test-sc64-default-armor
python3 -m unittest discover -s tests -p 'test_sc64_building_fire_pack.py'
```

Prepared-input commands:

```sh
make test-sc64-shared-mechanics test-sc64-firebat-damage
make unified test-unified
```

The shared-mechanics fixture covers campaign/custom modes, all owners,
thresholds, fractional rates and saved state, lifted/construction exclusions,
destruction, both repair adapters where compiled, and invincible shield
protection. Renderer tests use authored synthetic art to check attachment
slots, stages, frame timing, mirrored paths and clipping. Source art remains
private. Normal ARM memory guards, menu replays, hardware timing and full
campaign completion are separate qualification steps; this page does not
claim new 30 fps or physical-GBA results.

## Provenance

- [Pinned OpenBW burn threshold](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1132-L1135)
- [Completed/flying building burn and damage rate](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1160-L1175)
- [Damage-overlay health stages](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L815-L868)

The private importer manifest records ROM, GRP, IScript and LO provenance.
Broader combat differences are tracked in
[ENGINE-FIDELITY-AUDIT.md](ENGINE-FIDELITY-AUDIT.md).
