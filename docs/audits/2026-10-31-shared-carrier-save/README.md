# Shared Carrier and save experiment

This is a staged, default-off custom-game implementation, not a claim of full
cross-product parity. The Carrier experiment and its separate save experiment
are opt-in. Non-P7 campaign services remain fenced pending qualification.
P7 custom-game AI can now pay for and train Interceptors under the existing P7
training capability. With the Carrier experiment enabled without the save
experiment, non-P7 saves and loads are refused. The experimental Carrier save
namespace cannot currently be enabled together with the Scarab namespace.

The new compact journal generation preserves the Carrier pool and its free-row
ordering and omits only a checked, zero-filled campaign-only region. The older
Carrier generation and legacy save reader remain supported. Tagged candidate
loading is preflighted before commit. Existing legacy mission rebind paths
still require an atomicity proof; combined Carrier, Scarab and Defiler
persistence remains unfinished. This checkpoint does not activate these gates.

## Qualification evidence

Runtime source at merge commit `487db93` passed host checks for Terran, Zerg,
and Showcase. Each profile covered 234 custom-game configurations spanning
human and opponent races, one to three AI players, and two prepared maps.
Saturation fixtures used full actor and building pools, 66 synthetic valid
Carrier projectiles, 32 ordinary impacts, and two detached Yamato shots. The
resulting save payload sizes were 14,146, 16,308, and 16,252 bytes respectively,
within the unchanged 16,368-byte limit. Fixtures checked the full Carrier
packet, including free entries and subsequent reuse order. Separate paid-game
fixtures exercised Stargate, Carrier and Interceptor production and flight.

Actual save/load fixtures covered fresh-process mock SRAM, the older Carrier
format, legacy-reader compatibility, corruption fallback, and rejected saves
without journal mutation. Rendering fixtures exercised the active projectile
arena with cached terrain writes and canaries. The normal P7 services and
Carrier fixtures passed; the mixed Dropship branch was unavailable in that
product's actual raw map. All 109 prepared input hashes matched. Seven ARM
links passed the existing memory guards for the three experimental profiles
with gates on and off and normal P7. No hardware performance, stack-depth, or
full campaign-parity claim follows from these checks.

The source-only follow-up commits `99f0801` and `f784e09` fix test fixture
stubs, legacy-loop extraction anchors, and the fact-generator gate list. A
fresh unprepared default build and complete test suite passed at `f784e09`.
The added fact gates change generated fact signatures: gated builds at this
new revision require freshly generated, separately named fact tables before
they can be considered qualified. The saturation actors include synthetic
states; the tests do not exhaust every possible live phase, mission rebind, or
combined-namespace state.
