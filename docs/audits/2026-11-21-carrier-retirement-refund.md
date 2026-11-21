# Queued Interceptors on skirmish retirement

The USA N64 owner-transfer routine clears a Carrier-class production queue
before changing its owner. At `0x801376bc` it calls `0x80124fe4`, which
visits all five entries through the queue cancellation handler
`0x80124e50`. An unstarted raw-73 entry takes the refund branch at
`0x80124f20`. The resource helper at `0x80006948` adds that unit type's
price to the current owner's wallet. The live raw-73 table has a clear
low property bit and a price of 25 minerals and zero gas. A training
actor at the front uses a separate handler.

The ROM windows match normalized RAM from native Blood Bath step 9,053.
The owned USA ROM SHA-256 is
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
The state and RAM digests are
`fc037935a847bf9ba1c4e557658603b040104f34c0aa30fe04bfcf2a52e02e38` and
`93fe4eaa1d0d975507f71e713ea0aac21558f4e31b02fab86def90f6ce59ddcb`.
This capture authenticates the code and tables. A live native
Carrier queue at the moment of owner transfer remains an open witness.

| Native ROM window | Bytes | SHA-256 |
| --- | ---: | --- |
| Owner transfer at `0xfb344` | 124 | `4b94e65f39d98ce2e7c8686e3ae6f42d147141d7f1fa1a2102c3d212256a17f9` |
| Queue cancellation at `0xe8cd4` | 132 | `8085893a1455f66aeee42e321dc7d87cce9c878b32e696b322749f303d8335d7` |
| Refund branch at `0xe8be8` | 48 | `09f50e6a99497ca4190752d805960d00376c249cd4a8531a7ef9e450f3b42875` |
| Resource credit at `0x7548` | 96 | `2a29e364a965486ddaaa751c5e29771d94717c5198c97ae1ef2fa04de82807c8` |

The opt-in [elimination preview](2026-11-18-blood-bath-elimination-arm.md)
now accepts a valid Carrier-class queue with only unstarted orders and
fully docked children. It validates the whole departing roster and
hangar graph before refunding the original owner and clearing the
paid-order count. Docked actors keep their slots, parent and ranks.
Training, launched and detached children still defer the transfer.

The P10 sanitizer fixture pays for two pending orders, one on an empty
Carrier and one on a Carrier with two docked children. It checks that
bad cargo and duplicate hangar ranks leave the roster, paid queues and
wallet intact. The successful transfer refunds exactly 50 minerals to
the retired CPU, then preserves the neutral ownership, child links and
wallet through Pause save and fresh HOME load. The active training
fixture verifies that a blocked transfer also leaves a pending paid
order intact. The P10 receipt SHA-256 is
`ae8adc4b404de847d7273fc63e60334f3c72d78a111f31c5afbde26b70e199c2`.

## Compact Z10 save and load

The original Z10 Carrier fixture passed its shared save scratch
directly from the compact writer to detached preflight. The compact
writer had already replaced the scratch with encoded journal bytes.
The fixture now selects the written journal through the production
candidate loader before checking the detached state. The old fixture
also fails on the unchanged preview `96af35cd`.

That correction exposed a separate format rejection when a retired
Zerg owner's researched Overlord remained neutral with cargo. The
compact envelope checked the legacy transport format before the
preview decoder could read its existing neutral Ventral Sacs bit.
Both preview readers now use the same syntax check. It accepts bit
`0x80` only in transport byte 3 and preserves that byte's human
completion bit. The ordinary decoder still rejects the preview bit.
The full candidate validator still checks the resolved match,
retirement mask and completed research.

The sanitizer fixture verifies `0x80` and `0x81` in byte 3, rejects
the same byte with reserved bit `0x40` and rejects `0x80` in another
owner's byte. An actual prepared candidate fails detached preflight
after clearing either its retirement mask or the retired owner's
research completion. A fresh load of the original journal preserves
the neutral Overlord and its carried unit. The fixture also saves and
reloads after the neutral Overlord dies, and exercises neutral cargo
on human defeat.

| HOST receipt | SHA-256 |
| --- | --- |
| Z10 with unchanged preview, detached scratch failure | `5d88435f9ac8625448c9d6094440b2e49ff5418a143232ed4c5a1777c0fdfa42` |
| Z10 with fixture correction only, compact trailer rejection | `e16f703cac0aa83978ea84c96b512cc637abcf8d3f03cbf1fbf62556a68585ab` |
| Z10 complete ASan/UBSan fixture on composed runtime | `ac628aa0fa10770b720b97e4a1b92101b2edc42f6bd1346393915aab85231ebc` |
| P10 complete ASan/UBSan fixture on composed runtime | `cd574adec461c29df07c3590613c87ebaa1813008646152ab12ce9934b7d3198` |

## Current opt-in ARM check

Runtime `6b40f3a6` composes the preview with `b33a7e9d`,
including the shared per-type hangar capacities from #667. The single
Z10 ARM link uses the normal Carrier-enabled compiler vector, the two
private elimination-preview flags, 41 unchanged input hashes and
the separate `SELZ` cartridge ID. IWRAM ends at `0x03004e08` and
EWRAM BSS ends at `0x02039b48`, within both link guards.

The real controller replay loads a prior compact Blood Bath journal
from HOME with L at frame 17,301. It runs until a versioned active
Interceptor pulse at 17,305, saves with Start and B, and reads the new
32 KiB journal on a fresh emulator from HOME. The new bank keeps its
authenticated versioned packet and pulse layer. The fresh and
uninterrupted matches agree through 180 arena updates and 12 world
checkpoints. The preview marker stays clear throughout this replay.
The positive neutral Ventral Sacs save and malformed-candidate cases
are covered by the HOST fixtures.

| Private Z10 ARM receipt | SHA-256 |
| --- | --- |
| ROM | `a693b47c14e07aa5757083f9bc9a6137edab2db923b1e19514c3afef67bc62f3` |
| Compiler vector, input hashes and RAM guards | `de052bf6cb4a72199c5a29444708aacfc20f500578baa6bd2dbb94b3e959ba10` |
| Active compact journal and fresh continuation | `21d9dc6801b0c5b8307e03ad4c63d31c6b089819fcbeb897b55a7e641ca94047` |

An independent blank-cartridge Blood Bath match selects Terran against
named Zerg, Protoss and Terran CPUs. The resolved bases are [1,2,0,3].
Frame 17,359 has one human building and four living human SCVs in
slots 0–3. Advancing to 17,360 destroys the last building, publishes
defeat, and transfers all four slots to neutral owner 11. Health remains
60, 42, 60 and 60. Three workers move two pixels during the final
update. A normal pause and 32 KiB save taken at frame 17,359 loads
through fresh HOME and produces the identical defeat state and neutral
rows. The three CPU owners still hold 16, 10 and 18 buildings. The
CPU-elimination and neutral Ventral Sacs marker bytes remain clear.
This witnesses ordinary human transfer on the composed Z10 preview.
The paired receipt SHA-256 is
`ebf4f9737b0d1b7d76f46cc0b1814a64e15c45ac0ae8131ec8e5431990b766b3`.

Keep the build option off for normal products. Still needed are an
eight-slot Gantrithor retirement fixture, an ordinary active ARM
retirement with neutral transport cargo, and native CPU retirement
with a living survivor. The two retained clean mixed N64 journals
show only the human building census dropping to zero. The solo
Terran versus Protoss census ends with the Terran structures gone
and 14 live Protoss structures, after the Terran mobile count had
already reached zero.
