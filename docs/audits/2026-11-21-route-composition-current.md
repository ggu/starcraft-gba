# Current-source route composition: host and Z7 qualification

This opt-in composition combines the cold disconnected-component bypass
`6f836320`, warm blocked-goal gap retry `ec43748f`, and small-component fill
`c59e0963` with source release `5516dffa`. The tested composition is
`e91f735358c37d6ea867b5987a702935d14b8288`. No production flags were enabled.

`make test-sc64-route-composed` runs all three flags together under address and
undefined-behavior sanitizers. Its six route checks pass, including exact
384-cell and rejected 385-cell components, blocked roots, stripe escapes,
occupied reserves, stale owners, and canonical route comparisons. The
flags-off default ROM and a 321,445-query route trace match `5516dffa` byte for
byte. `python3 tools/check_source_tree.py`, `./dev doctor`, `./dev build`,
`./dev test`, and `make test-sc64-route-keep-fields` pass. The host suite's
exit status was observed directly; its complete output was not retained.

The current-source Z10 link uses the existing pinned input and toolchain vector.
It passes ROM and payload verification and RAM guards. The normal #671 ROM is
`ad2709458c154142fb776a096d42a444baffe98455832bc0e647a377633bb80a`; the
composed ROM is
`989eb1911da12ea69020c0410d5392b9f84e8c611e27808903c506518083b73c`.
The normal and composed IWRAM endpoints are `0x03004d60` and `0x03004ea8`;
EWRAM endpoints are `0x02039b30` and `0x0203a068`, respectively.

Both paired ordinary Z7 runs start from blank HOME, pause at frame 5,641,
restore in a fresh process, follow the same controller through call 231,031,
and win at frame 228,128. The pause and victory journals and fresh victory
readback match byte for byte. All 254 sampled world observations, six logical
cache cuts, 75 complete fields and 4,201 published partial directions match;
the latter use an independent BFS oracle. Nine of ten paired display cuts
match. At call 154,741, camera, palette and both framebuffer page hashes
match. Normal selects page 0 and composed selects page 1, so the displayed
pixels differ at that input sample. The VBlank observer below establishes
when each run presents its page.

| Window | Advancing calls | Normal mean / p95 / peak cycles | Composed mean / p95 / peak cycles |
| --- | ---: | ---: | ---: |
| Dense | 477 | 2,193,387 / 3,181,266 / 5,120,617 | 1,910,744 / 2,580,213 / 4,482,609 |
| Late peak | 926 | 2,449,609 / 3,733,549 / 5,811,026 | 2,355,215 / 3,330,589 / 5,244,636 |
| Late following | 905 | 2,381,249 / 3,624,506 / 5,365,242 | 2,280,739 / 3,183,649 / 4,498,748 |

These are emulator guest-cycle samples, not hardware performance. Other
campaigns, production build modes, full display equivalence and physical GBA
qualification remain open.

| Local evidence | SHA-256 |
| --- | --- |
| Normal Z7 result | `a7fc7bcbecb1ef79cdcd1990cdcd2d4bdef721215a5ef22aec9a87e7f2e67328` |
| Composed Z7 result | `648119ceaede4cfd9d110325ebb8b7ce14d59c5d4aa6efc1eee1162ff022d0fd` |
| Paired comparison including display mismatch | `8a06eedd3b8b264491dc0e757fe8ad6e8429101f1a8b7e6cd2b806d21885d392` |
| Pause journal | `4eaecbb90f5e923d73eb97a68012f5fb653d8c751fba7dc1e4d9d80ae54ca3ce` |
| Victory journal and fresh re-export | `1acce341f61b2a1e37cadc1e0b304dd63b5b31352789d128fa2d7bf20d6eb627` |

## Shared build option and current-renderer checks

Set `CANONICAL_ROUTE_POINT_PREFIX=composed` when building a trial
cartridge. The Makefile expands it to the same 13 probe flags as the Z7
experiment above and includes the choice in the build key. The default
remains `0`. The current source with the trial off reproduces the public
asset-free ROM byte for byte. The focused route sanitizer checks
pass with the composed flag set, including far-read and range-work
coverage with all of the composed flags enabled. That check compares
108 randomized full fields and 8,320 route-copy alignment cases.

All four prepared trial links on top of `878eea80` pass ROM, payload,
and working RAM checks. The current Showcase link ends at
`0x03004fc0`, 64 bytes below the IWRAM code limit. Ordinary mGBA
inputs match the release at both sampled updates of the three campaign
openings and first Showcase scene. Both maps on all four profiles
match sampled world state, the active page, both indexed pages and the
palette. A separate 54-case Custom Game matrix passes ordinary pause
saves and 120-update fresh HOME continuations, sampling both maps,
all human races and one to three separately named opponents.

The trial Terran cartridge wins T10 at frame 3,361 after a pause and
fresh load. A separate HOME process reads the terminal journal. Both
P10 cartridges restore frame 36,761 and win at frame 46,057 on the
same retained controller tail. All 68 world, camera, both-page and
palette samples match and the victory journals are byte-identical.
Over the 9,489-call P10 tail, measured guest cycles fall from
17,220,766,078 to 17,215,672,547, a 0.03% reduction.

Current Z8 restores its ordinary HOME journal at frame 16,659. All
17 state and indexed framebuffer samples match, including the
palette and route cache metadata. Three modal boundaries and all
203 timed logical route rows match.

| Z8 sample | Release | Composed |
| --- | ---: | ---: |
| Mean update cycles | 1,694,664 | 1,551,950 |
| p95 update cycles | 3,230,138 | 3,207,817 |
| Peak update cycles | 12,062,422 | 10,179,654 |

The mean falls 8.42% and the peak falls 15.61%. The mean still
exceeds the 30 fps budget by about 2.8 times.

The current-renderer Z7 pair matches the frame-5,641 pause journal,
all 254 world cuts, ten indexed display cuts including the active
page, and every timed route counter. An independent breadth-first
field check validates 4,201 published directions on six partial
fields. All 75 complete fields match at the six cache cuts. Both
runs finish the controller tape at call 231,031 and win at frame
228,128. The result journal and its re-export from a new HOME
process match between both builds. Physical GBA timing remains open.

| Current Z7 window | Advancing updates | Release mean / p95 / peak cycles | Composed mean / p95 / peak cycles |
| --- | ---: | ---: | ---: |
| Early dense | 477 | 2,193,883 / 3,179,446 / 5,109,060 | 1,911,338 / 2,580,850 / 4,479,095 |
| Late peak | 926 | 2,449,994 / 3,734,509 / 5,812,547 | 2,355,802 / 3,331,715 / 5,243,801 |
| Late following | 905 | 2,381,799 / 3,619,987 / 5,365,789 | 2,281,009 / 3,190,682 / 4,497,029 |

The mean falls 12.88%, 3.84% and 4.23% across these windows.

In a resumed three-CPU Blood Bath, 3,600 route counter pairs, 31
world and projectile samples and the saved journal match. All 16
fresh-resume comparison cuts match over the subsequent 240 updates.
Both indexed pages and the palette match at all 31 original samples.
At update 2,280 (simulation frame 19,585) the release displays page
0 and the trial displays page 1. These are different page contents
at the input sample, so the strict display comparator detects a
difference. Mean cycles rise 0.052% from 924,956 to 925,439.
This 64×64 map exercises the fallback route path.

An independent diagnostic reproduced that cut and stopped at VBlank
entry, VBlank return and the ordinary input boundary. At frame
19,585 the trial has completed its pending flip to page 1. The
release still displays page 0 with its flip pending. The release
flips to page 1 with the same page hash at its next VBlank. Both runs
display page 1 with matching page contents at the next logical
input sample. This identifies a one-VBlank presentation phase
difference for Blood Bath.

| Current-renderer receipt | SHA-256 |
| --- | --- |
| Composed host checks | `3cf38629f8158839ce4573d051381ea38a2acde5d06647e50ab49f67ea9a1041` |
| Composed host checks with range work and far-read | `04f6508f02dd5c1faf824c8e0231d1daaaf984981b64c7fa9610554f496efc82` |
| Four profile openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Both Custom Game maps | `21a71ce5db13778d0fc87f91dd6ccb8ad496613be453d3a158884e50e75ea5f3` |
| Custom Game fresh continuation matrix | `0b5f0e0034be519a321c62d7e9416db33d7c8c06f14603c4368f3bec507d2155` |
| T10 victory and journal readback | `3095bb1de7509ce1cefa971499aeb2377f4c2f4a3e44dc25453f83a45c41e67e` |
| P10 paired campaign tail | `6d14a579c7851da22dcdd118ccfe1c0a2a319b7b1fe02f50688709d760d092d8` |
| Z8 comparison | `c081897aa190d53986f4be6ddc22793bb5fa56c8c787db765780122a6e9e0b83` |
| Current Z7 comparison through call 155,650 | `3c807c9fc00361373aab4e80ec92dba152bf125c07b389af55edd01207ffd0be` |
| Current Z7 full victory comparison | `feed2bcba11b8dbba5cd98540a1b17eae570fafe2b110d36d7e12f379b8d88c8` |
| Late Blood Bath comparison and active-page difference | `4f2347a4854f2c9677ac57a2f31b797396caa00a94aaa8de6fc4403cbad192e2` |
| Blood Bath release VBlank observer | `f03446fc3d633230adf2d9718e2a2c376edcd30e3a76b66b05c0cc67e001cbe2` |
| Blood Bath composed VBlank observer | `d16d75682f814538a75ef8cb90234a4229cca90b8f4b40b2bad97fa8e9591270` |

## Dense Z7 presentation phase

A bounded replay of the earlier Z7 pair stopped at the ordinary
input sample and on either side of the VBlank presentation handler.
At call 154,741 the normal run completed its flip to page 0 9,434
guest cycles before the input sample. The composed run still had its
flip pending at that sample. It presented the same page at its next
VBlank, 35,921 guest cycles after the sample. At call 154,742 both
runs display page 0 and queue a matching flip to page 1. All nine
observed flip payloads and palettes match in order. This comparison
resolves the sampled phase difference. It covers calls 154,737
through 154,746 on the earlier composition.

For future display comparisons, preserve the exact input-boundary
observation. If the two contents differ only in page selection,
record both runs across the relevant VBlank with the pending flag,
displayed page and both image hashes. Compare the presented sequence
and the next input sample before diagnosing a draw-order regression.

| Local receipt | SHA-256 |
| --- | --- |
| Earlier Z7 release VBlank observation | `6b79fa7c725ada8e03c3d74fea05c0c4ad904b438b7aa47122c5aa946a8d70cf` |
| Earlier Z7 composed VBlank observation | `9908c40124f4d16077cf2ab19765a0e0531ee03b2e215f1003753de6ca9f56e8` |
| Earlier Z7 presentation comparison | `85c3ece31b8f8b478e6b2065518ec7c261370fff855e385a75e79e82b9c8dacf` |
