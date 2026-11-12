# Ghost Covert Ops research

This source-only stage models the four paid jobs at a Covert Ops. It
adds no game commands, spell effects, ROM assets or save fields.

The owned USA ROM is SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`.
The BOLT `00C/008` technology table SHA-256 is
`8ffb5f7eb6882a641f7812af1e47650c8f21f4651998be447375e8bc9608c66e`.
The `00C/006` upgrade table SHA-256 is
`5c7a104f37eab018d15719066052c2db25ef046d9330770045d7e4977f98c12b`.
The DAT files are column-major and little-endian. The action records
below are 24-byte big-endian records in the normalized ROM.

| Covert Ops research | DAT row | Minerals / gas | Source ticks | Energy per cast | Icon | Action record |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| Personnel Cloaking | Tech 10 | 100 / 100 | 1,200 | 25 | 252 | `0x0C4970` |
| Lockdown | Tech 1 | 200 / 200 | 1,500 | 100 | 240 | `0x0C4988` |
| Ocular Implants | Upgrade 20 | 100 / 100 | 2,500 | — | 249 | `0x0C49A0` |
| Moebius Reactor | Upgrade 21 | 150 / 150 | 2,500 | — | 256 | `0x0C49B8` |

Both upgrade rows have a maximum level of 1. The mineral, gas, timer,
energy and icon columns of the tech table begin at byte offsets 0, 88,
176, 264 and 528. The corresponding upgrade base fields and icon are at
0, 244, 488 and 854; the max-level byte column begins at 1159.
Ghost actions for cloak/decloak use icons 252/253 at ROM offsets
`0x0C3FE0` and `0x0C3FF8`; Lockdown uses icon 240 at `0x0C4040`.
These are data/button facts, independent of a live timing capture.
In `00C/004` (`orders.dat`, SHA-256
`0b79cc64f1cd8b23c77419bdf1fb39fb91cde1359c1ad9cb73884d5cceb8f7c9`),
orders 109/110 are interruptible background cloak/decloak orders and
cannot queue. The generic order lists tech 9 (Wraith cloaking), while
the Ghost button binds Personnel Cloaking, tech 10. Lockdown order 115
is an interruptible targeted main order, cannot queue, and references
weapon 32, tech 1, CastSpell animation 7 and icon 240.

The T8 CHK (`008/011`, SHA-256
`7c765fc6b8ccfd2ee63c321499dce934a25a81267aa6f8822149c0313facc3a2`)
has two human source-slot-0 Ghosts. Both have valid property mask
`0x000B`, including explicit energy 100%, and neither has initial
cloak state. PTEC and PTEx authorize both spells but start them
unresearched; UPGR and PUPx allow level one for both upgrades and start
them at zero. The T10 CHK (`008/013`, SHA-256
`d1bcc9759b948e95ad565537ed3c29eb980cd325935aa25480dc0ad4a8298b6f`)
instead seeds both spells and both level-one upgrades for source slots
0 and 6, but not human slot 1. Do not give every placed Ghost the
training default or every campaign owner the same research.

The Ghost raw-1 unit entry has sight 9 tiles at byte 8173 in
`00C/001`. Its special flags at byte 7036 are `0x18210200`
(organic, has energy, no mechanical flag). Ocular raises sight to
11 tiles in the [pinned PC reference](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L11680-L11685),
which also uses sight for the nuclear designator range. Moebius raises
maximum energy from 200 to 250, so a new trained Ghost starts at 50 or
62.5 respectively; a valid placed-unit override takes precedence.
While actively cloaked without passive cloak, the PC reference drains
10 Q8 units per source update and decloaks when the next debit is not
possible; otherwise it regenerates 8 up to the current cap.

## Paid state and integration

The model admits one job per producer, one concurrent copy of a given
research per payer, and separate completion for each authenticated
owner key. It charges both resources at admission. The runtime caller
must confirm an owned completed raw-117 Covert Ops with a landed
raw-116 Science Facility at the correct add-on socket. One tick is
one source update, independent of video rate. Detached or foreign
control freezes the receipt until the original payer regains a working
attachment; this is a staged policy pending native detach/resume
verification. Manual cancellation returns the full cost, and
destruction returns three quarters per resource, matching the
[PC reference](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L3294-L3323).
Only the authenticated original payer receives either refund.

Reuse the existing Physics Lab `level`/`queue_progress` lifecycle,
but assign Ghost owner bits independently and only add them to a
new, authenticated save layout. The shared Nuclear Silo stage already
provides `race_nuke_parent_slot` for the attachment check. Ocular
must update both the targeting radius and the actual fog footprint,
including for a stationary Ghost; include the upgraded radius in the
visibility snapshot key. The energy arithmetic reuses the existing
Battlecruiser Q8 helpers. Cloak needs shared detection and rendering,
not the one-off Kerrigan adapters. A selected player's HUD energy
already routes through `sc64_unit_energy`.

Lockdown raw weapon 32 in `00C/007` has target flags `0x17`
(air and ground mechanical units, no buildings), range 256 source
pixels, no damage, follow-target bullet type 1, lifetime 255 and
Lockdown hit type 4. The native projectile chain is flingy 146 →
sprite 345 → image 529 (17 headings, 16×16, BOLT `00E/0FD`,
SHA-256 `06c3125ce8b918257c3ab63d05aaa8e33f205e485a270d70e8d64b58a78c3622`).
Flingy speed is 8,533 Q8 and acceleration 267 Q8, with halt distance
136,352 Q8. The source origin is 25 pixels ahead of the Ghost.
Ghost image 228 IScript 70 plays sound 240 and releases its cast
after one source tick, then draws recoil base 204 for two source
ticks. Projectile IScript 240 plays sound 80 on Init and sound 9 with
impact image 530 on Death. Impact image 530 (`00E/0FE`) has ten
24×24 frames played for two source ticks each (IScript 313). The
separate status fields are images 361/362/363 (`00E/087`–`00E/089`,
36×36/42×42/54×54). Their shared IScript 245 plays 0 through 6, then
loops 7 through 12 and 11 through 8, two ticks per frame, before
playing frame 13 for two ticks on status removal. The impact and
status fields use source palette modifier 9, color shift 1.

The PC model disables on impact for 131 timer
decrements, one per eight source updates, and Restoration can clear it.
It also stops movement, interrupts applicable orders, makes the
status-affected target detectable, and detaches a disabled Ghost
from a missile still waiting for descent. An ordinary Ghost is not a
mechanical Lockdown target. Pending Lockdown requires a bounded moving
target handle rather than an instant damage event.

`make test-sc64-ghost-tech` requires no ROM data. It exercises all four
exact timers, independent owners, attachment interruption, two refunds,
scenario seeding, Ocular sight, paid energy and cloak drain arithmetic.
No native spell timing or field controls are qualified by this test.
