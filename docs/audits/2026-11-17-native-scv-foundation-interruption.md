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
