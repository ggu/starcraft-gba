# Early Protoss replays on November 17

The November 17 regular P10 cartridge at `fc0d5e1` won Protoss missions 4–6
from a fresh HOME screen in mGBA. The replay drove the existing controller
tapes through the normal buttons, checked every recorded observation and
dialogue acknowledgment, and stopped on the corresponding victory screen.
No campaign or controller source was changed to get these three wins.

| Chapter | Input calls | Victory frame | Matched observations | Briefing pages | Gameplay dialogue pages | mGBA cycles converted to updates/s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P4 | 130,239 | 129,310 | 146 | 7 | 13 | 15.30 |
| P5 | 5,172 | 5,057 | 7 | 12 | 5 | 18.30 |
| P6 | 101,370 | 99,906 | 113 | 6 | 17 | 12.56 |

These replay averages remain below 30 updates/s and are not physical GBA
measurements. This pass did not test fresh-process SRAM continuations for
P4–P6.

Fresh host controller runs with the regular P10 flags also won P1 at frame
25,621 (26,082 input calls; rescued hero alive), P2 at frame 33,803 (35,864
calls; rescued hero alive), and P3 at frame 45,951 (46,735 calls; no enemy
buildings). The P1/P2 input tapes are byte-for-byte identical to the archived
winning staged tapes.

P1 also won on the current regular ARM ROM after a fresh SRAM load. At
call 478 (frame 448), with no buttons held, START and pause-menu B wrote
the first journal to an empty cartridge. A separate emulator process
loaded it with L from HOME, matching the pre-save game observations and
all eight selected units. The unchanged input tape then reached the same
victory at frame 25,621 with all 27 checkpoints, all six briefing pages,
and all nine gameplay dialogue acknowledgments matched.

The P2 save made the same immediate round trip at call 440 (frame 372)
with six selected units. Checkpoint 1,000 matched, but at call 2,000 the
resumed run had three player buildings instead of five. An uninterrupted
run on the same ARM ROM passed that checkpoint with five buildings and
won at frame 33,803 with all 36 checkpoints, 14 briefing pages, and six
gameplay dialogue acknowledgments matched. The saved route stays open.

A paired 2,000-call trace isolates the first consequential P2 difference.
The live command menu remembered row 1 before saving; the fresh ROM reset
both the cursor and last command to row 0. At call 951 the same confirmation
entered building-placement mode in the uninterrupted run but attack mode
after loading. At call 1,042 the uninterrupted run placed a Gateway and
paid 150 minerals; the resumed run did neither. This is a dropped command
preference, with no evidence here of lost buildings at load. The existing
[opt-in UI context](../Z7-SAVE-UI-AUDIT.md) is the candidate to qualify
across current profiles before making it a regular build setting.

Rebuilding the same P10 profile with only `SC64_SAVE_UI_CONTEXT` added
left both RAM bounds unchanged. At the same P2 cut the tagged word
`0xa1010100` retained menu row 1 with no selected building. After fresh
title L, the same input tape matched all 36 checkpoints and won at frame
33,803 with the hero alive. The regular build still resets the row;
this result qualifies one experimental P10 replay.

The two P2 journals differ only in the tagged word and its envelope
checksum. Both P10 readers loaded both records and matched all sampled
world fields. Only the opted-in reader restored the marked command row.
Checksummed records with an invalid marker or a command row above the
limit left it at HOME, before any world state was applied. Other current
product vectors still need their own compatibility and replay tests.

Before enabling the setting by default, run the existing six-profile UI
host fixture, extend its coverage to the exact P10 and Z10 release flags,
and check ordinary title-load behavior on each current regular cartridge.
Include a legacy zero-word journal, a valid marked journal, a stale
selected building, and a corrupt newer bank with a valid older one. Check
both linked RAM bounds and a campaign and custom-game continuation.

P3 won uninterrupted at frame 45,951 with all 47 checkpoints, nine
briefing pages and three gameplay dialogue acknowledgments matched. A
separate P3 attempt saved at call 24,581 (frame 23,993) before placing
the expansion Nexus. Fresh title L restored the named world sample, but
the next checkpoint at call 25,598 had 2,375 minerals instead of 2,475.
This attempt does not qualify a full P3 SRAM continuation, and the P2
command-menu finding does not yet explain this difference.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Regular P10 ROM | `a4c5c93c307bc275aca13a61e4e2526b1fabccfdfa4465382fa72afd28be0dff` |
| Regular P10 ELF | `142c4c0ebde19e05972d7bf931e31291c36ef23ba29a5129d043873720a61a57` |
| P4 ARM receipt | `453910dd2232230f3f3d129d3a98ce6474ab03c5ce3f87ccc0513505800a8ea0` |
| P5 ARM receipt | `48dd677bdc4215c416c18bcc10dd0026a25d98017d7195194c5e8604f078b65b` |
| P6 ARM receipt | `b6be0f54215551d7068b20934bf5af33ccaf9db9c0cc826a2814b187a8cb9480` |
| P1 fresh SRAM win receipt | `f0a84a77b7159b6d109532c16dfc93bcdbd4d5e7df9a70d86614b66788529861` |
| P2 uninterrupted ARM receipt | `2a961c2f3d49463e85724ec216d7db41a3a1add9af93c31fae1b36746a85cf9d` |
| P2 first divergent checkpoint | `4c0b3a920082da3b7dfd1991aaa799fb70d29f549a6cb9111904e46f410cca40` |
| P2 paired command trace comparison | `fcc66f0e866719eb31d44246d9703b1c6cd665d9630cccc573fa133b76261cf1` |
| P2 opt-in P10 ROM | `c28cc7a024247cc542f0b1afde89983930b34304215832b52ed0a8082be447b8` |
| P2 opt-in fresh SRAM win receipt | `f68a5be5dde18a6d2880790b682ad43eb9d0b2217fb8b6f5a11d90f368331ee1` |
| P2 P10 reader compatibility receipt | `7279a5878663bc3ad1c8dad5c1b48f7749c294086f1d7349e3486c66f9f189e0` |
| P3 uninterrupted ARM receipt | `d5fb7257c27525d3082e5e980fd78553f32ec90ddce74f93527ec3b5e3bac527` |
| P3 save cut receipt | `1f844395fe68f26df117dadea3ca021fd092133faff5ac01c4ef3cfaab5b3345` |
| P1 host input tape | `70259667d448fabb7efc898e7674c6f051ac87f9e3f65ab414069bde818d94be` |
| P2 host input tape | `a1ab751c4e7cab20c1859405453a29196ab8e8c749da6296347b390f8652ab0b` |
| P3 host input tape | `0a0540f40220daf10fa88c682caaccf0064128a2ceeb68d84dd854e5062cc95b` |
