# Preserve paid production through Nydus reference cleanup

A successful canal entry removes unit-target references to its passenger.
Paid Eggs instead store a product enum in `target`; staged Guardian Cocoons
store raw product 44 there. Those values can numerically equal a passenger
slot. Treating them as idle-unit references cleared the paid job, which the
next production tick removed. The shared adapter now excludes these paid actor
identities while preserving normal idle, move, attack and Consume cleanup.
No save format, persistent array, mission objective or production price changes.

## Reproduction and checks

The unchanged engine with test-only `71b73c12` fails the paid-Egg survival
assertion after actual input-driven transit. The first fixture `b7099439` lacked
a producer and failed before reproducing the bug; that result is retained and
not counted as a reproduction. Fixed runtime `3ebed0e2` passes the complete
ASan/UBSan Nydus fixture, including:

- A genuinely paid Drone Egg surviving both transit directions and hatching
  through unmodified `game_update` calls.
- Ordinary idle/move/attack references clearing while an unrelated target stays.
- Existing blocked-exit, slot reuse, ownership and save/resume cases.

These are constructed HOST arrangements: Z7's authored source map has no canal
links. The fixture constructs linked endpoints and passenger slot/position,
funds and supply, then uses the real paid producer and ordinary transit controls.
The Cocoon branch is protected in code but has not received a separate actual
Cocoon-transit fixture. Scarab FSM target handling remains a separate staged
integration and is not qualified by this change.

All six normal ROM builds and unchanged memory guards pass at `3ebed0e2`.
BSS/IWRAM ends remain Terran `02036f1c/03004778`, Z7 `0203d474/03004f88`,
Z9/Z10 `0203dce4/03004bf8`, P10 `020391e0/03004ac0`, and Showcase
`02036d60/03004d20`. This is not a stack-depth or frame-rate measurement.
Terran, Z7, P10 and Showcase ROM bytes match the qualified pre-change main;
Z9/Z10 change as expected. Unified build and complete prepared unified tests
also pass. All 1,959 pinned inputs remain unchanged.

## Renderer fixture drift exposed by prepared inputs

The first full default test in this prepared workspace exposed five failures
that also reproduce on unchanged main `ae01547d`: three Control Tower vital-bar
comparisons and two renderer-fixture compiler failures. They are not silently
skipped or attributed to Nydus.

The canonical building-overlay oracle omitted visible selection ellipse pixels;
raw 115's small frame exposes them. An independent ellipse/cover model now checks
the complete selected overlay with a nonempty-ring assertion and full-frame
comparison. The two thin renderer compile commands omitted the production
controls/HUD capability pair and private icon dependency; they now use that pair
and declare the dependency. Production engine/HUD code is unchanged.

Both renderer test modules pass together: nine tests, one skipped, at `9e492abd`.
The earlier flag-only attempt lacked the icon include/dependency and failed;
that log is retained. Final source boundary, doctor, default build and complete default test pass
at `da316b94`: 1,004 Python tests with 13 skipped, plus compiled/auxiliary suites
(358 seconds). The prepared workspace exercises more optional cases than the
asset-free source-only runs. Engine/build source is unchanged from the six-ROM
and unified-qualified `3ebed0e2`; subsequent changes are tests and documentation.
