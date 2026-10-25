# Protoss early-chapter requalification

The canonical Protoss profile still exposes P1–P3. This change closes the
current-stock host playthrough gaps identified in the Z4/performance audit;
it adds no campaign entries and does not establish 30 fps or native-engine
parity. The source-only project still requires privately prepared game inputs.

## P2 timed reinforcement capacity

The authenticated USA P2 map (`008/01F`, TRIG record 5) creates five Dragoons,
five Zealots, three Reavers and Fenix at 900 seconds. The previous adapter
attempted each spawn and set `FENIX_READY` even when the bounded unit bank
could not hold Fenix. Its existing survival predicate then treated the absent
hero as dead and lost the mission.

The event now preflights fourteen free slots and publishes readiness only
when the entire roster exists. If the bank is full it leaves the event pending
and retries later, without partially granting escorts or deleting existing
actors. An unexpected partial failure rolls back only this call's actors and
its supply increment. The original valid-Fenix-death defeat remains intact.

P2 enemy production now budgets free slots against fourteen pending scripted
actors and **all owners'** already queued products, including paired Zerglings.
It recalculates the allowance after queuing a worker before considering an
army product. Mining and guard dispatch continue. This is enemy queue
backpressure, not a smaller unit cap or a global reservation: deliberate
player production may still delay arrival until room becomes available.
After successful arrival the extra queue allowance is released.

The policy is derived from existing mission/phase/readiness fields. There are
no new save fields, save generations, limits, objectives, statistics or timer
changes. Live pre-arrival saves retain the pending event across SRAM reload.
Already-broken older saves that contain a partial arrival and readiness with
no Fenix are not reconstructed; blindly replaying that batch would duplicate
escorts. Start a new mission in that case.

## Current ordinary-input routes

P1's former controller selected every combat unit, sending rescued Fenix into
the enemy base. The new route parks him in Antioch, box-selects the other
combatants and attacks the nearest surviving enemy structure. After casualties
free supply, it queues a Probe through the captured Nexus and attempts Pylon
placement, then restores the assault selection. The optional placement can
fail; the qualification does not claim a completed Pylon or Gateway reinforcement.
All actions use ordinary buttons through `game_update`; world reads choose
cursor and route goals. No actors, resources, damage or outcome flags are
written by the controller route.

The canonical-flag host route wins at input tick 9657 with P1 objective flags
7, completion phase, Fenix alive and SIDE-owned, WON and no LOSS. P2's existing
ordinary controller also wins with the capacity fix: input tick 31623, flags
140, completion phase and living SIDE-owned Fenix. It waits through the source
900-second arrival and then attacks the Cerebrate. Both traces consume their
entire input sequence without overflow. Repeated runs match.

Fresh independent input-only host replays and full ARM input replays also
match both terminal states: P1 input tick 9657 / game frame 9606, all 183 input
transitions consumed; P2 input tick 31623 / game frame 31611, all 21 transitions
consumed. Both run audio and render on every update, retain the canonical
`GBA_SPRITE_WORDS` blitter, and disable only the separate experimental
`SC64_UNIT_WORD_SPRITES` sidecar path. They use isolated mGBA instances without
a GDB stub; each process stops at the validated terminal marker. These are
instrumented input-driver proofs, not physical-hardware tests or new FPS
measurements.

Box selection has finite world-sized bounds, terminal/stall checks and endpoint
validation. Trace input exhaustion now fails explicitly instead of silently
truncating the recorded inputs. `make test-protoss` runs both early controllers
under the actual canonical flags; its P3 controller now uses those flags too.

## Checks and limits

`make test-sc64-p2-reinforcements` covers both shared and canonical builds under
ASan/UBSan: thirteen-slot delay with unchanged existing actors, repeated delay
without partial grants, actual SRAM roundtrip with pending production, retry at
fourteen slots, exact roster/ownership, no duplicate arrival, original hero-death
loss, non-P2/skirmish exclusion, and real AI queue/guard behavior near capacity.

Local default build/tests, unified and canonical render checks pass. The default
Python suite has 481 passing tests before the separate opt-in raster work.
`make test-race-campaigns` passes all four Zerg and all three Protoss controller
wins, 54 menu launches and 702 startup/save configurations. The separate
`make test-sc64-terran-full-campaign` also passes the ten Terran host mission
controllers, their save/regression fixtures and target/spatial checks.
Canonical race, unified and Showcase ARM links pass the unchanged reserves:
IWRAM below `0x03005000`, BSS below `0x0203e000`, save payload at most 16368 bytes.
No physical-GBA performance claim follows from these checks.

The fresh P4 Storm controller was also audited against current stock values:
it loses Tassadar at frame 86450, despite completing all 87 scripted creates
with zero source faults. Its startup/save/Storm/frame fixtures and ARM link pass,
but the current playthrough does not qualify promotion. Recorded older P4 wins
remain historical evidence. Native special weapons, physics and construction
limitations still apply, and the full 30-mission/near-30-fps goal remains open.

Private traces and local logs are under `.local/build/protoss-requalification/`
and `.local/build/p4-current-qualification/`. All checks run locally; no Actions
workflow is enabled or dispatched. ROMs, assets, saves and private traces are
not published.
