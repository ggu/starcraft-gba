# Scarab foundation after the shared graphics memory change

This is qualification of an explicitly gated, unfinished service. Normal
race and Showcase products still do not enable `SC64_SCARAB_RUNTIME`, and
Reaver production must remain unavailable until its full behavior works.

## Training and historical row correction

Training correction `16af0e7` follows the pinned reference's earlier actor
allocation, initial 2 HP, maximum 20 HP, and construction rate of 44 Q8
units per progress call, clamped at maximum. The 105th progress call leaves
the child unfinished with zero time remaining; the next production call
completes it. The adapter reconstructs undamaged Q8 HP from progress and
stores its integer floor. It refuses an unexpected integer HP mutation;
this does not implement fractional damage/healing during construction.

The original 17-bit carry proposal could not survive historical signed-16
row serialization. The replacement keeps low 16 packed bits in signed
`carry` with bias 32768 and stores packed bit 16 in `work_timer` bit 7;
progress 0–105 occupies timer bits 0–6. The decoder validates canonical
repacking before touching its output. This is a new, unshipped field
proposal, not an automatic interpretation of legacy Reaver ammunition.

Field child `68007ac` (same patch as independently reviewed `15b319d`)
passes four capability sets in both current compact and historical Unit
layouts under ASan/UBSan. The fixture extracts the actual historical
save/load loops from `race_save_adapter.h`, checks 7,759,566 accepted parent
and 673,400 child shapes per run, and round-trips representative boundary
rows through the legacy loops and all four compact codecs. All four actual
adapter and four presence fixtures also pass, with 102 prepared input
hashes unchanged. This is row representability, not whole-world persistence,
save namespace/migration, lifecycle wiring or ordinary-menu evidence.

## Merged foundation qualification

Source `52ee7527edbf069ee44ff160de9284462814f444` combines the training
correction with shared graphics merge `5720be9`. A clean indexed worktree
without private inputs passes source boundary, doctor, default build and
test: 782 Python tests with 23 optional skips, eight ancillary tests and C
fixtures. The field-only child above has focused qualification; the broad
suite is scoped to this compiled source, not a later child.

The actual four normal Make ARM recipes also pass with the explicit
Scarab identity/presence gate, authenticated additive sprite/facing
closures, and the matching P7 fact table. All 104 original prepared input
hashes are unchanged; all six selected additive art/facing/fact hashes
match the prior fit attempt. Each product passes unchanged RAM guards,
header verification and 32-MiB size. Independent objcopy comparison checks
the ELF payload outside the fixed boot header, all padding, and embedded
audio bytes. Dependency captures bind the actual compiler/include vectors.

| Gated product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `020370b8` | `03004db8` |
| Zerg Z6 | `02037124` | `03004e38` |
| Protoss P7 | `020382e4` | `03004c80` |
| Showcase | `0203df38` | `03004fc8` |

Showcase previously failed at `03005150`; the shared graphics change frees
392 bytes and now leaves 56 bytes below the unchanged strict `03005000`
boundary. No actor capacity, cadence, reserve or guard is reduced. This is
only the identity/presence foundation: registering flight, damage, save and
UI code still needs another fit check and measured runtime qualification.
No claim is made that the eventual complete service fits this margin.

All 29 actual native C translation-unit dependency passes exclude both
`sc64_scarab_fields.h` and `sc64_scarab_unit_adapter.h`; the field-only child
does not change those compiled products. It is exercised by the host
fixtures rather than silently treated as ARM gameplay coverage.

Private receipts are retained in the `scarab-shape-fit-20261031` worktree's
`.local/shape-fit` and the `scarab-shape-clean-20261031` worktree's
`.local/merged-clean-qualification`. Legacy fixture receipts are in the
`scarab-legacy-fields-20261031` worktree's `.local/scarab-legacy-proof-v1`.
No private inputs, ROMs, saves or captures are committed.

## Remaining work and performance scope

Whole-world candidate validation and a safe version/migration policy,
source-clock/flight/collision/arrival execution, delayed weapon-82 splash,
ownership and death/slot-reuse cleanup, paid UI/AI admission and fresh-process
save continuation remain required. Preserve source script-controlled
invincibility. Pool failure must never manufacture impact damage. A local
script tick is not by itself an absolute arrival-to-damage delay.

This round makes no Scarab performance or hardware claim. The separate
shared graphics production measurement remains +0.04717% cycles in one
instrumented busy scene, with sampled state/pixels matching. Canonical
campaigns remain T10/Z6/P7 (23 of 30), with both custom maps, all human races
and 1–3 mixed-race AIs retained. No Actions or SD writes are performed.
