# Z10 resolved-damage and production observation

This bounded HOST experiment follows the upgraded ordinary-input controller at
`1344d53fb84c914190423fbd9f1861117d331989`. Observer source is
[dc40e1646cc530fe488b1eac29174a4a88d35861](https://github.com/ggu/starcraft-gba/tree/dc40e1646cc530fe488b1eac29174a4a88d35861) on
`codex/z10-damage-attribution-20261105`. It uses the Z10 profile plus the existing
HOST shared-Egg and Egg-wire stages, not a delivered guest ROM. No resources,
units, objectives, saves or outcomes were injected.

The observer runs after resolved unit HP subtraction, before death cleanup. It
records target identity, previous/current HP, resolved damage and the supplied
source type/owner. A second observer records accepted AI production and the
town wallet before/after payment. Both are HOST-only and disabled in the
control build. They do not change the controller strategy or simulation state.

## Verification and limits

Both control and observer used Clang `-O1 -g0`, ASan/UBSan, the same frozen
source, and 1,959 hash-verified inputs under the native lock. Each stopped after
31,000 input calls at game frame 30,284, with fault 0, no victory/defeat yet,
two Drones and seven Hydralisks remaining. Their complete controller traces,
stdout and non-observer stderr matched. All 6,442 changed-input records also
matched the original upgraded run's input sequence. Trace SHA-256:
`6bf8fa8d414e480d7cc514c9ee5faeb374eecdee431c7c12205e4a9518f63410`.
This compares recorded state/decisions, not every byte of simulation memory.

The first wrapper incorrectly required the controller's return code to be 0.
A bounded non-winning run intentionally returns 1. The corrected wrapper
required a valid fault-free outcome summary consistent with the return code;
both runs returned 1, and no sanitizer error was found. The original failed
wrapper log remains preserved. Local evidence is under
`/tmp/sc64-z10-damage-audit/.local/qualification/damage-pair-retry/`.

## Observed losses

There were 433 resolved HP-damage events against SIDE actors, including 31
against Drones. A lethal event requires positive HP before the hit and
nonpositive HP afterward; construction consumption is not counted as a hit.

| Recorded source type | Lethal Drone hits | Lethal Hydralisk hits |
| --- | ---: | ---: |
| Reaver, mapped raw 83 | 12 | 63 |
| Archon, mapped raw 68 | 3 | 0 |
| Scout, mapped raw 70 | 1 | 15 |
| Dragoon, mapped raw 66 | 0 | 1 |
| Generic/placeholder mapped raw 0 | 0 | 2 |

The source type is the damage path's supplied identity, not a unique shooter
slot. Generic source 0 must not be called proof of a Marine attacker; building
and other generic paths can use that placeholder. The active Reaver path is
explicit in the code: the normal Scarab service is off and its fallback deals
generic damage without paid ammunition. Late sampled Reaver hits resolved to
98 HP damage after armor. This establishes that this path kills workers in
this port trial; it does not establish how the same battle plays on N64.

## Observed AI payments

| Queued product | Accepted purchases | Minerals/gas per purchase |
| --- | ---: | ---: |
| Zealot | 28 | 100 / 0 |
| Dragoon | 32 | 125 / 50 |
| Scout | 18 | 275 / 125 |
| Reaver | 21 | 200 / 100 |

All 99 observed accepted purchases deducted those amounts. A paid queue is not
proof of a completed or surviving unit. These data do not support an allegation
that the Reaver bodies were free; Scarab ammunition is the separate missing
lifecycle. Other Drone removals, target-selection priorities, town income and
N64 wave/attack behavior still need their own evidence.

## Next work

Prioritize the shared paid Scarab lifecycle, including production, capacity,
flight/pathing, impact/splash, AI use and persistence. Compare native behavior
before changing damage or campaign difficulty: adding source splash can also
increase danger, so this result is not evidence that an ammunition fix alone
will make Z10 winnable. See [project priorities](../../PROJECT-TODO.md).
