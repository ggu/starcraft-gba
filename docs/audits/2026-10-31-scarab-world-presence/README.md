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
