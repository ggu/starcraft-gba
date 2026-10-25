# Z7 ordinary-input controller trial

This experimental host controller uses ordinary HOME/menu/gameplay inputs
with the existing Egg, source-sight, Scourge and nearest-depot options. Live
state reads guide targeting, economy and retreat decisions. A full run
returns success only for a completed source victory; loss and tick limits
return failure and must not be presented as campaign completion.

With the private Z7 inputs already prepared:

```sh
make test-sc64-z7-controller
make zerg-z7-controller-probe
```

The trace stays at `.local/build/z7-controller/controller-trace.json`.
The separate ARM probe is `build/StarCraft-Zerg-Z7-Controller-Probe-GBA.gba`,
title `SC Z7 RUN36`, code `SCZQ`. Building it is not an ARM gameplay result.

The current controller has not won. Its paid second-Hatchery experiment
reached a source-frame 22,491 loss with 55 enemy structures remaining, versus
19,808 in the preceding ten-worker attempt. Ordinary build and production
readback confirmed the second Hatchery and paid use of both producers'
larva. The expansion was subsequently destroyed. ASan/UBSan reported no
error in the bounded attempts; longer survival is not a completed objective.

Earlier read-only combat diagnostics showed that delayed air targeting came
from controller input sequences and eligibility checks, rather than low
Mutalisk volley damage. The trial prioritizes defensive targeting before
worker evacuation. Native AI timing remains unresolved; this controller does
not change the game's AI, objectives, actor capacities or save format.
