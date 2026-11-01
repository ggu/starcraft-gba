# Shared race transport (activation candidate)

`SC64_SHARED_TRANSPORT` is enabled in the normal Terran, Z7, P7 and Showcase
candidate profiles and inherited by the Z8 and P8 candidates. It provides
ordinary Shuttle and researched Overlord transport while retaining Dropships
and mission-specific research paths. Explicit legacy Z6 retains its prior
feature-off profile.
Four owners have independent Ventral Sacs research jobs. The human command
menu can start or cancel research at a completed Lair or Hive; this stage does
not add an AI transport strategy or claim original-game AI parity.

The feature uses its own save tag and a 16-byte research trailer. It does not
import saves from older tags. Feature-on ROMs must have a distinct, versioned
artifact and save basename; reusing another version's SRAM is unsupported.
With impact events, the trailer reduces the maximum number of pending impacts
that fit in a save. A save exceeding the remaining space is refused rather than
truncating effects. The existing two-slot save journal remains in use.

These are candidate checks, not release or performance qualification.
Shared transport applies the same pinned cargo-capacity facts in custom games
and campaigns; mission actor availability and source ownership remain separate
checks. Campaign Overlord transport remains tied to its own research state.

## Qualification snapshot

At source `8e09a26a3cda146432060e60c39b8929667c82ee`, host
ASan/UBSan checks exercised T10, legacy Z6, promoted Z7, P7, the P8
candidate and Showcase with both maps, all human races and one to three
opponents, including mixed and all-Zerg setups. Structures, resources and
actor proximity were seeded. The checks include paid carrier production,
human Ventral Sacs research, boarding, unloading, ordinary save/load, carrier
death and slot reuse. Default-off custom Dropship checks cover T10, Z6 and Z7;
positive P5 and Z6 mission checks include the authored Z6 Overlord kill action.
They are not exhaustive controller playthroughs.

Feature-on ARM links passed the existing memory guards on the same runtime
source. The table records BSS and IWRAM end addresses; the IWRAM guard is
strictly below `03005000`.

| Profile | BSS end | IWRAM end |
| --- | --- | --- |
| T10 | `020370e0` | `03004b40` |
| legacy Z6 | `02037154` | `03004c48` |
| promoted Z7 | `0203d718` | `03004f80` |
| P7 | `02038314` | `03004a28` |
| P8 candidate | `02038cdc` | `03004a28` |
| Showcase | `02037008` | `03004cc0` |

The feature-gated P7 and P8 fact tables were regenerated from the pinned
inputs, with SHA-256 respectively
`a9a01d7d70085ab0cbb4ea58a44d5781f980a6dc1c07c77393050a586cb8c441` and
`f5920e90eeec1ba0372e174294ffe9e0f1d74303f5e7c14d6a008dea960e0b92`.
The clean, unprepared source check, doctor, default build and test passed:
926 Python tests with 26 skips, plus the separate five- and eight-test suites.
Pinned inputs were unchanged.

The initial Z6 mission fixture used an unavailable old probe bank and then
lacked a standard declaration and production art object; its recipe was
corrected and its positive test passed. An initial clean source check found
the main Makefile over its size limit, so transport recipes moved to an include.
A subsequent default test found the missing `SC64_SHARED_TRANSPORT` fact-table
gate; the generator and Z6 reference comparison were corrected before the
matching fact tables and affected checks were rerun.

This evidence does not establish AI transport or research tactics, native AI
parity, near-30-fps performance, or release admission. Shared transport combined
with the experimental Scarab save namespace has not been runtime or save
qualified. Campaign cargo permissions remain a separate qualification gap.

## Activation candidate

The activation candidate routes the normal Terran, Z7, P7 and Showcase builds
through the shared transport capability. The explicit Z8 and P8 candidates
inherit it. ROM filenames include `Transport1`, and `dev play` uses separate
versioned save directories for the normal race aliases. The explicit legacy
Z6 product keeps its earlier flags, filename and save directory. P7 and P8
use separately generated fact tables with the transport actor gate enabled.

These routes are candidates until their exact-source runtime, memory, save and
performance checks pass. A `Transport1` save is a separate namespace: the new
reader does not import a predecessor save. Reusing older SRAM manually is
unsupported; the new journal makes no claim to fence an older reader.

## Activation candidate checks

The `96108b2` runtime source passed host sanitizer checks for the normal T10,
Z7, P7 and Showcase profiles, P8 and Z8 candidates, and an explicit feature-on
Z6 comparison. The custom-game fixture uses the normal setup controls for both
maps, each human race and one to three AI slots. Its research, cargo and
ordinary pause-save/load checks use seeded resources, structures or proximity;
they do not establish an AI transport strategy or a match win for every setup.

P7 and P8 checks seed a completed Robotics Facility, Pylon and resources, then
pay for a Shuttle, issue the player boarding action, save/load, and unload.
A mixed loaded-Shuttle/Carrier selection checks that the visible unload row
and its action agree. Positive T5, P5 and Z6 transport checks passed. The full
ordinary Terran campaign target passed at runtime-identical `4090ee2`,
including the T6 controller after restoring its existing unload menu. Z7's
carried-controller and skirmish prerequisites passed at that source; a later
host-fixture-only change corrected the save-envelope expectation and its
promoted-profile check passed separately. The initial Z7 aggregate failure and
its targeted fix should not be read as a fresh full aggregate pass.

The P7 canonical controller reached frame 59768 without a win or loss. The
same source, inputs and controller with transport ON and OFF produced
byte-identical traces; both runs ended with ten human Probes, eight idle, two
on gas, zero minerals and no remaining human combat army. The cause of the
idle workers was not established. The full P7 aggregate also remains
incomplete because the separately packed private canonical P6 sprite bank was
not present in the verified input closure. No bank was substituted. The
literal `./dev sc64 test --profile terran` command stopped before compilation
on a missing private palette prerequisite; the same ordinary Make target
passed with verified input files explicitly marked existing.

All versioned candidate ROMs and explicit legacy Z6 linked and passed their
header checks on `96108b2`, using unchanged pinned inputs and the P7/P8
transport-gated fact tables listed above. BSS/IWRAM end addresses were T10
`020370e0`/`03004b40`, Z7 `0203d718`/`03004f80`, Z8
`0203dbd8`/`03004bf8`, P7 `02038314`/`03004a28`, P8
`02038cdc`/`03004a28`, Showcase `02037008`/`03004cc0`, and legacy Z6
`02037134`/`03004c10`. The strict IWRAM limit is below `03005000`.
The clean unprepared source, doctor, build and default suite passed: 927
Python tests with 26 skips, plus separate five- and eight-test suites.

The physical Z7 Transport1 run used an ordinary neutral cut, pause/save and
fresh load. Its bounded continuation through trace observation 324 passed; the
full fresh-resume replay reached WIN at trace tick 179753 / game frame 177804,
matching 302 post-cut named observations (303 including the retained cut).
This is named-state evidence, not whole-world equality or an uninterrupted
full run. It does not establish near-30-fps performance, full native AI parity, AI transport tactics, a Z8
win, or a P7 controller win. Experimental Scarab namespace composition remains
unqualified.

Compared with the retained earlier physical replay, those 302 named
observations matched. Mean reported cycles increased from 2,185,245.2746 to
2,186,610.8621 (+0.06249%); the peak increased from 50,583,602 to 53,931,783
(+6.61910%). The runs used different binaries, so this comparison does not
isolate the transport flag, establish whole-world equality, or measure GBA
hardware frame rate.
