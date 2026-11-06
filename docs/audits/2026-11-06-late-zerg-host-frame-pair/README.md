# Constructed late-Zerg host rendering checks

This is a test-only qualification aid, not an optimization or a performance
measurement. The normal Z7 and Z9 release flag vectors are compiled with and
without the existing five canonical cache flags. Each binary must pass its
own full-clear oracle; the runner also compares the complete ordered indexed
pixel and RGB555 palette traces between the two binaries. It records a hash
manifest before and after the run and uses the shared native-build lock. Its
private inputs and output traces stay outside version control.

The campaign fixture asserts the actual Z7 or Z9 mission identity. The custom
fixture constructs every available custom map, all three human races, and one, two,
or three AI players with an explicit roster. It checks repeated frames on both
pages, scrolling, pause, save/reload and HUD. These constructed host scenes do
not establish ordinary-input gameplay, native N64 equivalence, or GBA timing.

An initial custom oracle reported six differences on the arena's Zerg starts.
That comparison incorrectly reused one reference image for four render calls:
`sc64_walk_track` advances presentation-only stride/facing bits during drawing.
The corrected oracle builds a separate full-clear reference for each render,
restores the initial presentation state before replay, and checks the resulting
state as well as pixels. It also uses the production clipping and text-box
ordering. The original failure and traces were retained in the isolated local
build directory; no engine fix or cache promotion follows from this finding.

The required actual-engine ARM, ordinary-input state/pixel equivalence, RAM and
cycle qualification remains open. Do not infer a hardware speedup from these
host tests.

The `SC64_RENDER_KEY_ONCE` static-object key snapshots mineral, geyser,
building and applicable mission prop fields. It does not read the walk/facing
presentation array mutated by unit drawing. Thus this observed render-side
mutation does not by itself invalidate that particular key; it does mean the
renderer should not be described as generally free of presentation-state
writes.

At source parent `93adaab2`, the final campaign pair passed 91 captures per
profile; the Z7 trace SHA-256 was `65a007cdf85228349e544dabc0311d0d82ef6150856b6db60a57cd3b8c47eb17`
and Z9 `5d9ef885a8c4f72025e44c6cf5143144736f281724acc2d10d8822d365b93b9c`.
The custom matrix passed 576 captures per profile; Z7 trace SHA-256 was
`4c23a2c6c8fd79cdb10daa1bfdd01b11615c13e9e3b963bf634ffc5a0003f574`
and Z9 `d5b7e4cbfc94d8090646ee1030990d60a6999f714c245c1258be5912ef8bc0e7`.
Each pair matched in full, each reference check passed including walk/facing
state, and the input manifests were unchanged. The bank required by current Z9
was regenerated from prepared private inputs and matched the recorded SHA-256
`fafb2a0ac415ef3f12249539debcc95204560c15a565fbd11d72c2da701c1c9e`.

Source-boundary check, doctor and default build passed. The default test command
failed: of 1,004 Python tests, five failed and nine skipped. Four profile-roster
subtests encountered missing indexed weapon or BC-body headers or incomplete
Zerg sprite banks, and one private-output safety test encountered the isolated
workspace's `.local/build` symlink. The full test log is retained locally at
`build/late-render-default-checks/test.log`. The symlink has since been replaced
with an isolated directory; targeted rechecks and input repair remain pending.
