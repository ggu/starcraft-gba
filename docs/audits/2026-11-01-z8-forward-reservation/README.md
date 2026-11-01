# Z8 forward worker and budget reservation trial

This source-only host controller variant layers a new macro policy over the
boxed-offense trial and the separately staged shared spawn hook. Inputs still
flow through ordinary `game_update` buttons; it does not write wallet, actor,
selection or mission memory. The experiment is a coupled controller and engine
candidate, not a test of boxed selection in isolation.

Before an accepted Nydus trip, the controller completes its observed Pool,
Evolution Chamber, Hydralisk Den and Spore policies. It then watches the chosen
Drone's slot, owner and raw identity after every update, including updates
spent in UI routines; builder searches use `build_worker_available` and exclude
the reserved slot and Nydus-entry order. An accepted trip reserves 300
minerals against subsequent scripted spending until a newly observed Hatchery
foundation or an explicit abort. Wallet, queue, building and traveler
transitions are logged as observations; a before/after delta alone does not
establish the cause of spending. The offense waits for six actual living eligible human-source Hydralisks
inside the northern selection box, rather than counting accepted queue attempts.
The earlier first-raid, north-detection and southern-defense policies remain.

The same-slot, same-owner and same-raw-type reuse case cannot be identified
without a generation token. An accepted build order remains pending with its reservation until the
foundation appears, its actor is lost, or a 1,800-frame timeout leads to an
ordinary Stop command that is observed to cancel the build order. Three
unaccepted attempts explicitly abort. If Stop cannot be confirmed, the
reservation remains held. Previously observed base prerequisites may be
rebuilt after their disappearance; accepted but not yet observed foundations
are not treated as destroyed. Reserved funds and a delayed
push may themselves worsen defense. These are policy choices, not a proven
victory strategy. This variant has not been compiled or run; its strict
victory, original Kerrigan identity and update bound are unchanged.
