# Attack-query cost outside active Z5 cloak

The regular Z10 compiler inlined the Z5 detector scan into its common
attack-eligibility routine. The inactive-cloak path still paid a
128-byte stack frame. Keep the detector walk out of line. With the
prepared Z10 inputs, the common routine uses 24 stack bytes and
shrinks from 1,264 code bytes to 380. Calls with active Z5 cloak use
the same detector scan and live eligibility rules.

All four normal ROM and RAM guards pass. The Terran, Protoss and
Showcase artifacts match release `71809328` byte for byte.
The normal Z10 build has EWRAM end `0x02039870` and IWRAM end
`0x03004f60`.

The 3,600-update three-CPU Blood Bath replay matches all 31
world and indexed display cuts, 16 fresh HOME continuation cuts,
all route rows and SRAM. Mean guest cycles fall from 821,028 to
819,722 (0.16%), and p95 falls from 1,110,401 to 1,108,653.
The matched 1,200-update saved Z8 continuation averages 906,851
cycles on each build.

The prepared sanitizer fixture runs the ordinary attack predicate
with active Z5 cloak, an enabled detector in range, destruction
and the disabled detector. An ordinary Z5 HOME load at frame 900
has active cloak. The two ARM builds match through frame 1,020,
including 960 Q8 energy spent. They write the same pause journal
and restore the same active cloak and world from a fresh process.
The 54 normal Custom Game pause and fresh HOME cases pass.

On the rendered Z7 tape, ordinary saves at controller calls 3,038
and 7,486 match byte for byte. Fresh HOME, followed by A to restore
the selected Hatchery on the original neutral update, reaches
gameplay frame 9,438 on both builds. The two cuts match 18 and five
subsequent named observations, respectively.
[Rendered input and tape provenance](2026-11-22-z7-rendered-replay.md)

The retained Z10 final-chapter tape still writes its ordinary
frame-2,526 journal on both builds. Its first divergent historical
observation is call 8,953, and the trace restarts at call 9,053 on
both builds. These are input-tape observations. A fresh winning
trace remains a separate campaign qualification.

| Local evidence | SHA-256 |
| --- | --- |
| Three-CPU Blood Bath comparison | `d90d6e4269ef055c92c348e1cce08a6598d13abbacac6f1c188fe034da2fcc99` |
| Z8 paired continuation | `53cf38895c3830588e9c22837caeb4f0d0249db20f042c1c3ed250d031afc260` |
| Z5 active-cloak control | `823d7d9afeb9bd01e3d829c866b508397eb43829d538abd38162ddffcf65537c` |
| Z5 active-cloak candidate | `3479ebebdcbfd31ecd1eebd63981546ecfd7b8c16a291bd99874b1e3d0c56230` |
| Custom Game fresh HOME matrix | `5bb9402da5e8b13e40b79332a306144386ba413cac5a4db3b1d8a5543c788e1f` |
| Earlier Z7 fresh recovery on the candidate | `80019871dc2c14f16f5d362a0ce92985ba5f40b33f5f7cc2b7f310562fa8b89f` |
| Later Z7 fresh recovery on the candidate | `7fc4d97c5b2360f0d7166f9fee9a62c4f0a150c2feabcfb4afaa7eb5020ae915` |

## Combined with the missile trail release

Against `6c9b4482`, Terran, Protoss and Showcase ROMs reproduce
byte for byte, and the updated Zerg ROM keeps the same RAM endpoints.
The common attack function still shrinks from 1,264 code bytes to 380.

The 3,600-update Blood Bath pair matches world state, route counters,
all 31 indexed display cuts, 16 fresh continuation cuts and SRAM.
Mean guest cycles change from 821,530 to 820,235 (−0.16%). p95 changes
from 1,111,198 to 1,109,367. The 54 ordinary Custom Game launch,
pause and fresh HOME cases pass across all three race ROMs.
The active Z5 cloak pair matches frame 900 through 1,020,
spending 960 Q8 energy and reproducing the same recovery SRAM.

A fresh Z10 controller tape, generated with rendered inputs, restores
the frame-2,526 pause in a fresh HOME process. Both ROMs reach victory
at frame 63,523 and match all 25 observed world fingerprints, both
indexed display pages at the checkpoints, and victory SRAM.
The controller tape SHA-256 is
`7ed5ef9774299acc9efb4b01986938fe1af30db3088094fc389b595b516fd7b0`.
The paired victory SRAM SHA-256 is
`6f368629fb129e2e48633af3df014925e88e374a8dc952ef8f489ced172d8dd0`.

The current Protoss 10 ROM also resumes from frame 36,761 and
wins at frame 46,057. All 68 world, indexed display and fog cuts
and the victory SRAM match the preceding world-depth release.
The measured 9,489-call tail averages 1,550,286 guest cycles per
main-loop call. Dense campaign pacing remains open.

| Local combined receipt | SHA-256 |
| --- | --- |
| Three-CPU Blood Bath | `2ceb7a3e659fceb00b8a459e627b2d464bd35f46c0a2a7e973a0064859758705` |
| Custom Game fresh HOME matrix | `1c50ff6ed86166dceefda1e5739fffe73ce6264ee8d78935ad801451aa88428e` |
| Z5 active-cloak control | `138e7f7a4e161f86b2ae959311a6888263ffe4e56da65b67d6864a912d99e124` |
| Z5 active-cloak candidate | `6b2e2493a4d80650bb7087ca2ee2f7e8fd870eace49f09ecf173ed9f2adecab9` |
| Z10 victory control | `da581c5c4a2d91607babf46b61c2dabffe2ab37a3deea8f1a98859a9b2cf6a5b` |
| Z10 victory candidate | `56b2da62685384fdbf59d22f0d04c5a9b314b9b3175b3c65898c4a51e82c86c2` |
| Current P10 victory and timing | `aeb93c63d55a0bac711100683d7da239dfa34b4b2a24bc0f327698575fea461e` |
