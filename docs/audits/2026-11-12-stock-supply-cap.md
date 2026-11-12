# Stock 200 supply limit

A player used to be able to reserve more than 200 supply by building extra
Depots or Pylons or making more Overlords. The shared production gates now
use at most 400 engine half-supply units (200 displayed), after counting
live units and existing production reservations. The HUD denominator and
the race AI's planning use the same ceiling. This applies to campaigns,
custom games and Showcase without a mode-specific flag. The small
original-art and legacy skirmish builds also use the ceiling.

Provider capacity stays uncapped in the engine's incremental counter and
the legacy save field. Losing one surplus provider must not lower a
displayed 200 until the actual surviving capacity drops below 200. A
provider loss never removes units or cancels an already accepted queue;
zero-supply units such as Overlords remain trainable. Scripted
creations and rescue are not new paid orders, so they can leave actual
usage above capacity. The HUD reports that usage rather than hiding it.
The legacy loader accepts this state without weakening its check that the
stored provider total matches the surviving rows.

The official [StarCraft Overlord guide](https://classic.battle.net/scc/zerg/units/overlord.shtml)
describes the standard 200 maximum and the advantage of surplus providers.
It notes a Team Melee exception, a mode this port does not offer. The
official [Brood War Dark Archon guide](https://classic.battle.net/scc/protoss/units/darkarchon.shtml)
also describes independent race supply when controlling multiple races.
The port still aggregates mixed-race armies by owner, including missions
with a shared or rescued force. Per-race ledgers for those cases remain a
separate fidelity task. These sources establish the stock reference;
there was no fresh N64 limit replay for this patch.

## Local checks

`make test-sc64-stock-supply` exercises real unit and provider rows for all
three races in both campaign and skirmish contexts, for the human and
computer owners (all three AI slots in custom). It checks reservation to the last point, refusal without
spending, the HUD above the ceiling, loss of excess providers, paid queue
retention, Overlord recovery and scripted creation above the limit. The
same checks pass under both the Race and Unified HOST sanitizers. The
original-art HOST suite passes 30,000 simulated frames, and its field
fixture verifies save/load after losing enough supply to be over cap.
The asset-free race rule fixtures pass both build variants. The broad
Race integration binary has a pre-existing gas fixture failure reproduced
on unchanged main. ARM ROM and real-hardware performance are separate.
