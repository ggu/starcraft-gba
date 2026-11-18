# Native SCV interruption at a paid Barracks foundation

This checks whether an ordinary Move can leave a paid Terran foundation
available for another worker. It is separate from the [post-completion
mining plan](2026-11-13-n64-scv-construction-resume-plan.md), which asks what the
same worker does after completing a building.

The [ordinary Barracks route](2026-11-14-native-result-opening.md#ordinary-input-barracks-construction)
provided the paid scaffold and its original selected SCV at command end
7,659. A fresh native run matched all 98 archived command rows, the three
full raw images at 3,695, 4,200 and 7,659, and the byte-exact 7,659 guest
row. The cursor was on open ground left of the scaffold. The only new
button input was an eight-step C-Right pulse starting at 7,659; neutral
input continued through 11,900. C-Right has previously issued ordinary
Move on this controller. The recorded pulse does not independently
prove that the constructing SCV accepted it.

At command end 7,899 the selected SCV was still beside the scaffold.
The same Barracks was at full 1,000 HP in the read-only samples at 11,844
and 11,900, up from 214.75 HP at the 7,659 anchor. The original SCV's
position and order at both late samples matched the completed control.
Its late movement and order transition therefore do not establish that
the pulse interrupted construction. No unfinished foundation or stalled
progress was observed in this run. No replacement was attempted. This
does not rule out another way to interrupt an active builder. The run
kept its existing file caps and reported unchanged inputs.

## What would establish reassignment

The [native control audit](2026-11-07-native-actor-differential/README.md)
does not verify a Stop shortcut. First verify a displayed **worker Stop**
or an explicitly worker-scoped **cancel current order**. Then check the
original SCV stops working while the same paid foundation survives and
its measured progress stops advancing past the normal completion point.
Deselecting a worker only changes selection and is useful as a control;
canceling the building removes the original foundation and cannot test
reassignment. Do not treat contextual B as Stop without a matching prompt.

Select a distinct live SCV and verify the displayed action on that same
foundation before triggering it. Confirm the original worker remains
away and the foundation resumes before claiming replacement. Verify the
wallet separately if claiming no second charge. Removing the original
worker by enemy damage is a separate case. Do not weaken the exact raw,
guest-state or prospective action gates to obtain either witness.

The GBA's Terran skirmish CPU already can assign a new SCV to a paid
foundation. Its human repair action requires a completed building. This
native run does not establish a replacement command for the GBA.

| Local receipt | SHA-256 |
| --- | --- |
| Departure diagnostic | `52ddc467712fc11b3fd4579414e7d7833a5f2f2b36f4bb105c844a14c5650c84` |
| Departure replay manifest | `98cfad7e3e2295477e4951607adcdfc519c770ec6d34b7bed2bc76f5d0e6bb59` |

## The constructing SCV stays selected without a displayed Stop

A separate read-only observer adds samples at 9,308 and 9,332. It matches
all 104 archived command rows, all five full frames and the 7,659 guest
and renderer checks. The only A in the observed suffix was already in
that archived control: at 9,319, after the existing cursor pulse and its
neutral release. The 9,308 sample verified the active original SCV and
same unfinished Barracks before the suffix was replayed.

The same active SCV was the sole selection at both 9,308 and 9,332.
The same Barracks advanced from 583.75 to 589 HP while the SCV remained
on order 33. Neither frame displayed a Stop or any other SCV command
hint. The [N64 instruction booklet](https://nintendo64.pl/wp-content/uploads/StarCraft-64-USA.pdf)
describes visible C-button functions. The [pinned PC reference](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L16936-L16949)
treats Stop as an exception for constructing SCVs. The replay confirms
that Stop was not displayed in this exact native context; it does not
show that no controller shortcut exists. No Stop or other unarchived
order was sent.

| Local receipt | SHA-256 |
| --- | --- |
| Selected-worker contextual diagnostic | `f1ea92157f696da8731fc13857f9ea5fd68663cd79c8505e5ed92ef93d89b7e6` |
| Contextual replay manifest | `8ee2f306ea2227cd25cf9bc8fe952b992d03197e3141356e9212b5460219d3bc` |

## Shortest worker-loss candidate from the archived match

Do not wait beside the original Barracks. All four human SCVs survive
through the retained 24,418 sample. The [same Blood Bath witness](2026-11-14-native-result-opening.md)
instead records two opposing Terran Marines at (179,404) and (220,244)
from 15,845 through 15,962. The latter stays at (220,244) in the retained
samples through 24,418; (384,464) is also repeatedly occupied by opposing
Marines from 22,208. These are sampled positions, not known paths or
attack ranges. The northern Terran route is the first grounded
armed contact suggested by the archive. The nearer Zerg samples on the
existing southern route were workers, Overlords, or unarmed colonies.

Fork from the exact 11,900 opening, before committing to the southern
Marine attack. The next short calibration should scout west toward the
Terran contact, recover a verified reachable approach and confirm
ordinary enemy damage there. Retain that exact approach before placing
a new paid foundation near the contact. Keep another SCV outside the
fight. Verify the paid structure is unfinished and the original SCV is
still working when hostile damage begins. Require the original builder
to disappear from the live owner chain while the same building survives;
then require unchanged measured progress over a neutral interval. Only
after that result can a newly reviewed contextual action test the spare
SCV. Abort if the structure completes first, the opponent targets the
foundation, or the route exceeds its existing bounds. No live enemy
attack on a constructing SCV or worker-loss replay has been established.
