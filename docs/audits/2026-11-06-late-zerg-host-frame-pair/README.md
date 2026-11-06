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

Input provenance limit: the frozen Z7 vector used a Z6 sprite bank with
SHA-256 `c912a42a84134190e934f4ba4cb7e6f2075b3cf31f7188a3c3a40cfd9800a320`.
A prior prepared-input receipt records a different bank,
`ff145afa6ceec577b2d862176d7b983c66b98b179998654ffc18058034f537ac`.
The host comparisons establish equivalence for the frozen art actually used;
they do not establish historical-bank identity or complete stock-art coverage.
Recover and authenticate the required current banks before performance replay.

Subsequently, the Z4, Z6 and canonical Z7 banks were recovered in isolated
storage and matched the prior receipts (`d33024e0…`, `b8162b34…`, and
`ff145afa…` respectively). The full paired host checks were repeated. Campaign
traces: Z7 `9c0aa4cee9b3a5e2a169b4e7baa616ec7ef0e730fb6a2555ee1fd2c1aaf4dbba`,
Z9 `5d9ef885a8c4f72025e44c6cf5143144736f281724acc2d10d8822d365b93b9c`.
The 576-capture custom trace for both binaries of both profiles was
`d5b7e4cbfc94d8090646ee1030990d60a6999f714c245c1258be5912ef8bc0e7`.
Each binary passed its reference oracle, both paired traces matched, and frozen
input manifests matched before and after. These are still constructed host
checks, not gameplay or performance qualification.

Normal ARM baseline and cache-candidate ELFs linked successfully under each
profile's release vector with the recovered banks, without bypassing link
reserve guards. Z7 BSS/IWRAM ends were `0203d46c`/`03004f88` baseline and
`02036694`/`03004f50` candidate; Z9 were `0203dce4`/`03004bf8` baseline and
`02036f04`/`03004bc8` candidate. Link outputs alone do not establish scratch
ownership, runtime correctness, or speed. The retained ordinary-input Z7 tape
is authenticated as mission 18, but the diagnostic renderer probe's fixed
9,020-tick limit does not cover its late busy portion. A current-codec
fresh-process continuation and suitably bounded paired measurements remain
open.

Targeted follow-up after input recovery: the private-output symlink safety test
passed with a real isolated `.local/build` directory. The Z4 and canonical
Zerg profile-roster subtests passed with recovered sprite banks. The Terran
and Protoss roster subtests still failed compile-time CV5 cover guards because
the available race, skirmish and terrain headers lack the required cover
encoding. An otherwise generated P7 BC fact header also differed from an older
recorded receipt. The original full-suite failure remains the full-suite
result; these targeted checks do not replace it.

A separate bounded **diagnostic**, not a production or hardware FPS test,
replayed the first 9,020 calls of the retained mission-18 Z7 ordinary-input
tape and measured calls 8,021–9,020. The tape SHA-256 was
`42e9a86021b3a16f6f67d2e19f3e475c4c98096799ce1abdc652f9117359696a`.
The private probe was linked from a generated copy of the current source with
read-only workload observation and diagnostic counters, and ran on the
receipt-matched 0.10.5 mGBA runner. Probe and input hashes were unchanged during
the pair. Both variants matched all 90 sampled state/display hashes and the
recorded workload and modal counts; those hashes are sampled evidence, not a
full state or byte-for-byte framebuffer comparison.

The measured window had 965 advancing PLAY calls, 35 nonadvancing PLAY calls,
zero text-box/modal calls, and minimum counts of 21 friendly units, 85 enemy
units, five friendly buildings and 54 enemy buildings. Resources were
1,399 minerals/150 gas at the start and 1,482/150 at the end. Advancing-call
mean/peak cycles were 1,298,955/12,898,972 baseline and
1,236,870/12,623,268 candidate. Nonadvancing mean/peak cycles were
2,545,321/3,291,038 baseline and 2,516,515/3,339,019 candidate. For all
1,000 calls, the 50th- and 10th-highest cycle counts were 1,665,516/2,818,446
baseline and 1,538,656/2,788,389 candidate. Snapshot work is outside the timed
call but may affect pacing. These measurements cover only this early window;
they cannot establish later busy-scene, hardware, or all-mode performance.
Private probe artifacts and logs are in `build/late-render-z7-early-v3` and
`build/late-render-z7-probe-v3-*` in the isolated workspace.
