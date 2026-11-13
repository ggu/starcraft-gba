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
