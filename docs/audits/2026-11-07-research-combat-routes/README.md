# Typed research through existing integer damage paths

This staged change is based on the shared research prototype, not enabled in
normal ROMs. It does not add fractional HP/shield storage or complete shield
upgrade mitigation.

A common unit armor accessor selects the active custom owner's ground/air
family, retaining the established human legacy floor and untyped fallback.
Ordinary stock/fallback hits, siege splash, P6/Archon compensation and the
Scarab precomputed route use that same accessor. Archon weapon 70 uses Ground
Weapons; Interceptor weapon 79 uses Air Weapons when its delayed damage pulse
is consumed. The inactive-feature accessor reproduces the existing SIDE-only
legacy expression. No new actor fields, arrays or journal bytes are added.

At `674a92c2`, an actual P10 HOST ASan/UBSan fixture passed in 18.27 seconds
under the shared native lock with 40 authenticated private inputs unchanged.
It exercises stock and fallback unit hits, siege resolution, Archon damage and
its splash attack, and real Interceptor launch/motion/impact with exactly one
damage pulse. It distinguishes ground from air upgrades, different owners, and
the human legacy floor from an opponent's typed level. Independent read-only
review found no actionable defect in these changed integer paths.

Run the prepared-input check with
`make -f Makefile -f tests/sc64-research-combat/Makefile test-sc64-research-combat-routes`.
The private qualification selected the authenticated current audio bank.
These are constructed states, not an ordinary-input match, all-race/product
qualification, or N64 damage fidelity. Fractional damage, shields, T4 traps,
projectile source/current-owner and upgrade timing, campaign research binding,
and remaining specialized paths still need their own evidence. The prototype
must not be enabled in normal products on the strength of this fixture.
