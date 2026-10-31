# P8 Reaver and shared Scarab readiness

This source-only audit is against experimental P8 parent
`bdd306b90676347d76f43cb32f64735c2ec843ad`. It does not enable a feature,
change a save format, run a gameplay test, or promote P8. The objective is a
shared Reaver service that works in campaign, custom games, and Showcase; a
P8-only implementation or a campaign-objective predicate is not sufficient.
The earlier P8 normal-ROM win and fresh-process continuation at `71e7714`
used ROM SHA-256 `54cfbe2da8169767424cc71172b1b6319d7be508fb2c58dec62225f7f734cf68`.
Those results are not a replay of this parent. See the
[P8 condition audit](../2026-10-31-p8-condition-scan/README.md) for its
bounded comparison and [P8 candidate audit](../2026-10-29-p8-campaign/README.md)
for mission and service limits.

## Concrete admission gap

P8's authenticated PUNI input has the SHA-256 recorded in the P8 candidate
audit. Reading its already generated local effective bitmap with the bit
index in [`sc64_unit_availability.h`](../../../starcraft/sc64_unit_availability.h)
shows raw 83, Reaver, available for source slots 0 through 7. This includes
human slot 7 and AI slot 3. The generated header and source data remain private
and are not part of this audit. PUNI is an availability fact; it does not by
itself prove a player's current prerequisites or menu state. The P8 authored
AI explicitly requests raw 83 in both Conclave-town policies in
[`sc64_p8_ai.h`](../../../starcraft/sc64_p8_ai.h).

The normal P8 vector in [`Makefile`](../../../Makefile) does not enable the
experimental Scarab runtime, weapon, or human-control probes. Raw 83 is
mapped to `U_REAVER`, whose generic ground attack falls back to a 100-damage
Reaver approximation in [`game.c`](../../../starcraft/game.c) and
[`race_rules.h`](../../../starcraft/race_rules.h). This is not the paid Scarab
ammunition, flight, and delayed-impact lifecycle. The custom-game product gate
in [`sc64_skirmish_adapter.h`](../../../starcraft/sc64_skirmish_adapter.h)
explicitly rejects raw 83 while that service is incomplete. Hiding a
source-permitted campaign product or treating one P8 controller win as proof
of a complete Reaver is not a closure for this gap.

## Existing components and their boundaries

The disabled shared foundation merged as `652ff63` (PR #191), and gated human
controls merged as `d44891d` (PR #203). They contain useful identity,
visibility, parent/child graph checks, paid production, STOP/death hooks and
experimental motion/impact code. The [human-control audit](../2026-10-31-scarab-human-control/README.md)
records a 15-mineral queue, five-slot unupgraded capacity, and a 105-source-tick
training sequence with separate allocation and completion. It is a gated host
fixture, not product admission. Existing control paths pass `upgraded=0`;
owner-specific capacity upgrades and AI ammunition purchases remain to be
implemented and persisted if supported.

These are distinct from open drafts PR #210 (shared Carrier/Defiler) and
PR #212 (Carrier production, flight, and saves). The source-only
`codex/scarab-flight-save-bridge-20261031` component at
`6c91311a3cab75e605d9261cd4f4d0d3b1133181` explores ranked outside-child
wire fields across codecs; it has no production call site. The historical
`codex/scarab-human-current-fit-20261031` head
`eec407f9a318ace8d6de552ca7184e163e8e39b2` and packet-pool head
`b6c332db2dc7e780bd4d1466e5db191f08feb9e9` describe gated fits and tests,
not current P8 qualification. See the [flight bridge](../2026-10-31-scarab-flight-bridge/README.md),
[human fit](../2026-10-31-scarab-human-current-fit/README.md), and
[shared packet audit](../2026-10-31-scarab-shared-packets/README.md).

The current experimental gate chain in `game.c` requires namespace and weapon
probes for human control, autonomous and packed motion for weapon handling,
and the BC energy experiment for source cadence. The namespace stage in
[`race_save_adapter.h`](../../../starcraft/race_save_adapter.h) is restricted
to imported custom-game geometries. It is not a P8 campaign loader. The
experimental writer refuses live outside children and pending Scarab impact
packets. Dead-row normalization addresses a separate post-impact case; it does
not make an active-flight or pending-impact save complete. Existing bounded
motion and arrival evidence is not a general terrain pathing or native-order
proof.

## Integration constraints

- **Clock and pause.** The experimental Scarab tick calls
  `sc64_battlecruiser_source_tick_due()` in `game.c`. That predicate uses a P7
  service phase when P7 is active, a BC pause offset in one experimental
  configuration, and frame modulo otherwise. P8's authored source pause can
  advance its trigger clock while combat and production are frozen. A shared
  24-to-30-Hz service cadence needs an explicit active-world phase and
  pause/load contract. Neither P7 mission bytes nor the BC campaign experiment
  are a universal clock. P8's separate 42-ms timed service and eight-update
  trigger pass are authored; do not change them merely to activate Scarabs.
- **Owner and wallet.** `race_scarab_owner_bank()` in
  [`sc64_scarab_unit_adapter.h`](../../../starcraft/sc64_scarab_unit_adapter.h)
  keys only the engine owner. P8 binds separate source-slot 0 and 3 wallets in
  [`sc64_p8_runtime_adapter.h`](../../../starcraft/sc64_p8_runtime_adapter.h),
  although both can share an enemy combat owner. A generic service needs a
  provenance-aware wallet interface supplied by each mode, with atomic payment
  and exactly-once cancellation or transfer handling. Do not debit the shared
  enemy bank for P8 source-local AI work.
- **World lifecycle.** Training and inside children must be excluded from
  targeting, collision, visibility, selection, rendering, supply and victory
  census. Outside children need bounded movement, arrival, damage and cleanup.
  Handle STOP, parent/child death, ownership changes, transport, scenario
  removal and slot reuse. When the shared service is active, its attack must
  replace the generic Reaver damage path, not add a second hit.
- **Save budget and identity.** P8's persistent payload already holds mission
  runtime, wallets and the shared Protoss service clock in
  [`sc64_p8_state.h`](../../../starcraft/sc64_p8_state.h); those bytes are not
  free common storage. Its historical record is 16,292 bytes against the
  16,368-byte payload limit. The small remaining envelope is shared with the
  variable impact tail and any other enabled extensions; impact rows are six
  bytes in [`sc64_impact_save_types.h`](../../../starcraft/sc64_impact_save_types.h).
  Check actual final profile sizes rather than assuming that the whole
  difference is available. Validate parent/child identity, ownership, phases,
  queues, timers, references and every candidate row before committing live
  state. Cover the P7 typed, Z/P compact, Terran compact and supported legacy
  paths, including final live restore.
- **Version policy.** A distinct versioned product/save namespace with a
  separate save basename and no automatic import is one supported option.
  Explicitly reject incompatible historical records rather than interpreting
  their bytes as new parent or child fields. Coordinate any Scarab envelope
  with the separately staged shared-transport namespace and existing
  impact/BC/Yamato tails. Do not silently fall back across incompatible
  representations. This audit does not prescribe an old-reader fence or
  authorize malformed guest-SRAM experiments.
- **Assets and limits.** Raw 85 source body/facing/walk/death and command art
  need authenticated, private per-product closures; regenerate corresponding
  P7/P8 fact tables for the actual roster. No ROMs, generated headers or
  captures are committed. The codec uses a six-bit unit type: verify the
  final enum and each codec explicitly. Historical gated links in the audits
  are not current fit proofs. One cross-codec Showcase probe ended at
  `0x03004ff0`, only 16 bytes below its `0x03005000` IWRAM guard; P8 also
  carries an 800-byte condition index. Recheck every final product's EWRAM,
  IWRAM, bounded queues, CPU and selected art without weakening guards.

## Smallest safe sequence

1. Extract and test a common active-world source cadence and wallet interface
   without turning on Reaver admission or altering mission triggers. Preserve
   each existing profile's pause behavior and use no mission-specific save
   bytes as global state. Asset-free tests can establish pure clock and wallet
   behavior; profile behavior and saves still need their own checks.
2. Bring the already-authored flight bridge into a pure, namespace-neutral
   codec test surface, then design explicit versioned whole-record handling.
   Prove active-flight and pending-impact representation, capacity and full
   candidate preflight before installing it in any live save path. The pure
   bridge alone is not a migration or a gameplay feature.
3. Complete bounded owner-aware production, attack and lifecycle in shared
   engine code, including P8 AI stocking and ordinary human controls. Integrate
   authenticated source art and facts. Keep admission disabled until normal
   gameplay, cleanup and valid save/load states are representable.
4. Qualify the actual final common capability across P8, the retained P1–P7
   campaigns, both custom maps/all human races/independent AI owners, and
   Showcase. Exercise ordinary valid saves during training, inside, flight,
   pending damage, STOP, loss and load; validate source PUNI and owner wallets.
   Run normal-input current-ROM controller and fresh-process continuation
   separately with pinned inputs and existing memory guards. Do not substitute
   optional forged-guest-SRAM probes for ordinary-play evidence.

This is a staged engineering path, not a claim of source-perfect AI or physical
GBA performance. Remaining chapters and the near-30-fps goal remain open.
