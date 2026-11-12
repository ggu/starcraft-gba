# Ghost and Nuclear Strike groundwork

The GBA custom roster already lists the Ghost and its Barracks/Academy/
Covert Ops requirements. Terran has the normal Ghost actor; the
Zerg/Protoss campaign layouts can map raw 1 to a scenario-only Ghost
that cannot train. The Silo remains deliberately unavailable while its
ammunition, attachment, Ghost orders and persistence are incomplete.

The owned USA N64 ROM's The Big Push map starts with two Terran Ghosts,
two Terran Command Centers and two neutral Nuclear Silos. Missile raw 14
is permitted by the mission. An isolated ares run loaded the local
campaign-unlock state, selected The Big Push and entered the ordinary
field. A second bounded boot captured the field at 250 minerals, 100 gas
and 20/20 supply, then saved a private emulator checkpoint. No strike
was captured. The first neutral Silo is at source
(2784, 3296); the ROM's placement rectangles and add-on offset put the
matching landed Command Center center at (2688, 3280). That is the next
ordinary-button test. The earlier native T10 audio capture independently
identified the warning clip, not the full timing.

## Owned-ROM assets and blast facts

| Source component | N64 lookup | Observed data |
| --- | --- | --- |
| Nuclear Missile | Raw 14, sprite 266, image 316 | Native ascent and descent headings |
| Targeting dot | Sprite 231, image 233 | Two frames; one source script tick each |
| Missile burst | Image 428 | Ten frames; modifier 9 |
| Impact | Sprite 267, image 318 | 26 frames; modifier 9; source offset Y −42; two ticks per frame |

The private importer pins the ROM, extractor, BOLT assets and IScript
mappings. It packs 40 frames into 100,828 bytes of row data plus a
16,128-byte projected destination palette lookup. An independent decode
matches every projected source crop; the C renderer decodes all 40
frames, applies destination-dependent light and keeps the HUD clipped.
The warning and other source clips are available in the N64 bank; the
new gameplay audio events are not wired yet. No proprietary files are
committed.

The owned N64 weapon data confirms Nuclear Strike raw 31, air/ground
eligibility, Explosive and nuclear hit type, and 128/192/256 source-pixel
radii. Its nominal damage entry is 600. [Blizzard's classic rules](https://classic.battle.net/scc/terran/tspecial.shtml)
and the pinned [OpenBW reference](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L12853-L12859)
specify an actual impact based on maximum HP and shields with a 500
floor. The host blast uses that calculation, stock footprint distance,
full/half/quarter explosive packets and the existing armor and shields
resolver. The reference only applies the inner ring to burrowed actors;
confirm edge behavior in the native replay before claiming parity.

Host checks cover friendly and air splash, shields and armor, immune
actors and ring boundaries. The independent state machine checks one
paid missile per Silo, 200 minerals/200 gas, 8 displayed supply,
warning, abort before release, and independent descent. Cadence is
passed in deliberately; the test clock is not a native timing claim.

```sh
python3 -m unittest discover -s tests -p test_sc64_nuke_assets.py
python3 tools/sc64_nuke_assets.py
make test-sc64-nuke-blast test-sc64-nuke-render
```

## Before activation

1. Replay T8 in ares: land and attach the Command Center, arm the Silo,
   target with a Ghost, cancel one strike before release, and capture
   the first committed descent and impact. Measure alert, dot, missile
   and blast cadence against a native update clock.
2. Wire shared Ghost production, the parent/add-on lifecycle, missile
   supply and ownership, Ghost commands, flight, source art and audio.
   Cover Silo loss, Ghost death or incapacitation, transport, cloak,
   Lockdown and Ocular range; avoid selecting unsupported commands.
3. Authenticate armed/queued/active state and unchanged predecessor
   saves. Verify ordinary inputs and AI admission for campaign,
   skirmish and Showcase, all ARM memory guards and busy battles.
