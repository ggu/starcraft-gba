# P3 and P4 HOME controller preparation

The earlier carried P10 tapes won on replacement-loop ARM at 49,747
calls for P3 and 137,973 for P4. They are not winning tapes for the
current normal cartridge. Both host controllers queried the engine's
live route machinery while choosing inputs. P3 used the public path
query in five places; P4 used the lower-level route query in two.
Those queries can change cache residency, which movement later reads.

The opt-in `SC64_P34_NORMAL_HOME` HOST controller mode wraps the
queries with the shared route snapshot and restore helper and checks
the route fingerprint between game updates. It reads all briefing
pages and records ordinary button acknowledgments for authored
dialogue using the existing HOME controller helper. Historical
controller targets keep their previous settings.

To qualify the new routes, compile the P3 and P4 controller sources
using the staged normal P10 flags and HOST sanitizers, adding
`SC64_P34_NORMAL_HOME` and `SC64_TEXT_BOX_HOST_MODAL`. Use an owned
trace path for P3 and a unique attempt tag in an isolated output
directory for P4. Run P3 first because it is shorter. After a HOST win,
replay from HOME on the matching normal ROM and compare briefing,
modal arm/release, named actor and scheduler checkpoints, and the
terminal result. Stop on an unexpected RESULT before another key can
restart the mission.

No HOST compile or cartridge replay was started for this preparation;
the shared disk reserve was unavailable. This is source preparation,
not a claim of a new win.

## P3 staged HOST route limit

On November 14, the ordinary HOME P3 controller compiled under the
staged P10 gameplay vector with ASan/UBSan and HOST modal handling.
It read all nine briefing pages and passed the route-state fingerprint
guard through 180,763 inputs (180,000 simulation frames). It cleared
Force 3 buildings, then stopped at the configured limit with ten
Force 2 buildings remaining, the same building count as at frame
60,000. There was no victory, defeat, or sanitizer error.

The final trace shows the selected army still under attack orders, with
many units clustered around (1684,1356) near the home structures and
repeated path retries. The economy had 44 minerals and 44 gas and two
idle Probes. This identifies a controller route and economy limit; it
does not establish that any one building caused the jam.

Two isolated controller experiments also hit their limits. A local
waypoint cycle engaged too early and left 38 enemy buildings. Keeping
new structures out of a 96-pixel radius around the observed cluster
cleared Force 3 buildings but left 12 Force 2 buildings. Neither
experiment is carried forward. The next controller test should track
sustained lack of movement before regrouping and verify that the army
actually leaves the base before sending it on the long sweep. Restore
worker income if a new sweep cannot replace losses.

No ARM replay was started because no current route-pure HOST tape won.

| Local receipt | SHA-256 |
| --- | --- |
| P3 HOST limit tape | `d9c5a26f8d3b195ed0b403e53507465f5fb68f9aafd903d77262cb1e2baa127e` |
| P3 HOST launch and blocker log | `a62cda2ca9d5303823aa3470c92ecb12766c5f0cab732d25938fdec9e61612b3` |
| Waypoint experiment tape | `13a2d762b3187371522950623b635e70cfaa24c7a5083cf0b564ec7447bd4b83` |
| Build clearance experiment tape | `06445148f3cbff53472f75e28f82e1d734961613cfe2dc7628352680e714dfaa` |

## P4 full staged HOME win

Under the staged P10 flags, releasing the final briefing input advances
the first world frame and creates Raynor as specified by the mission.
The controller now checks that observed 138-unit opening, instead of the
137-unit pre-update census used by the legacy harness.

The route-pure HOST controller won at frame 129,310. A fresh
physical-button replay on the staged P10 cartridge matched all
146 observations, seven briefing pages, and
13 authored gameplay pages, including the rescue and the terminal
completion. Both heroes are alive at extraction with no trigger faults.
The replay used 130,239 input calls and averaged
15.30 emulated updates/s. This is the same
pinned staged P10 ROM and observer as the P5/P6 evidence; the exact
November 13 KODAK ROM and actual hardware are separate.

| Local artifact | SHA-256 |
| --- | --- |
| P4 HOST tape | `a744df3cdc3e88866dbb96092596a8ac0230643dfeb410e807e52b42acc236c0` |
| P4 authored pages | `aa904b858811393c3056491f2ed87e09c0c53f6a6f106d106bf54f11542ba5b6` |
| P4 physical replay receipt | `6c07d80e62b4754929ebea17aa498010a12b4e6027f6b59b1e7e3de6838b5c85` |

The same input tape also wins on the exact November 13 KODAK P10
cartridge, with 146 matching observations and both heroes at the exit.
It averages 15.30 emulated updates/s. The receipt SHA-256 is
`b4411bce2469bfc60b4c6bcd9c68940263335391cff49ab94d62f71554a8d1d0`.
The cartridge SHA-256 is
`7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310`.

## P3 egress follow-up

A local regroup probe independently detected the stationary army at
(1684,1355), beginning at frame 64,355. In this group 12 of 14 fighters
were waiting on a path retry. The probe found no reachable open rally
cell in its 96-pixel search, issued no commands, and produced the same
180,000-frame result as the previous route. The tape SHA-256 is
`d9c5a26f8d3b195ed0b403e53507465f5fb68f9aafd903d77262cb1e2baa127e`. Picking a different far building alone is not a
demonstrated recovery. Inspect local occupancy and the map corridor
before the home construction cluster forms, or sustain a reachable
second economy and army through ordinary inputs. No ARM win is claimed.

## Wide P10 packaged ROM: P3 victory

The earlier route reached a lasting jam near its first base and could
no longer replace losses. A direct waypoint experiment and a building
exclusion around the late cluster did not produce wins. The successful
controller changes the economy earlier: starting at frame 12,000 it
periodically considers visible mineral lines beyond the first Nexus.
It checks legal placement and a reachable Probe, ranks sites by nearby
ore, then issues the ordinary Nexus build command. The accepted route
built the second Nexus near (1888,96). The production rules, actor
state, and route engine are unchanged.

On the staged wide P10 flags, the ASan/UBSan HOST controller won from
HOME in 46,735 inputs, frame 45,951. The exact packaged wide P10 ROM
reproduced all 47 checkpoints across 27 named fields, read all nine
briefing pages, and acknowledged three in-game dialogue pages. Both
enemy forces had no buildings left. The P3 victory sequence completed,
the campaign objective and completion bits were set, and no mission
faults occurred.

| Input or receipt | SHA-256 |
| --- | --- |
| Wide P10 ROM | `2ded5e5e314466bc02e71916fcf7ca56227cd84a4d5ee999fd2ed7799fae05e9` |
| Matching ELF | `534e64d95beec4577b4e25978c5810e34a371f1268c8a8b6e3670c0ae9975dd9` |
| HOST tape | `0a0540f40220daf10fa88c682caaccf0064128a2ceeb68d84dd854e5062cc95b` |
| HOST briefing and dialogue log | `82e8dc2beaf9e1dd3e3d93998860156a0c4e6f7f4c00e897b376972020666a23` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Full normal-ROM receipt | `dc22b22e3fd27c7aeb7036cdd5f71a743eddbf9756240f7fd4be776c129d5f44` |

A separate run followed the same physical tape to call 24,588, frame
24,000, before the expansion. START and B saved a genuine 32 KiB SRAM;
a fresh process loaded it from HOME with L. The saved and restored
named world, campaign and P3 runtime state matched. The uninterrupted
and fresh-load paths also matched at frame 24,241 after neutral input,
and SRAM readback was byte-identical. This covers one pre-expansion
save point and a bounded continuation, without a resumed win claim.

The save receipt SHA-256 is
`6a56a77d0a10b2eee6e281f8d103b755f3ec8285c3d1b70f5e831caf5c2b5d6c`;
the physical SRAM SHA-256 is
`a07d7611ec5f986332872270b163779ef1f220560577e6eaf92b5cefea691c1b`.

## November 20 P4 fresh SRAM victory

The sanitizer HOME controller regenerates the 130,239-call route on
`6720a2fb`. The normal P10 ROM starts at HOME in an empty mGBA process.
At call 66,029 the input is neutral and both heroes have been rescued.
START pauses at frame 65,700 and B writes sequence one of the normal
32 KiB SRAM journal. The reader checks the record checksum, inner and
outer versions, mission and frame. The second bank remains erased.

HOME L in a fresh process restores the same named gameplay state,
controls and complete P4 runtime state. All 146 controller checkpoints
match, including 71 after the load. All seven briefing pages and 13
field dialogue pages match. The saved bytes are stable through the
remaining inputs. Both heroes reach extraction alive and owned by the
player at victory on frame 129,310. The campaign completion bit is set
and the mission reports zero unsupported actions and trigger faults.

The complete run averages 15.21 controller calls per emulated second,
including HOME and dialogue input. Physical GBA timing remains open.

In a prepared tree, build `build/px-p4-normal-home` and run it with a
unique `--attempt-tag`. The controller writes the winning tape to
`.local/sc64/`. Capture standard error for its authored page log.
Use both files with the pinned normal P10 ROM, ELF and observer in
`tools/sc64_p56_normal_home_replay.py --mission 23 --fresh-save-call 66029`.

| Input or receipt | SHA-256 |
| --- | --- |
| Normal P10 ROM | `2735a1b4832ab4030e8f55a8a348fef7f51b3b7e8a29774f1109ac375931192c` |
| Matching ELF | `136952c78337e9b8bc562219ce47b60bcdaad4bc70322e79140a1f3e69a9bcb7` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Regenerated controller tape | `dfdcbebc764006875b71ff5645da9ea9e5abb09ff63fc5f017af3e686371d069` |
| Authored page log | `d4754eac6f547e4e41e4c9a8b3736c28d2a4def7f68bf4b2385f23cdaa3fcd` |
| Pause SRAM | `d9af44b01f34e652eea0ee5e4e13a659647a6bb816ac19559afa2d74106403bf` |
| Fresh-process victory receipt | `b3a856e85fb50d8dbe533411a5c2b5873529f2cbd9b91f140f324660e6cf2632` |
