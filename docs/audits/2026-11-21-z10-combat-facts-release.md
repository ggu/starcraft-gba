# Normal Z10 immutable combat facts

The Z10 recipe generates one byte of immutable combat facts per unit
type. The target policy keeps live checks for visibility, cloak,
transport, invincibility, construction, and flight. The bank retains
the existing actor values and adds the combat table and its guard.
`Z10_COMBAT_FACTS=0` selects the previous policy. The same switch
controls the normal build, generator, sanitizer oracle and build
signature. Terran, P10 and Showcase retain their existing build vectors.

The prepared ARM links use the current packed buildings, actor bank,
audio and Carrier policy from `725ce67e`. The rollback link regenerates
the normal actor bank and reproduces the release ROM byte for byte.
The optimized link retains IWRAM `0x03004d60` and EWRAM `0x02039b30`.
Both links pass the cartridge, SRAM signature and ELF payload checks.
The focused fact oracle passes 2,739 comparisons under ASan and UBSan.
Terran, P10 and Showcase all rebuild byte for byte against the current
production ROMs with identical RAM endpoints.

## Normal Custom Game

The Blood Bath pair loads the retained compact frame-17,305 journal
with ordinary HOME L. Zerg is the human race and the independent CPU
seats resolve to Zerg, Terran and Protoss. From that active-shot cut,
both ROMs take the same inputs for 3,600 advancing updates. They match
31 world, projectile and indexed framebuffer samples, all
logical route counters, and the new 32 KiB journal at frame 20,905.
The journal loads in a fresh process and matches 16 sampled world
and projectile cuts over the next 240 updates.

| Blood Bath updates 17,306–20,905 | Rollback | Combat facts |
| --- | ---: | ---: |
| Mean cycles | 925,008 | 922,934 |
| Nearest-rank p95 | 1,218,223 | 1,215,249 |
| Peak | 4,118,623 | 4,116,527 |
| Mean orders cycles | 285,329 | 283,579 |

The sample has up to 89 live units, 34 actors with weapon cooldowns
and five simultaneous active Interceptor pulse records. The mean
decrease is 0.22%. Separate blank-cartridge starts cover Terran with
one Protoss AI on Outpost and Protoss with Terran and Zerg AIs in
nonadjacent Blood Bath seats. Both pairs match full SRAM, resolved
starts and 240 advancing updates after fresh HOME L.

## Campaign

The current Z8 pair loads the same ordinary frame-16,659 journal and
continues through call 33,900. It matches 17 world and display cuts,
three controller-acknowledged modal boundaries, and the frame and
logical route counters for 203 timed advancing updates. Mean cycles
fall from 1,696,432 to 1,694,349 (0.12%).

The exact normal Z7 pair starts each ROM from blank HOME, writes the
same frame-5,641 pause journal, and resumes in a fresh emulator. An
ordinary A press reselects the Hatchery. The 254 world checks, ten
display cuts, six route-cache cuts and sampled logical counters match
through victory at frame 228,128. Both write the same victory journal,
restore RESULT with fresh HOME L, and export that journal byte for byte.

| Z7 tape calls | Mean cycles, rollback → combat facts | p95 cycles, rollback → combat facts |
| --- | ---: | ---: |
| 47,758–48,237 | 2,193,528 → 2,193,360 | 3,182,692 → 3,182,885 |
| 153,812–154,741 | 2,492,987 → 2,424,115 (−2.76%) | 3,766,802 → 3,712,728 |
| 154,742–155,650 | 2,419,168 → 2,343,482 (−3.13%) | 3,666,292 → 3,595,765 |

The Z7 p95 values use inclusive interpolation on advancing updates.

## Protoss 10 comparison

A separate opt-in P10 link preserves all existing V2 actor and sight
data and adds only the combat facts. The sanitizer oracle passes
2,775 checks. Both P10 builds retain the same RAM endpoints. Their
active-shot frame-37,589 journals and 36 fresh continuation updates
match. The 12,512-call ordinary Blood Bath build-order prefix matches
the wallet, world, camera and display and is effectively neutral in
cycles. The paired campaign tail takes 9,489 calls from fresh HOME L
at frame 36,761, with 22 matching world and display cuts and an
identical victory journal at frame 46,057. Its mean falls only 0.074%.
The normal P10 recipe keeps its existing configuration.

| Receipt | SHA-256 |
| --- | --- |
| Restored rollback ARM ROM | `d664d71c80a6aa959cc98c018e596d69726a6a0c00570873e2d60e50fdc1a5d4` |
| Optimized normal Z10 ARM ROM | `dfb2e04b3858f849e0ebb19531782efb03c9413986a61767aebfb7babf8af162` |
| Optimized Z10 ARM link | `6a9803fe7f9b3e0007909f654a97d759a1db29079d36a545025c155ce81ce2f3` |
| Default build recipe and HOST oracle | `e5a4465e47b6c943cde10166c129dfd96aebfd408aadcd526ee31ece8052e946` |
| Other normal product rebuilds | `baaa1111e79154edab06f0a62c6b46d665cd8c3e54c8f4a97d8ab3674ea494ea` |
| Three-AI Z10 Blood Bath comparison | `83aa0c9ceecd55620cb7371ffd1337a437220783b1d5bcfc46814bf02eff1724` |
| Additional Z10 lobby and save comparisons | `6335f1bde8a3c4f780bee144a327972562c3ee5f091db3eba4705f82afa688c3` |
| Current Z8 campaign sample comparison | `7c58ba2257bd8f96dfe78c7aa7b0d2b342389b02a74d30d1bcb3d347c809af69` |
| Exact Z7 victory and timing comparison | `ba8f40bdcc2de712d4e3bea3505a540ce63fffbf1d6a07ea406dc8b1169f19c2` |
| Z7 victory and fresh HOME re-export journal | `1acce341f61b2a1e37cadc1e0b304dd63b5b31352789d128fa2d7bf20d6eb627` |
| P10 opt-in ARM link | `b148dde9067599c17fa8e3fdbdccfacd0f97b50cf47106b496e9a8cbd2a77851` |
| P10 campaign tail comparison | `d973b5285d93354ba22ee8c0347b927714a82c0682f75e11cdb104957c0aec4d` |

Cycle counts measure emulated guest work including presentation waits.
Dense gameplay and real-device frame pacing remain performance gates.
