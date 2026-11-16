# Ghost Covert Ops research

The pure rules model all four paid jobs at a Covert Ops. The separately
enabled `SC64_GHOST_TECH_PROBE` exposes only Ocular Implants and Moebius
Reactor in the normal field command menu. Cloak and Lockdown remain
unavailable. This stage adds no ROM assets or new save fields.

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

Across the original campaign catalogue, every placed raw-1 Ghost uses
valid UNIT energy 100%, and the Ocular/Moebius price and time selectors
in UPGS/UPGx use stock data. Upgrade availability differs: T1–T3
disallow both for slot 1; T5 disallows both for slots 0 and 3, and seeds
only Moebius for slot 6; T6 and T7 disallow both for slot 0. The T10
seeded owners already have both. Remaining original campaign PUPx/UPGR
rows admit both as unresearched. Only the two upgrade seeds are applied
in this runtime stage.

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

The Ghost upgrades use separate owner bits and a bounded 16-entry
receipt pool; an attached receipt caches its parent slot and rechecks
the live socket, owner key and flight state before ticking. Ocular
updates the targeting radius and the actual fog footprint, including
for a stationary Ghost. The energy arithmetic reuses the existing
Battlecruiser Q8 helpers; the selected unit HUD shows the current
energy and Moebius cap. Cloak still needs shared detection and
rendering.

The current Silo journal remains usable for source-seeded worlds.
If a Ghost upgrade has been paid for or earned, this experimental
build refuses to overwrite a save until an authenticated Ghost tech
layout exists. Canceling an unfinished upgrade can reenable saves
if no other nonserializable state remains. The active-strike journal
can still save supported painting or descent when no Ghost tech state
would be lost. The Silo-only journal still rejects flights.

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
With prepared inputs, `make test-sc64-ghost-tech-world` drives actual
B, D-pad and A inputs in the Terran HOST profile and checks the
research jobs, flight/attachment, static fog expansion, new range,
energy, scenario seeds and an unchanged live Silo save.
`make test-sc64-ghost-tech-world-active` runs the same profile with the
active-strike journal, including a supported painting save before an
upgrade starts and refusal to overwrite it once research is paid for.
These tests do not qualify native field spell timing or the two
unimplemented spells.

## Detached save experiment

`make test-sc64-ghost-tech-save-wire` verifies a HOST-only projection on
the `556ab0d` Ghost probe. This does not change what the game can save.
It does not allocate a journal tag, change a default build, or install a
reader or writer. Existing transport, nuclear, and research tags retain
their current meaning.

A fixed tail cannot preserve every legal predecessor: Z7, Z9, Z10 and
P10 have valid impact, transport and Yamato combinations that already
fill the [16,368-byte payload](../SC64-JOURNAL-DEBT-CAPACITY-AUDIT-20261107.md).
The experiment adds no bytes to those layouts:

- The 16 possible earned Ocular/Moebius bits are a two-bit-per-source-key
  delta from the authenticated map's seeds. An explicitly selected
  candidate layout XORs the delta into the low half of the known building
  damage marker in `state[25]`. Detached decoding restores the full
  canonical marker. A predecessor keeps its original marker, including
  zero, and restores only the scenario's seeds.
- Raw-117 Covert Ops rows use their empty `queue_progress` for up to 2,500
  remaining source ticks and their empty `level` for one of the 16
  payer/upgrade pairs. A receipt requires a live, complete Covert Ops and
  an independently validated same-owner, same-source-key Science Facility
  socket. The payer key must agree with the candidate's active bank and
  research permission. Unsupported, duplicated, foreign, and detached
  receipts refuse; no partial load plan is returned.
- Raw-108 Silo receipts, raw-14 active missile rows, typed Protoss
  research padding, and the transport/impact trailers keep their current
  projections. The fixture passes an armed Silo and painting strike on the
  same candidate through the existing nuclear codecs alongside both a paid
  upgrade and an earned upgrade. It rejects altered rows atomically and
  still accepts the preceding nuclear-only payload as a base layout.

A game integration needs its own reviewed exact outer/inner layout
authorization. The low marker bits or a nonzero Covert row must never
select it. The writer must check that decoded receipts reproduce the full
live pending mask before any SRAM write. The reader must use only detached
candidate data, normalize the marker and Covert receipt fields before
ordinary validation, and publish research, silo, and flight state only
after the entire candidate passes. Reserve the existing slot and sequence
selection rules so invalid new candidates can fall back to a compatible
predecessor. Source-key/owner mapping and socket derivation must be checked
on every admitted profile; holding detached or captured work requires a
separate original-payer proof. Also verify Ghost Q8 energy with each unit
row layout, including full Moebius energy and legacy signed storage.
Before a cartridge enables this, run independent writer/reader processes
on real predecessors, malformed newest records, full payloads, and
painting/descent saves, then qualify the normal ARM path and its guards.

## First live Terran custom journal (November 16)

`make test-sc64-ghost-covert-journal` runs three Terran HOST processes
against prepared inputs. The new opt-in `SC64_GHOST_COVERT_HOST_JOURNAL`
requires the authenticated active-strike and Terran packed save vector.
That HOST flag remains unavailable to ARM; a separate preview is below.
Neither flag combines with Carrier or other research journals. Campaigns
still accept and write their existing baseline
format, and still refuse paid or earned changes the baseline cannot
carry. No regular build opts in.

The exact new outer namespace is `0x4f`; its inner version is the
unchanged current Terran version. Only the journal selector can authorize
that pair. The detached adapter normalizes the building damage marker and
paid Covert rows before ordinary validation. It confirms occupied Terran
payers, checks each unique grounded Science Facility socket, and checks
live Ghost Q8 energy against the candidate's restored owner cap. A
receipt without a current unique same-owner socket refuses before a
write. Publication happens after selection and load preflight; the already
validated strike and Silo projections keep their existing wire layout.
Neither the fixed base nor the variable impact/Yamato/transport tail grows.

On the authored custom map, the fixture saved a nuclear-only painting
state, then paid for Moebius Reactor and Ocular Implants and saved an
active painting state with Moebius earned and Ocular still pending. The
new payload had the same 15,832 bytes as its predecessor. It wrote the
other bank and left the earlier bank byte-identical. A fresh reader
restored the 250-point Q8 Ghost, Ocular's remaining paid time, and the
active strike, then wrote another combined snapshot. A checksum-valid
newer receipt with an excessive timer fell back to the old nuclear-only
painting state without writing SRAM. With the old bank absent, it
refused without changing the live frame or strike. A detached live
receipt also refused without overwriting either bank.

These are scoped HOST fixtures. Remaining work includes both custom maps
and the full lobby matrix, saved descent alongside separately armed
Silos, saturated ordinary tails, campaign payer provenance, composition
with Carrier and the other research journals, and ARM checks.

### Detached reader stack reduction

The reader scans building rows in slot order and retains parent indices only
for accepted receipts. It validates every row and checks each paid row's clean
encoding before normalizing scratch. A malformed last slot cannot publish a
partial result. The previous Terran reader's 96-slot facts, fields, and parent
arrays alone represented 4,800 stack bytes. An asset-free ARM7TDMI Thumb
compile with GCC 16.2.0 and `-Os -fstack-usage` measures the new scan type at
128 bytes, its 16-parent array at 16 bytes, and the isolated pure wrapper at
192 bytes of static stack. The game adapter additionally needs one building
row and its codec/socket callees; the pure measurement is neither a
linked-game frame bound nor a hardware stack high-water measurement.

Pure research, stream/codec, and Nuclear Strike row tests pass under
ASan/UBSan. After the disk reserve recovered, the prepared writer, fresh
reader, and malformed-record fixture also passed under ASan/UBSan on
`72eb7623aa01e369d0d09ae4a089ee935759e709`. Free-space checks passed at
5,828,653,056 bytes before compilation and 5,787,873,280 before running.
The compact payload was still 15,832 bytes, and the complete 32 KiB SRAM
image byte-matched the earlier receipt (SHA-256
`5daa6912f252b20d86dde7f5ac414dbd9e99b1e9bdb3dff2a91e414d6f057f82`).
The same fresh-load and malformed-newer fallback checks passed without
changing either receipt.

### Separate Terran ARM preview

`make sc64-ghost-covert-arm` builds the explicit Terran-only Covert cart
`build/StarCraft-Nuke-Probe-terran-covert-GBA.gba` (game code `SCCT`).
The stock Terran and base Nuclear Strike preview targets do not use
`SC64_GHOST_COVERT_ARM_PROBE`. This separate ARM gate requires the same
active-strike and packed-building vector as the HOST fixture. Both gates
share the serializer and the same authenticated `0x4f` envelope; there is
no format change for existing HOST, nuclear-only, or campaign saves.

One link using the prepared inputs passed at 5,745,647,616 bytes free
against the 5.25 GiB disk floor. Artifacts were 73,853,469 bytes total.
The unmodified `__iwram_end < 0x03005000` and `__bss_end < 0x0203e000`
guards passed: actual endpoints were `0x03004ba0` and `0x0203a040`
(1,120 and 16,320 bytes below their guards). The ROM was 33,554,432 bytes
with a verified header and SHA-256
`543bf2287fd2a4b32402a065ff46d39ab1101bc6642595cf43ba238a9cd7ac38`.
The corresponding ELF SHA-256 was
`362e59f87e7e2a42c032e2287ffe6906fc26135b009f13348a3864744e327338`.
Disassembly shows the streamed reader reserving 416 bytes for its own
frame (380 locals and 36 register saves) and its matching parent finder
reserving 136 bytes for its own frame. These omit other callees, callers
and interrupts, so they do not bound the total load stack.

No emulator run was started. Ordinary ARM inputs, HOST-to-ARM SRAM
migration/fallback, active descent, and measured combined stack and
hardware timing remain open; neither preview is a release activation.
