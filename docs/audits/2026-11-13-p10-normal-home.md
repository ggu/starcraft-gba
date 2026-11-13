# P10 from HOME on the normal Protoss cartridge

The earlier [P10 input replay](2026-11-03-p10-ordinary-input/README.md) used a
replacement ARM main loop. A later normal ROM replay loaded a genuine save
midway through the mission. This run starts a fresh normal Protoss P10 ROM
from HOME, selects Eye of the Storm in the campaign menu, reads the complete
briefing, and plays the mission using ordinary buttons.

The host controller uses the normal cartridge's flags and enables the same
modal behavior on host. It advances each briefing page with A. For mission
dialogue it presses and releases an acknowledgement while preserving the
current gameplay keys and simulation frame. Both transitions are recorded
in the tape. The route reads actor and walkability state to choose orders;
ASan/UBSan ran throughout, and a hash before and after each update checked
that controller reads did not change the engine's route state. The older
replacement-main controller and its save/resume tape keep their old mode.

The controller takes the Gantrithor around the edge, keeps Zeratul out of
reach, develops Raynor's Terran base, rescues the Hyperion and attacks the
Overmind. Both the host compiled from the ROM's source revision and a second
host compiled from `d406ff8` recorded the same winning tape: frame 46,057
after 47,194 update calls and 6,134 held-key transitions. The tape includes
an ordinary pause-menu save and continued play in the same process.

The emulator run uses the unmodified normal ARM ROM and its ELF from
`6e843d814da131950c92da79901985fc2e05ff78`. It sets physical button masks
and reads named state; it does not write guest memory or start with SRAM.
The briefing has 10 pages. Mission dialogue has 14 acknowledged pages
across nine source rows, including four rows during the victory sequence.
The replay checks that each press arms a page and each release advances it
without advancing the battle.

The normal cartridge won at the same frame. All 20 observations at 19
distinct calls matched the host across 14 fields. The Overmind was gone;
Zeratul, the Gantrithor and the Hyperion were human-owned and at full health
(60, 800 and 850 HP). The human trigger runner reported victory, the primary
objective and campaign completion bits were set, and the unsupported action
count stayed zero at the end. This qualifies one route from HOME, including
a pause save and continued play. It does not requalify fresh SRAM reload,
original N64 timing or AI, or physical GBA behavior.

The replay averaged 1,923,127 guest cycles per main-loop update, or 8.72
emulated updates per second including rendering, UI and presentation waits.
This is below the near-30-fps target.

With prepared private inputs, build the host controller with the normal P10
flags, then point the observer at a matching normal ROM and ELF:

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0' build/test-sc64-p10-normal-home
mkdir -p .local/sc64
./build/test-sc64-p10-normal-home .local/sc64/p10-home.jsonl 65000
python3 tools/sc64_p10_normal_replay.py \
  --trace .local/sc64/p10-home.jsonl \
  --rom build/StarCraft-Protoss-P10-Transport1-GBA.gba \
  --elf build/starcraft-protoss-p10.elf \
  --runner /path/to/sc64-headless-replay \
  --out .local/sc64/p10-home-arm
```

- ROM SHA-256: `4efc352f15e90dbc5d27b77721c252fb2bd2a44c877e03c38b49c63269ed9373`.
- ELF SHA-256: `e092d78aed44710fdb6704ca2a91f46a7621397117826a9f77ec79b4b380c315`.
- Winning HOST tape SHA-256: `9df379a2f121a171ca76a2d1f5414923bd6006ec3966901c251653c3ade92cb8`.
- Full normal ARM receipt SHA-256: `00f636e75f48a0abdd3139226c5bc034456202cae8b276521be3847d64f81b19`.
