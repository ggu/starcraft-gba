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
