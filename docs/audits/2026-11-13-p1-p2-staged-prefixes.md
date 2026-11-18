# Retained P1 and P2 tapes on staged P10

The pinned P1 and P2 tapes won on an earlier carried P10 test build. The
read-only `tools/sc64_early_protoss_home_replay.py` verifier booted the
stock staged P10 ROM from HOME, navigated to each mission with ordinary
buttons, acknowledged the briefing and in-game pages, and stopped on
the first mismatching checkpoint. No guest writes or save inputs were used.

| Mission | Tape call / simulation frame | First differing field | Earlier tape | Staged ROM |
| --- | ---: | --- | ---: | ---: |
| P1, First Strike | 3,000 / 2,982 | Enemy units | 66 | 65 |
| P2, Into the Flames | 1,000 / 978 | Minerals | 86 | 88 |

All other compared fields agree at those checkpoints, including the
simulation frame and zero trigger faults. P1 also matches the complete
checks at calls 1,000 and 2,000. P1 acknowledges six briefing pages and
three authored gameplay pages; P2 acknowledges fourteen and one.
These are stale-tape failures, not observed mission defeats or proof of
the runtime cause. Generate fresh route-pure HOST input with the staged
P10 flags and retain its ordinary briefing and modal input events.

| Local artifact | SHA-256 |
| --- | --- |
| Staged P10 ROM | `0505b233994dca90cda47d92f1a153a14f01e781d980b3498390d3b4005e7c41` |
| Matching ELF | `6319d1647b73ffa7ac9c0f165c1db1a8dbdf3a5792dc066c97024203f6672d44` |
| Physical mGBA observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| P1 prefix receipt | `5c15c40493d0748486ecb77e062c6016552ef26dbb90f53291e2327bfcc8958f` |
| P2 prefix receipt | `b52bf9d71e9940ccead840bc855a47cf1d060c66722d0374650212a5e8def80b` |

## Fresh controller routes

The current staged gameplay flags win in ASan/UBSan HOST with route
purity checks and ordinary briefing and in-game dialogue input. P1
wins at frame 25,621 with the rescued hero alive; P2 wins at frame
33,803 with its rescued hero alive. The fresh P1 and P2 physical prefixes
match all four 1,000-call checkpoints on the exact staged P10 ROM.

P1's first full run matched all 26 periodic observations. It reached the
victory screen at call 26,081, exactly at the HOST victory frame, with
objective flags 6,919, the rescued hero alive and zero enemy buildings.
The strict verifier stopped because the tape has one remaining neutral
call. A focused fix validates this final dialogue release and compares
all terminal fields both before and after the remaining input. The
corrected replay finishes all 26,082 calls and matches all 27
observations, six briefing pages and nine authored pages. The hero
survives, the mission is marked complete, and all enemy buildings are
destroyed. It averages 23.82 emulated updates/s, with an 11.82M-cycle
peak. This is a win on staged P10 `0505b233`, separate from the older
KODAK P10 in the matrix and from physical GBA timing.

P2 also wins from HOME on the staged ROM. All 36 observations, fourteen
briefing pages and six authored pages match. The rescued hero is alive
at frame 33,803; the mission is complete with objective flags 908. The
verifier authenticates the final dialogue release at call 35,859 and
checks that all five remaining neutral calls preserve the full named
victory state. The 35,864-call replay averages 16.32 emulated updates/s,
with a 13.74M-cycle peak.

| Local artifact | SHA-256 |
| --- | --- |
| Fresh P1 HOST tape | `70259667d448fabb7efc898e7674c6f051ac87f9e3f65ab414069bde818d94be` |
| Fresh P2 HOST tape | `a1ab751c4e7cab20c1859405453a29196ab8e8c749da6296347b390f8652ab0b` |
| Fresh P1 4,000-call prefix | `da0597750da39d01260fa1811fd75f3a5afee4934bb157319d08a6deb8252786` |
| Fresh P2 4,000-call prefix | `c17db52a5fb28dfc9a899d58f8068146b0660d9dd21d29344d341818be61e278` |
| First P1 terminal observation | `e0e90327e31e217772ca36835ffe839ce110c00492ffb6f583dac54e6abb37da` |
| Complete staged P1 replay | `61d215d6df53fc2ff74fe67ba680f1b2ceb91bc836be175170bbee3edae94315` |
| Complete staged P2 replay | `d224289d3d9c19e5ed44872f5a3e84a1eaa9fb9685ef67188b8afe40308289f0` |
