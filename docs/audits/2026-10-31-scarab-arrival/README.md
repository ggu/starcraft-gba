# Experimental Scarab arrival and death timing

This default-OFF checkpoint is based on `a3e0181`. It uses pinned OpenBW
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8` and authenticated StarCraft 64
source data, not native N64 execution. The input ROM SHA-1 was
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`; the staged IScript SHA-256 was
`f8e5f3d0fcca644470c37de65486e44b09eb30141eedccea3445afe7bc0f65bc`.
The existing authenticated loader rechecked staged tables and images against
the owned ROM. No raw assets or generated headers are committed.

In pinned `order_ScarabAttack`, arrival starts SpecialState1 immediately and
sets state 6. The decoded SpecialState1 path executes `useweapon 82` and
`sigorder 1` at local tick zero; state 6 consumes the signal on the next
order update and calls `kill_unit`. Weapon 82's target-position bullet enters
its Death animation; its Death script executes `domissiledmg` at local tick 5.
The separate Init script's damage event at tick 7 is not the observed path.
Unit death creates image 444, a ten-source-update overlay. Death order, slot
retirement, and the overlay's visible lifetime are distinct events.

The retained instrumented OpenBW flat-map world probe ran eight geometries or
headings, including a deliberately injected turn, at eight attack-start delays
(64 cases). All 64 observed the same sequence relative to state-6 arrival:
weapon 82 and child signal at +0, Die order at +1, victim damage at +5, and the
child sprite pointer null at +11. `tools/sc64_scarab_arrival_check.py` verifies
case coverage, per-log hashes, bullet Death wait 4..0 and this timeline. The
initial instrumented run had 16 failures because its assertion assumed the
first post-release observation always captured untouched 90/7 initialization;
the revised probe permits an already-advanced initialized state. Both attempts,
argv, exits, logs and hashes remain under ignored `.local/arrival-oracle`.
The delayed starts change attack timing in the synthetic world; they do not
cover all targets, collisions, scheduler states or hardware implementations.

The GBA probe already created an independent packet at arrival and applied
its damage on the fifth subsequent source tick. This checkpoint names that
value for the source Death-script event and tests child removal on the first
subsequent source tick rather than claiming an unexplained universal delay.
The original visual observer scanned on alternate updates and could add one
or two game updates of latency. For an outside Scarab removed by the
experimental dispatcher, it now creates the existing source death effect on
the Die update and clears its observer bit so a later scan cannot duplicate
it. No per-actor storage or new asset is added; the effect remains transient
and uses the existing bounded pool. Source animation timing and screen update
quantization are not a native-hardware timing claim.

Sanitized actual `game_update` fixtures passed five controlled cadence phases
and the existing upgrade, pool-full, building, destruction, owner change,
slot-reuse, shield and fractional-packet cases. They check same-update effect
creation, one effect, ten-source-tick expiry and no early/double damage. The
separate fresh STOP/parent-loss/retained-target dispatcher fixture also passed.
This does not establish general terrain pathing, full bullet pool behavior,
persistent upgrades, repeated fractional residue, outside-child save or
release readiness. All experimental activation gates remain OFF.

With the full experimental gate stack ON, all four current ARM links passed
unchanged reserve guards: Terran `020370f0/03004e68`, Zerg
`02037164/03004f68`, P7 `0203831c/03004c38`, and Showcase
`0203df70/03004ff0` (16 bytes below the IWRAM limit). These match the prior
probe endpoints. The P7 link used the previously qualified ignored
Scarab-roster facts; all 104 frozen inputs still matched after the links. No
new clean default qualification was run for this gated-only change.
