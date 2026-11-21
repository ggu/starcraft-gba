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
The later training change below leaves launched and detached children
fenced.

The P10 sanitizer fixture pays for two pending orders, one on an empty
Carrier and one on a Carrier with two docked children. It checks that
bad cargo and duplicate hangar ranks leave the roster, paid queues and
wallet intact. The successful transfer refunds exactly 50 minerals to
the retired CPU, then preserves the neutral ownership, child links and
wallet through Pause save and fresh HOME load. The active training
case in that run verified that a blocked transfer also left a pending
paid order intact. The P10 receipt SHA-256 is
`ae8adc4b404de847d7273fc63e60334f3c72d78a111f31c5afbde26b70e199c2`.

## Active Interceptor training

The native training service creates the next paid child at
`0x80127c2c`. It stores the child in the parent at `+0xe4` and
stores the parent in the child at `+0xbc`. Creation inserts the
child at the head of its current owner's actor chain. For the ordinary
single-player elimination path, the defeat finalizer visits that newer
child before the older parent. It relinks the child to owner 11 even
while training is incomplete. The incomplete-state guard comes after
relinking and skips the primary-order reset.

When the finalizer reaches the parent, its queue cancellation uses the
still-linked child's current owner for the active front order. It
credits the full raw-73 price, 25 minerals and zero gas, then sends
the unfinished child to the death-order cleanup. Pending orders take
their refund from the same price table but credit the parent's former
owner before its own relink. The active refund therefore credits the
neutral wallet for the ordinary child-first chain. These findings
come from matching the owned USA ROM and the step-9,053 RDRAM.
A live native Carrier training cancellation during defeat remains open.

| Native ROM window | Bytes | SHA-256 |
| --- | ---: | --- |
| Training child and parent links at `0xeb860` | 368 | `659cfc0fb168dbbffe9ea8093bb595fac49146d3987ff21c0d9f89e18806dba0` |
| Owner chain insertion at `0xf81d0` | 128 | `e8eaa79a79879e30014d553235791788766e03ea65ea2063158b80e11c6e434e` |
| Defeat chain traversal at `0x8d6b0` | 160 | `0da63b54db1c9d85798afb1bcdebbe0b4de9a006e754d41d1fc8e6bf124531fd` |
| Active child refund at `0xe8f0c` | 56 | `fc9b16262ca6b8aafb47778d17fc7c39d593a5f9ef59d7cfc4f0f9ce7eefc804` |
| Child removal dispatch at `0xe9004` | 52 | `28f56b09dd8a0623a0057cbc9ffcbdde4b1c7b33f984ed986daa152382b25dfc` |

The opt-in preview validates the full departing roster before clearing
the valid hidden trainee and paying the split refund. The existing
wallet arrays carry both balances through the typed and compact journals.
The CPU fixture exercises one trainee, two paid pending orders and
two ranked docked Interceptors. Bad cargo, duplicate ranks and a bad
training timer all leave the roster and wallets intact. After a valid
retirement, the former owner receives 50 minerals and neutral receives
25. The docked actors keep their slots and ranks, and an ordinary Pause
save and fresh HOME load restore both balances and no trainee. The
Protoss human defeat fixture also verifies one active and one pending
refund and restores both wallets through the terminal journal. The
launched and detached fixtures still defer CPU retirement.

The current opt-in P10 and Z10 HOST sanitizer suites pass on
`9153b24b` with the prepared profile vectors.

| HOST receipt | SHA-256 |
| --- | --- |
| P10 complete elimination fixture | `a65c80e5479645130535c2f08dfd43583698c38c8db695d21b044fbda7207842` |
| Z10 complete Carrier-enabled elimination fixture | `74c79e1e360c17f001bc70b187681ad6759ed673afbedcbb9aa9df7d7202509d` |

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

With active-training cancellation code included, runtime `c9fdd5f1`
retains the same `SELZ` RAM endpoints and passes the ROM and payload
checks using the 41 pinned ARM inputs. The ordinary mixed Blood Bath
replay again saves a versioned active Interceptor pulse at frame 17,305.
The fresh HOME L load matches 180 uninterrupted updates and 12 world
cuts. The two paths leave at least 6,508 measured system-stack bytes
above the guard. This replay exercises the version-0 predecessor
journal and an active projectile. The paid trainee and wallet proof
comes from the sanitizer fixtures above.

| Active-training preview receipt | SHA-256 |
| --- | --- |
| ROM | `93ea8d2638c081cba76d4a49c59b8dbf32aea4745af1184354a186a0d8e44171` |
| ARM vector, input pins and RAM guards | `76b0b99d5292353e871fe880341dfa0a9b10d1d1988fe27a575ab1a2ccf9cada` |
| Ordinary compact save and continuation | `f48f075475587ddcc0e102b22e8944bc7cd6969d2aeaccec8dd459569622c7b5` |

Keep the build option off for normal products. Still needed are an
eight-slot Gantrithor retirement fixture, an ordinary active ARM
retirement with neutral transport cargo, and native CPU retirement
with a living survivor. The two retained clean mixed N64 journals
show only the human building census dropping to zero. The solo
Terran versus Protoss census ends with the Terran structures gone
and 14 live Protoss structures, after the Terran mobile count had
already reached zero.
