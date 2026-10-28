# Optional Z7 contact grouping

`SC64_CONTACT_GROUP_INDEX` groups collision candidates with the same type and
position in the existing spatial buckets. The eight-direction blocked mask
can evaluate one eligible member per group; other bucket readers still visit
every actor. Group membership is updated when a slot moves, is reused, or an
egg changes type. Each mask query checks current keys before collapsing a
group, falling back to individual candidates after an unhooked mutation.
Dead, loaded and self candidates keep their original treatment.

The flag is **off in every product**. It currently supports the Z7 contact
profile only, without P6 combat, persistent collision or steering lists.
With already-prepared Z7 inputs, the optional host checks are:

```sh
make test-sc64-z7-contact-group
```

They cover slot birth/reuse, same/cross-cell moves, representative promotion,
death, cargo, morphs, stale keys and world edges, and compare the cached
queries to the reference collision predicates. The baseline and grouped
random runs each pass 3,291,197 step queries, 506,338 active masks, 130,292
inactive masks, 171,024 stock-motion checks and 71,896 mutations. The recorded
matrix contains no strict-step queries; it does not claim that separate scope.

A private ARM pair used runtime source at `83b264c`, the actual expanded Z7
release flags (including the existing contact math table and new controls),
the same audio bank, and the same previously ARM-written generation-36 save.
Only the group flag differed. Over 2,000 ordinary suffix input calls, both
advanced 1,547 simulation frames and matched all six sampled state,
persistence, RGB555 and death-effect records against fresh host goldens.

| Measured cycles | Baseline | Grouped |
| --- | ---: | ---: |
| Update | 4,953,848,365 | 4,275,961,244 |
| Draw, including presentation waits | 1,307,571,838 | 1,320,364,169 |
| Total | 6,261,420,203 | 5,596,325,413 |

Total measured cycles fell **10.62%** in this crowded suffix. This is not a
whole-campaign or hardware frame-rate result. The save stayed unchanged; no
win was expected or claimed. Both painted-stack measurements used 3,640 of
11,424 bytes, leaving 7,784 bytes untouched above the reserved floor. The
private grouped link adds 240 BSS bytes and 568 IWRAM bytes, ending IWRAM at
`0x03004f90` (112 bytes below the unchanged `0x03005000` limit). BSS ends at
`0x0203d844`; both ROMs are 32 MiB. These are instrumented harness bounds,
not a separate production-release qualification.

Before product activation, qualify a full current-controls Z7 win and carried
missions, fresh-process saves, production bounds, and performance outside this
suffix. The new-controls Z7 controller remains under investigation. The
per-query member validation and group maintenance costs are included in the
measurement; duplicate-count estimates alone are not evidence of speed.
No game assets, generated tables, ROMs, saves or captures are committed, and
no GitHub Actions were used.
