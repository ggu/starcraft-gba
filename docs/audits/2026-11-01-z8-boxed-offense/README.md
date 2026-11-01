# Z8 boxed northern offense controller trial

This is a host observation/controller experiment behind
`SC64_Z8_TRIAL_BOXED_OFFENSE`, which requires the existing Nexus-push trial.
It does not change the engine, mission, actor statistics, or selection state by
writing game memory. At the existing 5,500-frame stage it uses the ordinary
L-held drag gesture over the northern staging box, validates the selected
human roster, opens the ordinary command menu, and issues one Attack command.
It rejects workers, foreign or unexpected actors and checks screen, selection,
identity and command mode throughout the input sequence. After the source
owner-2 Nexus is observed destroyed, a box around surviving original northern
slots is used for the westward order, while the existing southern detector
Move order is retained. Source-world observation is explicitly part of this
host trial; hidden enemy coordinates are never used to issue an attack.

At the post-command observation, each surviving selected actor is reported
separately. Attack-move validation accepts the engine's formation destination
or reachable-formation fallback; direct-target validation distinguishes actors
that cannot attack the target. Actors that changed or died, commands whose
post-state no longer shows the order, and incomplete UI flows are reported,
not counted as successfully applied. The actor identity check uses slot, raw
type and source owner; it cannot distinguish same-identity slot reuse without a
generation token. A wide westward bounding box can include unwanted friendlies;
the controller rejects that selection rather than silently ordering them.

This source-only variant has not been compiled or run. It does not claim a
coherent arrival, a destroyed Nexus, a campaign victory, or physical-input
qualification. The original single-unit order helper and strict victory bound
remain unchanged.

## Bounded candidate result

The `b4f4383` host trial used the boxed policy together with the shared spawn
hook candidate. It is not an isolated measurement of grouping: the box also
selected Overlord 16 and Zerglings 21 and 22, which were absent from the eight
individual offensive orders in the preceding `90f361b` trial. In that earlier
trial, by frame 6360 both Ultralisk slots had been reused by enemies, both
Hydralisks and Zerglings 17–19 were dead, Zergling 20 was still alive, and
Zerglings 21–22 remained alive at staging; Zergling 20 died by frame 7200.

The single bounded candidate run reached LOSS at frame 13412 (source trigger
17; native frame 10729, fault zero), not the strict WIN condition. Its initial
box selected 11 actors, and all 11 were observed with attack-move formation
orders at frame 5803. By frame 6000, one Ultralisk slot had already been
reused by an enemy and Zerglings 18 and 19 were dead. Both source Nexus actors
were alive at the frame-5500 push start; no east-cleared event was observed.
The original Kerrigan remained valid at 328 HP at the loss. This run gives no
campaign-win or isolated causal claim. Its local log SHA-256 is
`98bb4d485b2c1c58e05229be83a355fced15d8a5bf050ba9558ec45f576ccfed`.
