# P1 and P2 race controllers, and the Probe economy under them

This answers two questions left open by the
[carried Z2 audit](../2026-11-03-z2-carried-controller/README.md): why P2 lost at
tick 45,892, and why P1 depended on start timing. Both were controller defects on
`d60e75eb`. The rebuilt routes in #253 fix them, so this change is stacked on #253
and corrects what remained: Probes that stopped mining with tens of thousands of
minerals still on the map. The engine and generated inputs are unchanged.

## P2 at tick 45,892

The old route waited 15 minutes, attack-moved once and never trained anything. In
both runs logged, offsets 0 and 40, only Fenix was left by tick 35,500.

**Why the tick is identical.** After the force dies, nothing the controller did
matters. `race_campaign_ai_tick()` releases two idle units every 5,400
`elapsed_frames`, and a start offset delays only the controller's input, not that
clock. It takes the lowest-numbered idle units and sends them at the lowest-numbered
side unit, which by then is Fenix, who was still idle at his arrival point.

| Elapsed frames | Units released | Result |
| ---: | --- | --- |
| 37,800 | Two Zerglings (slots 2, 3) | Fenix kills them |
| 43,200 | Zergling (slot 4), Mutalisk (slot 5, raw 43) | Fenix kills the Zergling and cannot hit the Mutalisk |

The Mutalisk attacks from about 47 pixels away, and his 240 shields and 240 HP are
gone 2,676 ticks after its release. All 144 offsets from 0 to 143 end at tick 45,892.

**Why it used to win.** #253 bisected this to #133, which limits new automatic targets
to what the player can currently see. With that check disabled in a scratch copy of
both helpers (`sc64_auto_target_visible`, `sc64_auto_target_candidate_allowed`), the
same old route wins 144 of 144 offsets. So the route won only through targeting into
fog. The fog rule is intended and stays.

## P1 on the old route

P1 won 90 of the first 144 offsets. The 54 losses had three causes:

| Losses | Ended at about | Cause |
| ---: | --- | --- |
| 43 | Tick 5,100 | The route gave up when Fenix's 3,000-tick parking walk expired. In the run inspected, he was still walking beside idle Dragoons at 633,937. |
| 6 | Ticks 13,900 to 17,300 | The force ran out with one building left. The route stopped there, leaving Fenix unused and 800 minerals unspent. |
| 5 | Tick 575,000 | It selected the force once and never again. In the run traced, the box caught only two units, those two died, and every later attack order went to an empty selection until the 100 passes ran out. |

The cursor change in #249 moved only a few of these, because none of them involve
that cursor loop. The one-shot reinforcement never completed at any of the 144 offsets.

## What #253 left

#253 wins P1 and P2 at most offsets, and all four of its 144-offset counts reproduced
exactly on separately copied inputs. It described its remaining losses as "ordinary
attrition: the army runs out with a few buildings left and no minerals." That was not
the main cause. Runs showed Probes idle, none mining, and a bank too low to train,
while the map held 20,000 to 29,000 minerals. P1 stalled until its 150,000-tick limit
at six of the first 720 offsets.

Three separate defects stopped the Probes:

**A Probe parked by a patch blocks it for good.** `selected_worker_mineral_target()`
returns nothing while `selection_target()` finds an own unit within 31 pixels or a
building within 26, and an order needs the patch within 16. So one idle Probe beside
the best-scoring patch turns each tap into a selection. Each economy pass made up to
27 taps on that same patch and never tried another. With only the selection change
below applied, the Probes parked near one patch kept all six idle empty Probes from
mining at P2 offset 33. The route now picks only
patches with a tap point clear of both.

**Half the workforce stayed idle with cargo.** A builder carrying minerals goes idle
holding them, and the mineral order skips loaded Probes. At P1 offset 534, three of
six Probes were in this state, and at P2 offset 33 it was six of twelve. The route
now taps the Assimilator, which discards the cargo, then presses STOP so the next
mining pass can use that Probe. It costs 8 minerals each in the runs inspected.

**The drag fallback cleared a good selection.** In a stack, a tap takes the nearest
Probe, which may not be the one requested. The route counted that as a failure and
fell through to `controller_select_worker_box`, whose drag there has no area and
clears the selection. At P1 offset 534 this repeated until the time limit. It now
keeps a tapped Probe that stands within 4 pixels of the requested one and has the same
cargo state.

## Start-timing sweep

Wins out of 720 offsets in each column, in scratch builds without sanitizers. Offsets
720 to 1,439 were not looked at while the changes were being written.

**Races vector (`make test-races-controller`):**

| Controller | P1 0-719 | P1 720-1,439 | P2 0-719 | P2 720-1,439 | Stalls |
| --- | ---: | ---: | ---: | ---: | ---: |
| #253 | 697 | 699 | 685 | 686 | 16 |
| All changes | 718 | 713 | 712 | 718 | 0 |
| Without the patch check | 717 | 714 | 692 | 689 | 0 |
| Without freeing cargo | 714 | 713 | 712 | 716 | 2 |
| Without the selection change | 716 | 710 | 719 | 714 | 1 |

**Carried vector (`make test-sc64-protoss-carried-missions`):**

| Controller | P1 0-719 | P1 720-1,439 | P2 0-719 | P2 720-1,439 | Stalls |
| --- | ---: | ---: | ---: | ---: | ---: |
| #253 | 668 | 675 | 658 | 642 | 46 |
| All changes | 716 | 717 | 711 | 712 | 0 |
| Without the patch check | 714 | 716 | 650 | 638 | 3 |
| Without freeing cargo | 707 | 709 | 693 | 689 | 12 |
| Without the selection change | 715 | 714 | 714 | 711 | 0 |

Stalls are runs that reached the 150,000-tick limit, counted across both missions.
Outcomes are chaotic, so differences of a few wins mean little.

- The patch check is the main gain for P2 on both vectors.
- Freeing cargo removes the remaining stalls on the races vector, and matters more on
  the carried vector.
- The selection change has no clear effect on win counts once the other two are in,
  and its P2 columns are about level or slightly lower. Applied alone to #253 on the
  races vector it scores 714, 713, 688 and 693 with 2 stalls, so it is a large
  gain for P1 by itself. It stays because it corrects a traced failure, and the comment
  above it already claimed that behavior.

For comparison, the old route on `d60e75eb` won P1 at 90 and P2 at 0 of the first 144
offsets on the races vector.

## Still failing

- 9 P1 and 10 P2 offsets of 1,440 still lose on the races vector. In every one
  inspected the army was gone. The P2 losses end between ticks 36,889 and 47,742 with
  106 minerals or fewer. Those inspected had no more than one visible patch left, with
  about 19,000 minerals elsewhere on the map, because these routes stay on their home
  patches. I did not classify the carried vector's losses.
- Sending Fenix to attack alone once the army is gone can turn these into outright
  mission losses. #253 does that deliberately, and it was left alone.
- `make test-sc64-p7-campaign-composition` was not run here; #253 records its P3 step
  as failing, and that route is not touched.

## Results

These are native host results, not a physical-key replay or hardware measurement. No
ROM, save or mission is promoted. The checks and tested commit are recorded in the
pull request.
