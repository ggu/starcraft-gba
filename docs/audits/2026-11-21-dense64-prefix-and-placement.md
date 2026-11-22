# Dense routing trials on the attached-depth release

All Blood Bath runs start from the same three-CPU pause journal via
ordinary HOME. The 3,600-update paired runs match all 31 sampled
world and indexed-video cuts, the 16 fresh continuation checks,
route build and hit counts, and the next pause journal byte for
byte. The control is the attached-depth Zerg ROM `e39909df`.
The relocation probe changes where existing instructions run.

| Blood Bath trial | Mean guest cycles | p95 | Peak |
| --- | ---: | ---: | ---: |
| Current release | 921,678 | 1,215,527 | 3,805,463 |
| Refactored control, prefix off | 921,826 | 1,217,340 | 3,816,777 |
| Prefix, ROM loop | 932,491 | 1,277,927 | 3,909,281 |
| Prefix, IWRAM loop | 928,377 | 1,252,251 | 3,695,705 |
| Prefix, partial donor tagging | 920,227 | 1,217,552 | 3,694,638 |
| Prefix confined to donor refills | 922,817 | 1,222,838 | 3,694,921 |
| Facility loop in IWRAM, ARM instructions | 917,757 | 1,203,785 | 3,435,734 |
| Facility loop in IWRAM, Thumb instructions | 919,780 | 1,216,153 | 3,620,798 |

## Resumable point flood

The 64×64 trial parks at most four FIFOs of 128 padded-cell indexes.
It checks the origin between batches of 32 pops, up to 2,048.
The packed field retains the directions already discovered. The
first version materializes a complete field for donor publication
and whole-field consumers. Under the measured replay it parks 166
FIFOs and resumes 165 to completion. The pack and restoration work
offset the avoided initial pops. The targeted HOST differential
compares complete and partial directions with ordered BFS over 100
grids and tests eviction, topology and all 4,096 packed groups.

A second version retains known routes in a tagged partial donor.
This version parks 175 FIFOs and directly resumes 41. An unknown
origin in a donor with no FIFO may take a new flood, so 41 is a
count of direct resumes. The trial lowers the peak modestly, with
near-control mean and p95. The release keeps full point fields.
A production partial-donor policy still needs range and save
scratch ownership qualification and better evidence of sustained gain.

## Facility loop placement

The ARM placement lowers the Blood Bath peak by 9.7% on Zerg.
Showcase fails the existing 11 KiB IWRAM stack reserve in that
configuration, including the attempted size reductions. Keep that
stack reserve. The Thumb placement moves the facility loop
into IWRAM and moves the dense point setup and packing to ROM.
The point expansion loop stays in IWRAM. All four links pass
with the same EWRAM endpoints. Showcase has 32 bytes remaining
before the IWRAM guard. The other releases retain more space.

| Thumb placement profile | IWRAM endpoint | EWRAM endpoint |
| --- | --- | --- |
| Terran | `0x03004a10` | `0x0203aa2c` |
| Zerg | `0x03004d88` | `0x02039b50` |
| Protoss | `0x03004e48` | `0x0203b64c` |
| Showcase | `0x03004fe0` | `0x02039188` |

On Outpost, 95 ordinary-input world and indexed-video cuts and all 800
sampled route-counter pairs match. One displayed-page phase differs.
Over the sampled window, the peak falls from 2,668,046 to 2,260,282
guest cycles, the mean falls from 1,240,660 to 1,236,083, and p95
rises from 1,644,923 to 1,657,048. Full-route mean falls from 827,580
to 827,228. The whole route still takes long updates outside that
window. The Thumb trial gives a peak reduction on both maps with
little change to sustained work. Leave production placement as-is
until further steady-load and physical timing checks.

| Local evidence | SHA-256 |
| --- | --- |
| ROM-loop prefix comparison | `971d3b8563ed7d0d332af6f100e0c9993b8c67d6c01c72404effa013d2253cc6` |
| IWRAM-loop prefix comparison | `fa0937e6b46afa997c08631afd8aa6d4490de4ea930b1a7b03fa7b924517b251` |
| Partial donor comparison | `0a2bd521fe92b7817e6116cc8cf08f4dc21a6dd2f07aab063ebce79c07362855` |
| Refill-only prefix comparison | `d728a8eea251cd4ae7dbeb8adc0f81ea768a3609002d9be2a2df8497ab4cd8a9` |
| ARM facility placement on Zerg | `5296bed3c36848dc36c2ff958e8b0462d534be5b9bf13fa02340afa6fed78e57` |
| Thumb facility placement on Blood Bath | `123f2fef48de692a234fc284d8257c077bfbe30df677bc071ed3eec97104fa64` |
| Thumb facility placement on Outpost | `29dae780b0306a368ddfa7b19f4324fe52ca44138ab1e170654312bc7032872d` |
