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
