# Protoss 5 and 6 from HOME on staged P10

The controller opens the normal campaign menu, reveals every briefing page,
and acknowledges gameplay text using A or Start. Test-only steering queries
restore the live route bank, and a fingerprint checks that controller
decisions leave it unchanged between game updates. The old replacement-loop
targets and normal ROM build flags are unchanged.

The ASan/UBSan controllers both won under the staged P10 flags. P5, Choosing
Sides, rescues Tassadar and unloads him and two Zealots at the extraction
point. P6, Into the Darkness, rescues Zeratul and reaches extraction with
Tassadar still alive. Their input traces start at HOME with an empty save.

| Mission | HOST victory frame | Input calls | Briefing pages | Authored pages |
| --- | ---: | ---: | ---: | ---: |
| P5 | 5,057 | 5,172 | 12 | 5 |
| P6 | 99,906 | 101,370 | 6 | 17 |

The read-only physical-button verifier reproduced the P5 win on staged
P10. All seven checkpoints matched, including Tassadar's position and
ownership, the mission trigger state and the victory frame. All 12 briefing
pages and five gameplay pages matched. The completion bit was set with
zero trigger faults. That run averages 18.30 emulated updates/s; it does
not measure a physical GBA. The exact November 13 KODAK ROM is distinct
from the staged ROM used here.

The same staged cartridge also wins P6. All 113 checkpoints match,
including Zeratul and Tassadar alive with 60 and 65 HP at extraction.
All six briefing and 17 gameplay pages match. The cartridge sets the P6
completion bit with zero trigger faults. The run averages 12.55 emulated
updates/s, well below 30. Some dialogue pages share an input boundary:
the same read proves one page released and the next is ready to
acknowledge. Earlier verifier attempts reached the same results but
missed these shared-boundary observations; the final tool checks both.

| Retained artifact | SHA-256 |
| --- | --- |
| Staged P10 ROM | `0505b233994dca90cda47d92f1a153a14f01e781d980b3498390d3b4005e7c41` |
| Corresponding ELF | `6319d1647b73ffa7ac9c0f165c1db1a8dbdf3a5792dc066c97024203f6672d44` |
| Physical mGBA observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| P5 HOST input | `b0806cae59b0e595e5f9de874d8f6c5a556587fd1a12ae532f6d228e01e2f10d` |
| P5 authored pages | `60767da76b9f10d75451bc104093f5454c785d25c9c77bfe111dc09c3cd4c318` |
| P5 physical receipt | `c5a7548ed810cc914bc7678836ce8b000c4d41fbbca1f67d3b26b79d88757c66` |
| P6 HOST input | `f405c609b278c07827ceb227e0fb7947f9f90aba6b8f40abd5dc5d0c47582bd4` |
| P6 authored pages | `c892ed584c8bb8b333d8a9f2d7ae7f242beed4c59af7bfae24734ac1808882ed` |
| P6 physical receipt | `5b05e3cb38dc14fd0f696175fe52820ad2854603dfeb878744b054a4b8005f3c` |

With the prepared P10 inputs, build `build/px-p5-normal-home` and
`build/px-p6-normal-home`. Run them from a new directory with a separate
`.local/sc64` for attempt results; pass `--limit 180000 --attempt-tag
home-p5` to P5 and `--trace /path/to/fresh-p6.json` to P6. Capture stderr
for authored-page metadata. Supply each tape, its page log and the matching
ROM, ELF and mGBA observer to `tools/sc64_p56_normal_home_replay.py` with
their SHA-256 pins. The verifier leaves the artifacts unchanged.
