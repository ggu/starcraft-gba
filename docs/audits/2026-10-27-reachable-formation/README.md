# Experimental reachable formation destinations

`SC64_REACHABLE_FORMATION` is a default-off race-engine experiment. Ordinary
group Move and Attack-Move commands assign compact formation offsets. On the
Z7 terrain, some offsets fall on a statically disconnected destination even
when the clicked point is reachable. Affected units keep retrying that order.

The experiment keeps each original slot unless a ground unit cannot route to
it and can route to the clicked point. Only then does it use the clicked point.
Air units retain the original offsets without ground-route queries. Existing
loaded-actor, immovable-actor and Egg exclusions stay in place. Single-unit
orders and commands whose clicked point is also unreachable keep their existing
destinations. No terrain, combat facts, objectives, AI rules or save fields
change. This is an authored fallback, without a native formation-algorithm
equivalence claim.

## Focused checks

With the private Z7 inputs already prepared:

```sh
make test-sc64-z7-reachable-formation
```

The target compiles the fixture with the full-render Z7 flags, both with and
without the experiment, under ASan/UBSan. Two synthetic Hydras use the real
Z7 terrain; an enemy Hatchery within map bounds keeps the objective live.
Selection and commands use ordinary R and field-command-menu inputs. The
fixture verifies the old disconnected slot, the reachable common-point
fallback and subsequent movement, a single-unit control, a flying-group
control, and an unreachable clicked point. Runtime faults are checked during
input and the 4,000-update movement continuation. These are isolated fixtures,
not a normal-menu campaign launch or mission-completion proof.

A private full-render ARM link with the gate enabled passed unchanged guards:
IWRAM end `0x03004ca8` (856 bytes margin), BSS end `0x0203df00` (256 bytes
margin), and ROM load end `0x09ead280` (1,387,904 bytes margin). The padded ROM
is 33,554,432 bytes. This is link evidence, not ARM playthrough, stack
high-water, SRAM-continuation or hardware-performance qualification.
The default build passed with the original-art ROM still byte-identical;
`./dev test` passed 554 Python checks and its host fixtures. No Actions ran.

## Promotion limits

The gate is absent from canonical flags and the full-render probe defaults.
The real normal-input controller still has no Z7 win. One private policy with
the gate reached 18 remaining enemy buildings at its 240,000-tick bound and
then stalled; the ungated policy also remained incomplete. Neither result
qualifies Z7. Separate scouting/target-selection and combat-attrition problems
remain under investigation.

Routing adds up to two static-route queries per selected ground actor on a
command. Command latency and worst-case cost still need measurement before
promotion; no near-30-fps claim is made. Staged campaign qualification stays
22/30. Generated inputs, ROMs, traces and saves remain private.
