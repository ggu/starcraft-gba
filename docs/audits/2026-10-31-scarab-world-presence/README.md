# Gated Scarab world presence

This candidate adds world-presence checks under `SC64_SCARAB_RUNTIME`. The
production products do not enable the gate. The real-Unit adapter remains
unregistered with the gameplay clock, UI, death/slot-reuse hooks and save
validator; this is not a playable Reaver service or campaign qualification.

Training and inside actors (phases 0/1) are excluded from draw, minimap,
visibility, targeting indexes and selection. They also do not block unit
movement, building placement or applicable P3 production exits. Outside
actors (phase 2, and the pending self-destruct phase 3) participate in the
world; initial no-collision ticks bypass unit collision in the persistent,
legacy, contact-cache and scan paths. The reference's Scarab UI exclusion is
by identity in pinned OpenBW `ui/ui.h:457–460`; this does not establish native
N64 UI execution.

The adapter initializes the live invincibility flag from the authenticated
type facts. Ordinary attacks reject that flag. This is not an unconditional
type ban: an explicit script override can clear invincibility on an outside
actor. Hidden actors remain off-map regardless. Scripted removal and expiry
need separate lifecycle handling, rather than damaging an invincible child.

Eligibility-changing child setters invalidate active collision, owner/target
indexes and the optional contact memo so the checked scan paths are used
until the next normal setup. They do not reset these caches for a pure life
countdown with unchanged eligibility. Hidden actors never unlink through an
uninitialized target-index cell. Source parent destruction can detach an
outside child as parent 255, preserving its owner and remaining flight data;
it cannot bind to a recycled parent slot.

`tests/sc64_scarab_presence_game.c` uses actual `game.c` and the real Unit
adapter with deliberately constructed phase states. Four prepared profile
vectors plus the gate pass ASan/UBSan. The P7 vector uses isolated source-only
fact tables generated with its exact feature gates. These inputs do not
regenerate or replace existing art banks. Tests cover hidden draw/health-bar
suppression, selection, vision, spatial-index membership, building placement,
applicable exit checks, collision and transitions while indexes are active,
live invincibility and its explicit override, and detached lookup. They are
not an ordinary-input acquisition replay. Earlier private attempts include a
missing relative BC-body input and an unguarded fixture-only P3 exit call;
both were corrected before the four-vector pass.

Remaining integration includes source-clock scheduling and training tick
ordering, authentic source collision/movement facts and complete art banks,
flight, splash and source-script death, all parent/child removal and slot-reuse
hooks, transport/transfer and AI policy, save namespace/migration and atomic
candidate-world validation. Normal ARM products, unchanged memory guards,
fresh-process SRAM and busy-scene timing must be qualified before enabling
the service. This candidate makes no hardware, frame-rate or full-roster claim.

## Qualification at ee7ca97

A clean indexed checkout with no private inputs passed source boundary,
doctor, default build and default test: 781 Python cases with 23 optional
skips, the separate eight-test suite and C fixtures. The new Make workflows
ran four actual adapter and four presence vectors under ASan/UBSan; captured
executed arguments included all canonical BC/CV5 gates plus the Scarab gate.
The 102 captured prepared inputs were unchanged; both newly generated
source-only P7 fact headers had SHA-256
`c720227debe1ca0bd96e7f0687df6f5d6890258155a99778eca2f49c8a425be1`.

An additional normal-Make ARM fit experiment used the additive authenticated
Scarab art and facing closures, with compiler dependency checks confirming
their selection. It froze existing generated inputs and kept 104 input
hashes unchanged. These are world-kernel lower-bound builds: the training
adapter and flight/UI/save integration are not registered in these ROMs.

| Profile | BSS end | IWRAM end | Result |
| --- | --- | --- | --- |
| Terran | `020370b8` | `03004f40` | Link and ROM verifier pass |
| Zerg Z6 | `02037124` | `03004fc0` | Link and ROM verifier pass |
| Protoss P7 | `020382e4` | `03004e08` | Link and ROM verifier pass |
| Showcase | `0203df38` | `03005150` | Existing IWRAM guard fails |

Showcase must end strictly below `03005000`, preserving the 11 KiB stack
reserve. No Showcase ELF/ROM was produced, no guard/cap was weakened, and
the failed candidate is not a user build. The last packaged user ROMs
remain #189.

A separate exact-ee7ca97, gate-off normal Make aggregate also completed with
exit zero: `unified test-unified terran test-terran
test-sc64-terran-full-campaign`. It retained 104 fixed prepared inputs, ran
the original full Terran startup/save/controller recipes and emitted nine
controller traces (T3 emits none). This does not qualify the gated service.
All nine traces also match the exact b638b50 row-candidate traces byte for
byte, with private per-file hashes retained.
Gate-off binaries are not claimed byte-identical or assigned the earlier
#189 scene timing; compiler code generation changed despite the false
feature conditions.

A subsequent isolated scalar-ROM factoring experiment preserved the cheap
type check inline and moved only payload decoding out of line. Eight host
fixtures and a broad predicate differential passed, but Showcase grew from
`03005150` to `03005190`; extra calls, register preservation and veneers
outweighed the removed bit code. It was rejected, is not integrated here,
and was not benchmarked after failing the memory guard. Do not assume that
moving a small repeated expression to ROM saves IWRAM or improves cycles.
