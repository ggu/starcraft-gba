# Interceptor order decisions

`starcraft/sc64_interceptor_orders.h` provides an asset-free decision model for
the Interceptor's attack passes and return to its Carrier. Its only gameplay
caller is the default-off generation-43 flight adapter
(`starcraft/sc64_p7_interceptor_flight.h`; see
[Carrier projectiles](CARRIER-PROJECTILES.md#generation-43-interceptor-flight)).
It does not qualify another campaign mission.

The module is excluded from `STARCRAFT` runtime sources until that adapter is
ready. Its isolated host target links the implementation explicitly.

Run the authored C regression suite with:

```sh
make test-sc64-interceptor-orders
```

The check also runs with `./dev test`. It requires no game assets, reference
checkout or emulator, and uses AddressSanitizer and UndefinedBehaviorSanitizer.

## Adapter contract

Call the decision function on the existing 24 Hz source-order clock. The caller
owns timer progression, target acquisition, geometric range checks, movement,
RNG, animation, visibility, hangar lists and damage. A decision returns movement
destinations and requested effects; it does not execute those effects.

The launch phase requests the wide initial approach and a 15-source-tick timer.
After the ascent wait, the attack phase requests a weapon attempt and steers
through the narrower passes. A weapon attempt is not a shot: the future adapter
must preserve attack animation and IScript event timing before calling the
existing projectile firing API. The weapon table's cooldown of one must not
become one shot per update.

Shield retreat uses a strict quarter-shield threshold in Q8 units. Missing
targets, failed reacquisition and the Carrier's leash can also order a return.
Return decisions lower the Interceptor near its parent and request hiding and
hangar relinking at the arrival boundary. The adapter must commit the list and
visibility changes together, including slot-reuse and parent-death handling.

Keep the module disconnected until those effects are implemented and tested
through ordinary game input, save/load and ARM execution. No persistent memory,
save format, actor capacity, ROM identity or gameplay feature flag is changed
by this foundation.

## Reference and scope

Behavior was researched in the pinned OpenBW revision
[`4b046d5f65302b10cb0a745f0fecd37ec85b20a8`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h),
particularly `order_InterceptorAttack` and `order_InterceptorReturn` around
lines 7262–7378, and movement-target restriction around line 763. The module is
independently authored GPL-3.0-or-later C; no reference implementation, script
or game data is included.

The input predicates are an explicit boundary. Host tests of this module do
not establish that the game's current geometry, target acquisition, RNG stream
or timer scheduler produces native-equivalent inputs. Actual aircraft movement,
Carrier launch selection and cadence, weapon events, hangar integration and
campaign playthrough qualification remain separate work.

## Validation on 2026-10-25

The isolated C suite passes with ASan/UBSan. It covers the launch/ascent/attack
pass transitions, requested firing, Q8 retreat, reacquisition, leash, parent
loss, reflection versus final bounds, return/relink requests and transactional
rejection of invalid inputs. The standard `./dev test` also passes, including
199 Python checks and the sanitized host suites. `./dev build` passes the
unchanged ARM guards. The default ROM is byte-identical to the same-source
baseline without this module (`0dc02375af72f873dd8179208278ed05e21676e2fac05bf02f5733d3fe2b34ef`).

A private synthetic fixture directly calls the pinned OpenBW attack/return
methods and compares them with this model in ten ASan-only cases: low
shields, one Q8 unit below/exactly at/one Q8 unit above quarter shields,
both sides of the parent leash, ascent wait expiry, and return movement at 61, 60 and 11
source pixels. Those comparisons pass. The native 10-pixel hide/relink fixture
could not complete because its synthetic world lacks the visibility and
unit-finder registration required by `hide_unit`; the 10-pixel decision is
covered only by the authored C suite. Native weapon execution, acquisition,
random approach passes and actual hangar relinking are not qualified by these
ten reference comparisons. The fixture, reference checkout and generated
executables stay ignored locally.
