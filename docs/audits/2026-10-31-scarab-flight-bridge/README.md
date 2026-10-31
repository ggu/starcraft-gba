# Scarab flight save bridge prerequisite

This unregistered source-only component does not enable flight, change save
namespaces, or claim compatibility with older game saves. Its caller must first
authenticate the new Scarab namespace and the raw-85/type identity; the
helper rejects Scarab rows if that namespace is not explicitly supplied.
Non-Scarab rows pass through unchanged.

The live child carry holds parent (8 bits), phase (2), and two 9-bit position
fractions (18). Hidden children require zero fractions, zero life and grace,
idle state, and no target. Outside parent 255 represents a detached child.
The five accepted logical FSM states occupy three order bits. A future integration
must explicitly map native source states, including source state 6, into this
encoding; this assumption is not proof that source flight or pathing fits it.
Dead raw-85 rows retain their fields until slot reuse. An expired outside row
may have zero life when dead. A live outside row may have zero life only
with the logical fresh-release code (1) and zero grace, representing the
observed source frame before the next-frame flight initialization. This code
is a bridge mapping, not a raw source FSM value.

A compact wire key ranks the hidden and outside states in 126,353,888 values.
Mixed-radix digits use the signed-17 carry code, 242 allowed P7 build codes,
one high order bit, and timer bit 10. The resulting 126,877,696 possible
codes include 523,808 rejected holes. The bridge reconstructs logical build
-1, FSM and timer before world checks. Historical signed-16 word rows instead
use biased low 16 carry bits and the high 11 key bits in the build word.
Weapon, flags, shield regeneration, waypoint, and retry fields are not used.

The source-only test exercises all keys and holes, representative states through
the four actual compact codec families, and the production historical word
loops in four enum sets. The derived codec intentionally drops weapon; the
historical signed-16 row does not preserve a weapon word with bit 15 set.
Those are existing codec limits and are not asserted as bitwise preservation.

Integration still requires a distinct collision-safe namespace and full
semantic preflight, conversion before save codec validation and after decode
before generic world checks, equivalent conversion in final legacy live restore,
field-preserving lifecycle writers, and actual source movement/pathing and
render-facing behavior. No current runtime callsite invokes this helper.

Independent review of exact child `6c91311` found no component blocker under
this explicit namespace contract. Its eight profile/layout combinations pass
ASan/UBSan, including all ranked keys and rejected holes. The combined draft
at `1c1a9dd` repeats those eight combinations successfully; this does not add
whole-save, migration, runtime, ARM or fresh-process qualification.
