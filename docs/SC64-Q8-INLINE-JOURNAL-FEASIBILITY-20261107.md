# Q8 HP fractions in existing journal rows — source audit

This is a design audit, not an implemented codec or a proof that all lossless
in-row encodings are impossible. The normal SRAM format and actor capacities
are unchanged. The experimental Q8 combat stage currently refuses saves and
loads; it must continue to do so until a versioned implementation and its
migration, rollback, and resource tests pass.

The retained production-six build at `10bda18e` supplies the product vectors
and ARM layout receipts cited in
[the payload audit](SC64-JOURNAL-DEBT-CAPACITY-AUDIT-20261107.md). The Q8
lifecycle and journal source examined here includes the `904f7698` checkpoint;
new experimental actor stages may introduce further types or wire tags. The
previous typed-unit and phase prototypes are at `f2387939` and `c3312280` on
`codex/research-q8-wire-proof-20261107`; they are not integrated journal code.

## Actual row budget

| Products | Units | Unit row / used bits | Buildings | Building row / reserved high bits |
| --- | ---: | --- | ---: | --- |
| Terran | 192 | 38 bytes / 303 bits; one validated-zero bit | 96 | 28 bytes / 7 bits |
| Z7, Z9, Z10 | 240 | 35 bytes / all 280 bits | 160 | 28 bytes / 7 bits |
| P10, Showcase | 240 | 28 bytes / all 224 bits | 160 | 25 bytes / 5 bits |

`starcraft/sc64_compact_unit_save.h`, `sc64_zp_unit_save.h`,
`sc64_p7_typed_unit_save.h`, `sc64_compact_building_save.h`, and
`sc64_p7_typed_building_save.h` define the exact fields, zero-row special
cases, sentinel bits, and checks. The old decoders validate their currently
reserved bits as zero where applicable. `game.c` sets the unit/building pool
counts; `sc64_race_save_types.h` selects the row types. Product flags were
read from the six retained link receipts rather than inferred from product
names.

The experimental research codec uses 96 of the building padding bits for six
2-bit levels for each of eight owners; custom mode additionally enforces its
five-owner limit. Unused bits remain checked. Merely appending eight HP-debt
bits for each unit and building to these spare bits falls short even before
considering aliases: Terran has 768 bits left for 2,304 bits of hypothetical
independent debt, the Z profiles 1,024 for 3,200, and typed products 704 for
3,200. Eligible Terran building burn already has an HP fraction in an
existing field, so this independent-debt accounting is an intentionally
conservative direct-placement calculation, not a reachable-state entropy
proof. A profile-sized 288-byte Terran tail fits its worst-case record by eight
bytes; see the payload audit. A 400-byte fixed tail does not fit the worst-case
Z7/Z9/Z10/P10 records.

## Bounded codec candidates and limits

For a live P10 typed unit, canonical type reconstructs `maxhp`; the outer
journal validator requires `0 < hp <= maxhp`. The earlier actual P10 HOST
census at `f2387939` found 53 admitted types, maximum canonical HP 850.
The prototype keeps all other row bits unchanged except the existing
18-bit HP field, uses the alive bit to distinguish live Q8 HP/debt from the
old biased signed dead-row HP, and exercises all 256 debts at each type's
maximum plus a negative dead-HP row. This is a plausible starting point for
P10; it is not yet a journal integration, all-HP-value proof, or a new census
for every current profile or actor-stage flag. The 18-bit field can represent
10 bits of displayed HP and 8 bits of debt for canonical max HP <= 1023.

For compact and ZP units, the historical 18-bit signed HP and 17-bit max HP
are independently retained; the general live outer validation requires
`1 <= hp <= maxhp <= 131071`, rather than reconstructing max HP from type.
For that entire rectangularly described live domain, rank `(maxhp, hp_q8)`
in `sum_{m=1..131071}(256*m) = 128*131071*131072` states: it fits 41 bits,
so it needs six more than the existing 35 HP/max-HP bits. ZP unit rows have
no reserved bit; Terran compact rows have one. A dead row would need an
alive-discriminated old signed-HP/max-HP class and exact old data retention;
the all-zero sentinel must remain unambiguous. Treating type's usual max HP as
universal would narrow previously accepted max-HP states unless every old
record is handled losslessly by an escape path with a worst-case bound.

The same joint rank is a possible compact-building approach: the
existing 18+17 HP/max-HP bits plus seven padding bits give 42 bits; a live
41-bit rank would leave one bit per row. On a 160-row Z profile, 96 such bits
can retain research, leaving 64 bits for a hypothetical global unit extension.
A straightforward fixed six-extra-bits-per-unit scheme needs 1,440 bits, so
this specific combined scheme is still short 1,376 bits (172 bytes) before
any additional markers or escapes. This is not a lower bound over all possible
cross-field codecs. It also requires proof for legacy special beacons and
100,000-HP props, dead rows, live HP and `maxhp` bounds, and exact original
queue/flags/shield state; a row validator alone is not the outer journal
validator.

Typed buildings have a different possibility: their type reconstructs max HP.
Their 7-bit type, 18-bit HP and five padding bits total 30 bits. Reserving one
of these per row gives 160 bits, enough for the 96 research bits; the other
29 bits could rank `(type, hp_q8)` if the sum of `maxhp` across **every**
admitted type is at most 2,097,152. An alive bit can select a distinct dead
class that retains the old signed HP/type. This is a criterion, not an
established fit: the exact current-profile census must include special
100,000-HP canonical types, all legal dead rows and any sentinel exceptions.
The building's independent 17-bit shield field cannot be narrowed to the
usual canonical shield maximum: the existing general candidate validator does
not universally enforce that bound. No five-cell queue, inactive slot, or
transient-looking field is presumed disposable; research jobs and other
services already alias them.

## Migration and integration boundaries

A future new tag must be globally inventoried and selected before interpreting
row bits. All existing 0x43/0x44/0x45 readers, and the separate experimental
campaign 0x47 path where combined, must retain their existing meanings and
fallback; checksums establish consistency, not payment authenticity. The
current prepared selector accepts at most five tags
(`GBA_SAVE_MAX_ACCEPTED_TAGS` in `common/gba.h`), so adding another format to
that combined accepted set requires a separately reviewed selector change or a
proved equivalent selection architecture, not silently dropping a predecessor.
Normalize only detached selected candidates, keep original SRAM and live
world untouched until validation and revalidation succeed, and use row-local
scratch rather than assuming another full 16 KiB buffer fits. Preserve all
saved impact/transport/Yamato combinations and guard/cap values.

Old-format migration is context-sensitive. Eligible Terran buildings already
store burn HP debt in the low byte of `shield_remainder`, alongside the
source-owner bits; do not set that debt to zero. Shielded actors' old ordinary
phase uses denominator 320 whereas Battery-active state already uses 1280;
other raw types use the same field for Z5 regeneration, traps, energy, or
other state. The Q8 Z5 experiment changes its old 0..79 cadence alias to a
0..4 residue, so migration must preserve future healing timing without
inventing or discarding health. Late-Terran repair preflight currently tests
integer HP against max; a valid fractional-only repair in a new format needs
debt-aware detached validation. Dead-slot cleanup and every in-place morph,
full clamp, unit reuse and damage path must be established before assuming
unrepresented debt is zero.

The smallest next source experiment is an opt-in new-version **real typed row**
codec plus independent rank/unrank and maximum-domain census, followed by
separate-process real P10 writer/reader, 0x43/0x44/0x45/0x47 predecessor and
malformed-newer fallback tests, then profile-specific RAM and ARM guards.
In parallel, the compact/ZP path needs a lossless bounded solution for the
remaining worst-case bits, not just a common-case compression measurement. If
that cannot preserve every accepted state, the standalone Flash1M transport
and commit-sector journal (`e02e4ce9`, `common/gba_flash1m.*` and
`common/gba_flash_journal.h`) offer a larger-slot research direction. Actual
game integration would need a feature-gated transport/selector bridge from
`common/gba.c` and `race_save_adapter.h`, bounded streaming into the existing
candidate scratch, new-tag serialization and revalidation, and a proven
old-SRAM migration strategy. The standalone model is not evidence that an
EverDrive automatically migrates save types or that real hardware has passed
Flash timing, geometry, reset, or power-cut qualification.

No build, emulator or source mutation was performed for this audit; it is a
static inventory and a bounded implementation plan, not an enablement claim.
