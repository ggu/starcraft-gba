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
