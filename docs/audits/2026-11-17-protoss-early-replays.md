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
winning staged tapes. This verifies the routes on the host; only the
normal-ROM replays above establish current ARM wins here.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Regular P10 ROM | `a4c5c93c307bc275aca13a61e4e2526b1fabccfdfa4465382fa72afd28be0dff` |
| Regular P10 ELF | `142c4c0ebde19e05972d7bf931e31291c36ef23ba29a5129d043873720a61a57` |
| P4 ARM receipt | `453910dd2232230f3f3d129d3a98ce6474ab03c5ce3f87ccc0513505800a8ea0` |
| P5 ARM receipt | `48dd677bdc4215c416c18bcc10dd0026a25d98017d7195194c5e8604f078b65b` |
| P6 ARM receipt | `b6be0f54215551d7068b20934bf5af33ccaf9db9c0cc826a2814b187a8cb9480` |
| P1 host input tape | `70259667d448fabb7efc898e7674c6f051ac87f9e3f65ab414069bde818d94be` |
| P2 host input tape | `a1ab751c4e7cab20c1859405453a29196ab8e8c749da6296347b390f8652ab0b` |
| P3 host input tape | `0a0540f40220daf10fa88c682caaccf0064128a2ceeb68d84dd854e5062cc95b` |
