# Campaign and skirmish checks on the steering release

The ROMs packaged for `c2624205` run the same runtime as
`4aff72cf`. These checks use the matching release ELF and ordinary
held-button input in headless mGBA. The earlier
[all-mission sweep](2026-11-21-late-campaign-home-sweep.md) used the
attached-depth cartridges from November 21.

## Campaign continuation

| Chapter | Pause frame | Victory frame | Matched checkpoints | Fresh victory readback |
| --- | ---: | ---: | ---: | --- |
| T4 | 4,968 | 8,911 | 10 | Identical re-export |
| T7 | 2,289 | 3,931 | 12 | Identical re-export |
| T10 | 2,276 | 3,361 | 6 | Identical re-export |
| P5 | 2,700 | 5,057 | 7 | Pending third process |

Each Terran replay saves through ordinary Pause and continues from
fresh HOME. For P5, the complete HOME replay checks twelve briefing
pages, five dialogue pages and mission completion. A separate fresh
HOME load picks up the Shuttle in flight with both passengers and
matches seven checks through victory. The earlier mission-wide
coverage and these current cartridge replays remain distinct receipts.

| Local campaign receipt | SHA-256 |
| --- | --- |
| T4 continuation and result | `a0fd13ac4198916f1dccf6fd0b041b766c9c05308fd8694a7ae8a917d6fdbd41` |
| T7 continuation and result | `826c960ef7e6a3585bb3cba8cb921d44af9ecbb4ba759016e0476ff84797e299` |
| T10 continuation and result | `2531aff81cfa073039a4f58207f2260bea4fc4a8d707063c2bfe0fdf86a6b619` |
| P5 HOME victory | `a203741cfac555b469601a81e430d504af92613ef3040966cf99fd570456669d` |
| P5 Shuttle pause continuation | `88c972f353511a679f79924382e34e7878f098d9ac8e84e5440aea10d89327c2` |

## Normal Blood Bath win

The retained Terran controller starts from blank SRAM, chooses
Blood Bath and one named Zerg CPU, and wins on the current Terran
ROM at frame 26,692. The replay matches 158 action checkpoints and
160 total recorded observations. It finishes with 76 Terran units
and 18 buildings. The same 27,281-call trace now averages 850,752
guest cycles per call, down from 979,650 in the earlier
[matched release measurement](2026-11-21-blood-bath-terran-win.md).
That is a 13.2% reduction across the intervening engine changes.
Calls per second rise from 17.13 to 19.72 in emulation. Physical
handheld frame pacing still needs measurement.

The HOST controller now accepts
`SC64_SKIRMISH_RENDER_INPUT_FRAMES=1` to render between physical
input updates while authoring a tape. On the exact current source,
the Terran case with this option produces identical tape bytes to
the retained winning route. Its tape SHA-256 is
`a5935d2918b44ceb98fd91e2e25de74b7197d6e297e6cb12980def38fa4341da`.
The physical receipt SHA-256 is
`47f8e39d60fbf1f937ef7d5e0c57048a5499e0e5bedadf90d1ab8212b693a3fa`.

## Normal two-CPU terminal cases

The rendered HOST controller also drives a Protoss Blood Bath game
and a Zerg Outpost game against two named Zerg opponents. Both tapes
reach defeat on the current normal ARM ROMs. Paid action wallets,
the actual chosen seats and terminal actor counts match the HOST
traces. Both controller policies left over 4,400 minerals unspent,
so these replays qualify terminal behavior and motivate a stronger
production policy for the active human victory trials.

| Human and map | Defeat frame | Matched observations | Local result SHA-256 |
| --- | ---: | ---: | --- |
| Protoss, Blood Bath | 31,663 | 21 | `cc0f0cae063e5f3327b8ebc98ebb5b20f330281465a46b93637cbd41df967439` |
| Zerg, Outpost | 23,688 | 25 | `047e119d389b57f73d9ab576f0899634ee7f5a94d6c85a7233af5e34a7928c37` |

## Blood Bath Melee preview

The existing `SC64_NATIVE_MELEE_STARTS` flag builds privately in all
four normal profiles against the current release inputs. Each binary
passes the ROM and RAM guards and enters Blood Bath Melee through
the ordinary lobby with named Protoss, Zerg and Terran CPU seats.
The human race follows the corresponding race ROM. The Showcase
case uses Protoss. Each run plays to frame 1,501, saves through Pause
and then continues with neutral input. The separate fresh HOME
process matches the saved world, resolved starts, mode and four
sampled world/result cuts. All four runs end in human defeat.

| Preview | Result frame | IWRAM end | Local result SHA-256 |
| --- | ---: | --- | --- |
| Terran | 11,580 | `0x03004b60` | `3e0a262955bcb43b3bb7fbd8b606a5cca4703d6dc69ad7e4e451fbc07360a4ed` |
| Zerg | 11,498 | `0x03004f98` | `9e4e75d9e7fa8c1d3e8434d043463d024e8ab76619172f55ba23486bd6eb2c35` |
| Protoss | 11,515 | `0x03004ed0` | `a910ae53da77d9ffb5ce76dac2eede0b51f0a48431bc502c3c474f1dcd5782f7` |
| Showcase | 11,644 | `0x03004f20` | `5e90b9ace23e8930ac06107d5b1f6bdb5e0cdda6d2a2af27cdfc329ba8b5046e` |

The results extend [earlier terminal testing](2026-11-12-skirmish-seat-options.md#current-terran-arm-terminal-and-save-checks)
to the current builds. The existing flag stays opt-in. An active human
win, native neutral survivor transfer, seeded Free for All compatibility
and a sustained busy performance comparison remain priorities.
