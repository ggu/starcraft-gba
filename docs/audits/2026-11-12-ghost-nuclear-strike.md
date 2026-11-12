# Shared Ghost and Nuclear Strike probe

The opt-in probe trains the ordinary Ghost at a Barracks with an Academy
and attached Covert Ops, builds a Nuclear Silo at a Command Center, and
trains one missile per attached Silo. With the Silo selected, open **B
Commands** and choose **Arm Nuclear Missile**. The missile takes 200 minerals,
200 gas and 8 supply. Select a Ghost and choose **Nuclear Strike**, then
press **A** at the target. The Ghost approaches if needed and can abort
before descent. Once it starts descending, killing the Ghost or the Silo
does not cancel the missile. Blast damage affects friendly units and air
targets as well as enemies. The probe rejects writes to the normal save
format and has distinct ROM codes. Default ROMs still omit this feature.

The owned USA N64 ROM's The Big Push map starts with two Terran Ghosts,
two Terran Command Centers and two neutral Nuclear Silos. Missile raw 14
is permitted by the mission. An isolated ares run loaded the local
campaign-unlock state, selected The Big Push and entered the ordinary
field. A second bounded boot captured the field at 250 minerals, 100 gas
and 20/20 supply, then saved a private emulator checkpoint. The first neutral Silo is at source
(2784, 3296); the ROM's placement rectangles and add-on offset put the
matching landed Command Center center at (2688, 3280). In the native game,
the CC's C-Down **LAND** command highlights the compatible add-on socket.
After A confirms landing there, the Silo turns from neutral to player
ownership and the advisor says to use Ghosts to target missiles. The
owned Silo exposes C-Down **BLD NUKE**. At 250/100 and 20/20, it refuses
with **NOT ENOUGH SUPPLIES**, before checking gas. The probe uses the
same refusal order. In a separate checkpoint copy, only the initial gas
and available supply were raised to fund the order. Ordinary native
**BLD NUKE** charged 200 minerals, 200 gas and 8 supply; **CANCEL** returned
all three. The native Silo showed a missile icon and progress bar while
building. A fully timed strike and damage comparison still require a
native replay. The earlier T10 audio capture independently identified
the original warning clip.

## Owned-ROM assets and blast facts

| Source component | N64 lookup | Observed data |
| --- | --- | --- |
| Nuclear Missile | Raw 14, sprite 266, image 316 | Native ascent and descent headings |
| Ghost painting | Raw 1, sprite 229, image 228 | Three directional poses, then holds the last while tracking |
| Silo and Covert Ops | Raw 108/117, images 312/288 | Built and almost-built bodies, working and landing overlays |
| Targeting dot | Sprite 231, image 233 | Two frames; one source script tick each |
| Missile burst | Image 428 | Ten frames; modifier 9 |
| Impact | Sprite 267, image 318 | 26 frames; modifier 9; source offset Y −42; two ticks per frame |

The private importer pins the ROM, extractor, BOLT assets and IScript
mappings. It packs 40 frames into 100,828 bytes of row data plus a
16,128-byte projected destination palette lookup. An independent decode
matches every projected source crop; the C renderer decodes all 40
frames, applies destination-dependent light and keeps the HUD clipped.
The bank also imports the original Ghost poses (3,289 indexed pixel bytes)
and the two add-ons (6,925 bytes). The Silo working light animates during
production. Optional audio preparation adds the native warning, sound 127,
to the ordinary advisor mixer; existing banks remain compatible and silent
for this event. No proprietary files are committed.

The owned N64 weapon data confirms Nuclear Strike raw 31, air/ground
eligibility, Explosive and nuclear hit type, and 128/192/256 source-pixel
radii. Its nominal damage entry is 600. [Blizzard's classic rules](https://classic.battle.net/scc/terran/tspecial.shtml)
and the pinned [OpenBW reference](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L12853-L12859)
specify an actual impact based on maximum HP and shields with a 500
floor. The host blast uses that calculation, stock footprint distance,
full/half/quarter explosive packets and the existing armor and shields
resolver. The reference only applies the inner ring to burrowed actors;
confirm edge behavior in the native replay before claiming parity.

Host checks use the ordinary GBA B and A input path in Terran, Z10, P10
and Showcase custom games. They cover Ghost and Silo production, paid
cancellation, refusal above the exact 200 supply limit, reservation
transfer, abort and impact after the Ghost and Silo die. Terran also uses
the actual T8 world and source availability tables to test the Command
Center's landing and neutral-Silo capture. Other impact fixtures check
friendly and air splash, shields and armor, immune actors and ring
boundaries. The imported missile script waits 250 source ticks during
WarpIn. Ascent, reentry, warning and impact still use a bounded model
based on the [reference order sequence](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L5829-L5939)
and owned motion data; this is not a claim of measured native timing.

```sh
python3 -m unittest discover -s tests -p test_sc64_nuke_assets.py
python3 tools/sc64_nuke_assets.py
python3 tools/sc64_nuke_building_assets.py
python3 tools/sc64_nuke_ghost_assets.py
make test-sc64-nuke-blast test-sc64-nuke-render test-sc64-nuke-world
make sc64-nuke-probe
```

To include the original warning, prepare the optional audio bank and
pass it to the build:

```sh
python3 tools/sc64_audio_pack.py --nuke-warning --out .local/sc64/audio/nuke-generated
make sc64-nuke-probe SC64_AUDIO_DIR=.local/sc64/audio/nuke-generated
```

## Before activation

1. Measure the complete native T8 warning, dot, Ghost pose, reentry and
   impact. Verify what cancels tracking or prevents it (including
   incapacitation and transport), targeting a moving unit, and splash damage
   at the ring boundaries. Import the remaining Ghost/launch/impact sounds.
2. Implement stock Cloak, Lockdown, Ocular Implants and energy research.
   Match the full add-on and missile actor lifecycle, including lift in
   custom games and whether a visible missile can be attacked.
3. Authenticate armed, queued and active strikes and read predecessor
   saves. Verify the ordinary ARM controller sequence, bot production
   and launches, and busy fights before enabling the default ROMs.
   The asset-free `make test-sc64-nuke-save-rows` validates a proposed
   no-growth format independently. It is not connected to the journal yet.
