# Z7 local collision steering-list probe

`SC64_COLLISION_STEER_LIST` is an opt-in Z7 movement probe. When a ground
mover's direct step is blocked, it builds a bounded list of nearby collidable
actors once, then uses that list to evaluate the eight local steering offsets.
The existing `unit_step_clear()` remains the fallback when a worker is mining
or gathering gas, the list exceeds its capacity, or setup cannot be trusted.
The list's geometry and answer are checked against the original step query in
the host fixture.

The Z7 implementation keeps the 48-entry candidate list and count in a
caller-owned context on the blocked-steering path. It no longer adds that
storage to BSS. The context is 388 bytes (48 eight-byte entries plus a
32-bit count), enforced by a compile-time assertion. The context lives in a
noinline helper so ordinary `move_toward()` calls do not reserve it. For
`SC64_Z7_RUNTIME` ARM builds, the builder is `noinline` in ROM; other profiles
keep the existing `GBA_HOT` placement and global-list API. The collision
step-check helper retains its existing hot annotation; in the final standalone
link the compiler inlines it into the ROM steering helper. No save fields or
serialized data change.

## Measured late continuation

A private ARM replay compared the baseline with only
`SC64_COLLISION_CLASS_CACHE` and `SC64_COLLISION_STEER_LIST` added, using a
ROM-placed builder. It loaded the same input-generated Z7 save at frame 89,340
with `started=250`, then ran 2,000 idle ticks through the normal HOME/L load
path. Both runs matched 20/20 explicit digests through frame 91,340, with no
fault, win, or loss.

| Candidate replay | Update | Draw | Full |
| --- | ---: | ---: | ---: |
| Baseline | 7,820,685 | 436,689 | 8,257,375 |
| Steering list | 7,143,719 | 437,938 | 7,581,657 |
| Change | -8.66% | +1,249 cycles | -8.18% |

This was an emulator cycle measurement for one idle continuation, not a frame
rate or hardware measurement. The benchmark predates the caller-owned stack
context now in the tracked implementation. Its static candidate array failed
the standalone product BSS guard, so these means are evidence for the list
algorithm and placement direction, not measured performance for the final
storage layout. The final context version is separately measured below; the earlier figures
must not be substituted for its performance.

## Fit evidence and rejected variants

The standalone ROM with the static steering list plus the 240-byte collision
class cache ended BSS at `0x0203e178`, 376 bytes beyond the `0x0203e000` bound.
Moving the list into the 388-byte blocked-path context removes that BSS
allocation. The baseline standalone ROM had only 256 bytes of BSS margin.
The final standalone stack-context product links with BSS ending at
`0x0203dff0` (16-byte margin), IWRAM ending at `0x03004c60` (928-byte margin),
and ROM load ending at `0x09ead628`. Its 32 MiB ROM passes the header/SRAM
signature verifier as title `SC Z7 STEP36`, code `SCZS`. The guards were not
relaxed. The earlier replay-harness link had more headroom than the product
and did not establish standalone fit.

The ROM-placed builder replay ended IWRAM at `0x03004f08`, 248 bytes below the
`0x03005000` guard, and ROM load data ended at `0x09eaf580`, below the 32 MiB
limit. Those are replay-harness measurements. The stack context adds 388 bytes
plus the helper's compiler frame to the blocked path; its measured stack
headroom is recorded below.

Two other late replay candidates did not improve the baseline:

- `SC64_ROUTE_L_FAST_PATH`: 8,258,217 full cycles versus 8,257,375 baseline (+0.0102%).
- Partial route flood: 8,295,557 full cycles, slower than baseline.

Neither is included in this probe.

## Host fixture and scope

`tests/sc64_z7_steer_game.c` starts canonical Z7 and compares list answers with
`unit_step_clear()` for live Z7 actors and a dense mixed Zergling/Hydralisk
world. It exercises the gathering-worker fallback, list-capacity fallback,
collision-class invalidation on slot reuse, and save/load followed by cache
rebuild. The fixture is run with ASan and UBSan.

The probe does not change actor rosters, movement cadence, collision geometry,
or save format. The host differential fixture alone does not establish ARM stack safety,
product fit or timing. Paired P7 reference/candidate sanitizer runs also
retain identical neutral-opening and attack-move outputs after the shared
header change. These tests do not qualify a completed mission or hardware.

Private measurement artifacts are retained under
`.local/build/z7-live-20261027/late-steering-rom-builder/`; the original IWRAM rejection is under
`.local/build/z7-live-20261027/late-steering-list/` and the actual standalone
static-list BSS rejection is under
`.local/build/z7-live-20261027/steering-public-final/failed-global-list/`.

## Final caller-owned context ARM check

The final implementation was rebuilt into the actual standalone product and
into a separate immutable replay harness. The harness loaded the same genuine
late save through HOME/L and matched all 20 explicit game-state digests over
2,000 idle ticks, ending at frame 91,340 with no fault, win or loss.

| Final stack-context replay | Update | Draw | Full |
| --- | ---: | ---: | ---: |
| Baseline | 7,820,685 | 436,689 | 8,257,375 |
| Candidate | 7,188,545 | 435,796 | 7,624,342 |

Full measured cost was 633,033 cycles lower (about 7.67%). This is one measured
continuation; it remains far over the 559,240-cycle 30 Hz budget. It does not
establish complete-mission or physical-hardware performance.

A private stack canary was painted after GBA initialization and before game
initialization/normal Load, from `0x03005000` to 512 bytes below the then-current
stack pointer. Through Load and the 2,000-tick continuation, the lowest changed
canary word was `0x03007858`: 1,448 bytes below the initial system SP
`0x03007e00`, leaving 10,328 bytes above the guarded stack boundary. This
measures the observed system-stack depth for those paths; it is not a bound
for all menus, battle states, interrupt stacks or hardware behavior.

Final product, replay sources/commands/hashes, host output and canary evidence
are retained privately under
`.local/build/z7-live-20261027/steering-public-final/`. The probe is opt-in;
regular race ROMs remain unchanged and Z7 is not campaign-qualified.
