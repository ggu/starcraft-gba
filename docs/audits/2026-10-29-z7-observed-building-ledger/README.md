# Z7 observed-building ledger controller checkpoint

Status: source-only controller checkpoint for PR #123. This does not enable
Z7 in the default catalog or change the campaign/runtime rules.

## Controller change

The host-side Z7 controller keeps up to 64 enemy-building observations keyed
by world coordinates. After every controller `step` calls `game_update`, it
records an enemy building's coordinates, raw type, owner, and last-seen frame
only when the building center is currently visible. It retains an entry in
fog and clears it only when its recorded location is visible and no hostile
building is observed at that coordinate. Entries do not retain source slot
identity or hidden hit points; a remembered coordinate is only an Attack-Move
candidate, not evidence that a building still exists.

When the late-air scout policy has no higher-priority native order to issue,
it can gather a bounded group of four to eight idle own Mutalisks using normal
selection, Move, box-selection, and Attack-Move inputs. It preserves workers
and units with active orders. The rally is selected from currently visible
map cells, has a finite arrival bound, and the final order is accepted only
when every selected group member reports the expected Attack-Move near the
remembered coordinate. Candidate attempts use finite cooldowns. This change
does not change attack range, weapon damage, movement rules, game state, or
source objectives.

The new ledger and its candidate picker use remembered visible observations
and own-unit state. This does not certify every inherited controller query:
other global path/placement predicates still need a coherent
player-observability audit before calling the full controller hidden-state
clean.

## No-force host result

The controller snapshot was compiled against the current human-fog runtime and
run with ordinary inputs using `--policy baseline --limit 1000000`. The winning
vector has `SC64_DIRECT_FIRING_POINTS` on and `SC64_ATTACK_RANGE_ROUTES` off.
The Z7 canonical flags now include that same direct-firing helper. No forced
phase was applied. It reached the native RESULT win before the host-input limit:

This recorded execution used a direct host compiler invocation against the
tracked controller and runtime sources, captured absolute immutable prepared
generated-header inputs, and the matching feature defines. The ordered `-D`
vector matches the current Make-expanded `ZERG_Z7_CANONICAL_FLAGS` (apart from
the host-only `-DGBA_HOST` define); generated include roots are path-rebased to
the captured prepared inputs. The invocation, `-H` header provenance, source
hashes, and executed binary are preserved in the ignored evidence directory
listed below. This is not a literal Make-built ROM or a full normal-product
build qualification.

| Checkpoint | Result |
| --- | --- |
| Host ticks / game frame | 821,565 / 808,493 |
| Recorded held/pressed transitions | 70,561 |
| Screen / win / loss | RESULT / 1 / 0 |
| Native Z7 model outcome | `SC64_Z7_WON` |
| Started / fired actions | 254 / 254 |
| Runtime fault | 0 |
| Enemy buildings at terminal | 0 |
| Own units / buildings / workers | 89 / 8 / 12 |

The run accepted eight last-seen-building group orders. Every group member
reported the expected native Attack-Move:

| Remembered coordinate | Last observed raw type | Group issued | Game frame |
| --- | ---: | ---: | ---: |
| (464, 144) | 143 | 4 / 4 | 694,323 |
| (544, 64) | 149 | 4 / 4 | 703,077 |
| (1280, 160) | 144 | 4 / 4 | 714,473 |
| (1872, 304) | 146 | 5 / 5 | 719,982 |
| (128, 704) | 146 | 5 / 5 | 731,355 |
| (880, 384) | 144 | 8 / 8 | 802,975 |
| (80, 768) | 144 | 7 / 7 | 805,168 |
| (1056, 176) | 144 | 6 / 6 | 807,721 |

A separate seven-Mutalisk escort order followed an observed loss of the own
scout; it is not counted as a building-ledger order. The run used no passive
terminal diagnostic coordinates as controller input and did not inject game
state. It is evidence for this controller/runtime snapshot, not a whole-game
hidden-information audit, ARM qualification, or hardware performance result.

Frozen run evidence (ignored local artifacts):

| Item | SHA-256 |
| --- | --- |
| Controller source snapshot | `07ddda19e9bb6af2a88e3be6cdbd0e33f3cdf42d89ff4e8174842dce77faa3c5` |
| Human-fog `starcraft/game.c` | `3b13ccffe90f3e1dc1130edadc63a62d0fece239f897261d8b96d82f855746de` |
| Executed host binary | `a58220bb0007884b6b1e1eee3fc7548c6cd5323aa14919e7bf25a33b2fa54876` |
| Ordinary-input trace | `2d28b63d74ac4438630561b714444ba77a12a9e79e9b4062133c928c447b8198` |

The private replay is under `.local/build/direct-corridors-622-z7-muta-scout-private/v7-last-seen-buildings/normal-v1/` and is not part of the repository.

## Focused controller checks and remaining qualification

The focused ASan/UBSan fixture passed ledger fog retention, visible-empty
clearing, same-coordinate morph refresh, slot reuse, hidden-position
invariance, cooldown reset, coordinate-key post-order bookkeeping, native
four- and eight-Mutalisk orders, a valid group without four units in a
64-pixel cluster, an isolated first unit, worker/active-order preservation,
and repeat suppression. This fixture is separate from the normal campaign
win.

The public Z7 profile remains a candidate. Before enabling Z7 or making a
strict full-controller observability claim, review the remaining global
path/placement oracles and qualify the public build/test flags against the
exact tracked source. No assets, imported data, ROMs, saves, private traces,
or generated files are included in this checkpoint.

An additional ordinary-input run of the same tracked source and runtime with
`SC64_DIRECT_FIRING_POINTS` omitted ended at the 1,000,000 host-input limit
with 18 enemy buildings remaining (`model=0`, `started=250`, `fired=250`).
Its trace SHA-256 is `23e9b5f1950aea91b051441d0bf2d3215ade59b5329ded6f043f95b66faef552`.
This is evidence that the direct-firing helper is material to the recorded
win; it is not a win qualification for the DIRECT-off build. The helper is
enabled only in the unpromoted Z7 canonical candidate flags here; this does
not enable Z7 in the default catalog or qualify a shipped product.
