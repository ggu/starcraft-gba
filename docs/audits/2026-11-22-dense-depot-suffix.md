# Dense depot flood continuation on Blood Bath

The regular Zerg cartridge reloads an ordinary three-CPU Blood Bath
journal through HOME and advances 3,600 neutral updates. At simulation
frame 19,360, the walkability grid opens six cells after a Sunken Colony
is destroyed. A read-only mGBA observer measures 34 depot field lookups
across three depots. The facility calls take 1,102,115 guest cycles,
including the first lookup's grid rebuild. All five external frame
clocks, route counters and world observations match the uninstrumented
replay.

Twenty-eight lookups hit an existing field. Three fills start a new
field and three continue a parked queue. The first reachability check
for depot 2 discovers 2,595 walk cells and takes 652,457 cycles. Its
next fill resumes from that frontier and discovers 97 additional cells.
All 34 requests use the depot circle. These costs make the long new
expansion the first target.

## Shared route change

For a parked-capable 64×64 facility search still expanding after 768
pops, translate the remaining availability bits into the existing
padded-grid scratch. Continue the same FIFO with the existing pop and
neighbour order. The byte grid records only new directions. A sparse
suffix updates its newly discovered cells in queue order. A longer
suffix packs rows and merges the new directions into the prefix,
restoring the exact remaining availability bits. The queue keeps its
original cell indices throughout. The next request can resume the same
parked frontier.

The threshold is a measured tradeoff. A 384-pop trial cut the longest
depot lookup more but raised a shorter resumed lookup from 147,342 to
251,050 cycles. A 512-pop trial with sparse publication improved the
overall peak by 3.26%, with both indexed pages identical at 31 cuts.
Its displayed-page bit differed at one cut. The 768-pop version
matches the displayed page at all 31 cuts. It applies through the shared
route code whenever the existing 64×64 frontier and dense-grid features
are active.

| Busy Blood Bath metric | Current release | Dense continuation |
| --- | ---: | ---: |
| Mean guest cycles per update | 812,646 | 811,651 |
| Nearest-rank p95 guest cycles | 1,102,874 | 1,099,651 |
| Peak in-game guest cycle counter | 3,677,137 | 3,566,597 |
| External clock at frame 19,360 | 3,677,630 | 3,567,090 |
| Inclusive depot field lookups at that frame | 1,102,115 | 991,445 |
| First depot 2 lookup at that frame | 652,457 | 528,947 |

The mean falls 0.12%, p95 falls 0.29%, and the peak falls 3.01%.
All 3,600 build/hit counter pairs, 31 world and indexed-video cuts,
16 fresh title continuation samples and the entire exported save match.
The breakpoint observer reproduces all 34 query results, route slots,
frontier sizes and discovery totals in the peak update. The instruction
observer measures the same lowest system stack pointer, `0x03006950`,
and IRQ stack pointer, `0x03007e98`, on both builds in that update.

The asset-free facility oracle passes six sanitizer variants. It checks
packed directions against an independent flood, including multi-depot
resumes that park after the dense transfer. All eight normal and preview
Terran, Zerg, Protoss and Showcase images pass the link guards with
identical RAM endpoints to their respective control. The 9,489-update
regular Protoss 10 victory replay matches all 81 world, fog, HUD,
framebuffer and route cuts and the victory journal. It still averages
11.01 emulated updates per second. Hardware frame pacing remains open.

All 30 campaign entries start through the three normal HOME menus and
advance gameplay after acknowledging any opening dialogue. Both sets
pass 54 Custom Game start/save/reload cases each. Normal Showcase Free
for All and preview Showcase Melee each pass a three-CPU Blood Bath
continuation. The asset-free ROM is byte-identical to the control.
The full local `./dev test` on `03288bd0` passes, including the
1,176-test Python suite with 35 skips. It runs in an isolated RAM disk
checkout to avoid the internal disk limit.

The prior [native depot contact audit](2026-11-21-blood-bath-worker-opening.md#contact-follow-up)
measures N64 worker contact at a structure footprint. This change
preserves the current GBA deposit geometry. Keep contact parity in the
worker follow-up. For routing, prioritize earlier proof of reachability
when openings leave a valid old route, without skipping a nearer depot
that became reachable. The measured peak still includes about 870,000
cycles of separate point floods.

## Reachability after the Blood Bath opening

A second read-only probe stops just before frame 19,359 invalidates
the existing cache. The three partial depot fields already contain
positive directions for every distinct origin queried at frame 19,360.
The separate old point fields also contain a walkable witness from
each origin to a legal goal cell. The grid diff is six openings in a
2×3 rectangle and no closures. A scalar multi-source flood over both
grids reproduces the old stored directions and finds the following
changes among previously discovered cells.

| Depot slot | Distinct requested origins | Old cells discovered | Old cells with changed first steps |
| --- | ---: | ---: | ---: |
| 0 | 4 | 109 | 0 |
| 2 | 16 | 2,705 | 2 |
| 3 | 10 | 660 | 0 |

None of the requested origins changes its first step in this sample.
Neither of the smaller fields touches an opened cell. Ten discovered
cells in the larger field neighbor an opening. Wholesale reuse of the
larger field would yield stale directions at two cells.

A retained path can preserve its exact direction when its original
length is strictly below the lower bound on any path through an
opened cell. With no closures and identical root cells, use the sum
of the Manhattan distances from the origin to an opening and from
that opening to its nearest root. Strict inequality also excludes
changes from equally short routes. This bound certifies 29 of the
30 observed origins. The expensive first request for depot 2 has
length 92 and an opening bound of 123. The one uncertified origin
needs the ordinary field query.

A private source trial records positive reachability for workers with
an existing compatible depot, using the current origin cell, the
depot geometry and a route field's discovered entries. A rebuilt grid
with any closure cancels the proof. Depot ranking, ownership checks
and movement still use the ordinary engine paths. Limiting the trial
to building destruction changes the 3,600 paired update results as
follows.

| Metric | Dense continuation | Opening proof |
| --- | ---: | ---: |
| Mean guest cycles | 811,651 | 814,494 |
| Nearest-rank p95 guest cycles | 1,099,651 | 1,123,064 |
| Peak guest cycles | 3,566,597 | 3,279,220 |

The original peak improves 8.06%. The first trial, which considered
all invalidations, avoids 32 of the 34 peak depot lookups. Its next
update rises from 1.67M to 2.20M external cycles. The destruction-only
trial preserves every sampled world, visible and indexed page,
palette and SRAM value. Route counters change when a proof bypasses a
field query. Mean and p95 both regress, so keep the proof out of the
cartridges. Next measure retaining the exact directions of untouched
frontiers and certifying a retained direction only when an opened
route cannot tie or improve it. Compare the following update and the
whole replay to account for route-cache warming.

The previously failed N64 Wasteland observer also runs again when the
child receives the installed ares MoltenVK framework path. The isolated
9,000-step normal-input replay from the archived USA ROM reports GPU
ready and reproduces all three frame hashes and the final RDRAM hash
from the retained selection sample. This confirms the recorded
Wasteland witness. The same input in the current Terran candidate
selects all five Marines and leaves all five SCVs unselected. It
matches both unit-list observations from the prior Terran build.
Blood Bath native mining contact remains a separate comparison.

## Fresh distant requests on the pacing release

The follow-up starts a bounded 64×64 facility search in byte-grid
scratch when the first origin is at least 48 Manhattan cells from the
facility center. Nearby and resumed requests retain the 768-pop packed
prefix. The threshold selects the scratch representation. Both paths
keep the same seed order, FIFO pops, neighbors and stopping rule.

The paired regular Zerg match continues the documented Blood Bath
journal from gameplay frame 17,305 for 3,600 neutral updates. Both
builds take the same six-cell opening at frame 19,360.

| Guest cycle metric | Merged #724 | Fresh distant transfer |
| --- | ---: | ---: |
| Mean per advancing update | 810,180 | 808,903 |
| Nearest-rank p95 | 1,096,267 | 1,088,201 |
| Peak at frame 19,360 | 3,565,643 | 3,440,685 |
| All 34 facility calls at the peak | 991,833 | 873,492 |
| The first large depot 2 fill | 528,988 | 405,319 |
| Following update | 1,663,631 | 1,664,057 |

Peak, p95 and mean fall 3.50%, 0.74% and 0.16%, respectively. All
3,600 route counter pairs, 36 sampled world, two-page and palette
cuts, the exported pause save and 17 fresh HOME continuation cuts
match. The breakpoint observer matches each of the 34 facility query
results and parked frontier records at the peak. The long depot 2
lookup leaves the same 25 pending entries and 2,595 discoveries.

All six facility FIFO sanitizer variants pass. All eight normal and
preview builds match the previous IWRAM and EWRAM endpoints. The six
12,000-update Custom Game pairs match their 120 sampled world,
route, palette and two-page cuts and stay within 0.05% in full-run
mean timing. The ordinary 9,489-update Protoss 10 victory matches all
81 sampled state and display cuts and its victory save. Terran 10
wins from a fresh ordinary HOME load with six matching checkpoints
and byte-identical victory readback. The 48 Custom Game start/save/
reload cases pass on both variants, all four products and both maps.
Retain a small disconnected-region holdout for future threshold
tuning. The hardware pacing check remains open.

The normal-button startup pass starts all 30 chapters, reads each
briefing entry, and advances beyond the opening dialogue. The
previously pinned Z1 HOME tape reaches victory at frame 50,669 with
50 matching periodic checks and the Chrysalis alive. The separate
opening save at frame 97 reloads and matches the next 120 neutral
updates. A full same-tape reload from frame 984 first differs at
frame 1,422 when the uninterrupted A action spends 50 minerals.
Both #724 and the candidate clear the selected Hatchery on reload.
The replay's next A selects it, while the uninterrupted A queues
another Drone. The restored queue, wallet and world match through
call 1,437.

The successful fresh-load replay replaces neutral tape ticks 1,000 and
1,001 with a physical A press and release. The cursor still points at
the Hatchery. This restores the building selection within the same
two gameplay updates. All 49 remaining checkpoints and the terminal
state match. Z1 wins at frame 50,669 with the Chrysalis alive and
30 Hydralisks. A third fresh HOME load confirms the terminal state
across eight neutral updates and exports a byte-identical 32 KiB
victory readback. Physical-call counters in the receipt restart when
the fresh process opens.

The two normal Showcase ROMs run all eight scenes from their hidden
menu. All sampled world, two-page and palette fingerprints match,
with the same losses at update 150 in each of the three Last Stand
scenes and matching scene restarts. An ordinary Blood Bath scouting
replay selects and moves an Overlord, then pans the camera along
its route. The 4,800 advancing updates match all 41 sampled world,
fog and display cuts. Mean cycles fall from 791,050 to 789,686.
Four cuts see other units in the view. Use a sustained visible-fight
workload for further actor-rendering qualification.

The fresh asset-free suite passes on runtime `80047031`, including
1,176 Python tests with 35 skips and the native sanitizer checks.
The default ARM image is byte-identical to #724.

| Local evidence | SHA-256 |
| --- | --- |
| Control Z10 ROM | `00b7bd4e6796519319a0e5470cc4f82ee502cdc29b96b543476bde1a5f23777a` |
| Dense continuation Z10 ROM | `c86d475fd3c9edc46af5642e463c06c5c25a91caa790c4ff0764c0e666503d2e` |
| Control depot observation | `a116f19846959f6b0cbfd43d700ca0dbdb4acc5999b9ad5335f151ae4bea9ebb` |
| Dense continuation depot observation | `cd511b519604ac3dc18cb8b00ae936f034d42b61be8b53b874c75499f40a6a60` |
| Paired stack observation | `3bb681dffc60ea44a57fc5cbee9736bfcbe07c70f73963fd300a0b026eaa03d2` |
| Dense Blood Bath replay | `fe78614d6febc993a31221da3d167a061151eaaff650fd268c6518be1295bd3a` |
| Protoss 10 replay | `a9f38617e2dc39f3f1b55e53fd4e068c5e0d55d5a010081c2cb7fb4acc81e596` |
| Pre-invalidation route observation | `3a78f9deb87921bcf59ceaece6d6b936be40dfa76bf6f75b29f3dd7f044ab8e9` |
| Destruction-only proof replay | `00dbe1514b06602513c13ece8cd34e88139b2af1f71e60971212fd713ebe1fe3` |
| Fresh native Wasteland final RDRAM | `39d363a89f1c3f6cc3b9ce06c7ea2742d036e5ae460e6a3f2f3bf1bc1406240d` |
| 30-entry physical-button campaign start matrix | `bf2ee7dee428996c5523ce59c6ba8e4f9390c8b20359396ceade90dcaf7a761c` |
| Asset-free suite log | `477f54803f37534a76b8a13fa48ac4931c6f68ceb3ddf69a22531c0d0b4034b4` |
| Opening-path bound sample | `ebceed3322d90bc376b61c67ad96ec9f7c572704689d6456826b2b4c18169164` |
| Current Terran Wasteland selection reply | `c54bf2c312d2e57831d3e17d7657219c7f6f77318b47c3d44ca999150c8530ee` |
| Fresh distant transfer control replay | `f48c27409631ced6b4485033309a5f62cc14a25d71999f021bb597c8dda0c156` |
| Fresh distant transfer candidate replay | `bd96a1c3151637b9f6fbb902e7801a94e3f9bbecdc755360c046df55ef993d8f` |
| Fresh distant transfer control call ledger | `2acbee36289e6f027145cf77290b2d61d6ff7e0d23016723fb827248bdd9731e` |
| Fresh distant transfer candidate call ledger | `ea692573028d79b7dc829c1dc10dcc58a49ce22a19ce9465622fb45ae3bd7709` |
| Follow-up six Custom Game replays | `175d235997b28ebf52d0679546003c814861d3d703d1ff0b08daad0a37a5ad59` |
| Follow-up P10 replay | `a1a61c53469792bf5124a33a6f9904b47873a87143aeaacda41be7af18b306e7` |
| Follow-up facility sanitizer log | `007a73ea5c5ea435c41b37134e768b800ed662a7aad241d28f776fde865e980e` |
| Follow-up 48-case Custom Game save/load matrix | `4a15eaf37f9e32aaf44c38bce41782a22244c4c8a77fcf4044e693ed8ab803c1` |
| Follow-up Terran 10 fresh-SRAM victory | `c100e289d903babf4e80e1194f173c6e4abc298c932eecc51f27d7b6826d422c` |
| All 30 candidate campaign starts | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Candidate Z1 pinned HOME-to-victory tape | `3c0b07f7408e478b6c035f08c91fabb55f088dfdba3965e2126817f6e180789f` |
| Candidate Z1 opening fresh load | `8755e6931dcbcf5591c98891b0ece0b81f834cecb8b38a0419d98e48dd12e436` |
| Candidate same-tape Z1 resume mismatch | `677c2a8d8042a99693b6d7c23f54e4df0330ea20644e67011b3bd66c5a139427` |
| Showcase scene control | `8adb5e944d012edbc41f5cd19e702bfbdeed8ada21428d9f0205f2d271ca732b` |
| Showcase scene candidate | `34f08029ad488d7228ace20012b0f7826009ec08e50fc8685858222e0b678298` |
| Blood Bath scouting control | `6bd04d92180be267ad0c7e45dbad4dc439d6fb5c00684c73ebac8a61b02fba83` |
| Blood Bath scouting candidate | `409c7d0411084981e63ccdc3dd3b8e7aa535692c65d24246a4e3929685488da0` |
| Z1 fresh load, Hatchery reselect, victory and fresh readback | `b42e444ad2ec04811bb2df975445a75456f4359dd6d922a7c382e638720eb256` |
| Z1 identical victory SRAM and readback | `fd6b53a2e5be9d898d44e93c39e203a6148a3f7f4efc7309a77ca29036b936d2` |
