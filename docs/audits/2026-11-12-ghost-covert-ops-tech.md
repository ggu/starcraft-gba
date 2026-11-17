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

### Bounded physical ARM research and reload

On cart `SCCT` from `84a6833`, the pinned read-only observer launched a
blank normal Custom Game as Terran on map 0 with one computer opponent.
The paid trial started separately from the authenticated earlier slot of
the HOST fixture: the original nuclear-only bank was retained verbatim,
and the other bank was erased in a separate input image. No part of the
blank match constructed the donor's Covert Ops, Ghost, Silo, or economy.

In the paid trial, ordinary title L restored the donor at frame 1,874
with a painting strike. The Ghost's field menu aborted that strike.
Ordinary Covert commands spent 150 minerals and 150 gas on Moebius at
frame 2,047, let it complete in play, then spent 100 minerals and 100 gas
on Ocular at frame 5,252. An ordinary START/B pause save wrote a
15,832-byte combined custom payload and left its older nuclear-only
bank byte-identical. The new record had Moebius earned, Ocular pending with
2,499 source ticks remaining, and 34,416 Q8 Ghost energy. A fresh process
loaded it through ordinary title L and matched the frame, research masks,
paid job, Ghost energy, and 2,410/1,050 bank exactly.

The run used no guest memory writes or emulator state restoration. The
5.25 GiB disk guard passed at start (5,669,859,328 bytes free) and finish
(5,667,770,368 bytes free). The same local observer had SHA-256
`4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549`.

| Local receipt | SHA-256 |
| --- | --- |
| Validated input plan | `54cc1c083e3b4fa55f6ce3e49ca5cc3c38cf7f00f22ab5004e95a95505eb2eb5` |
| Controller inputs | `8ff6cf6ff6f3fb9dcff6a45d99f1af231903451b2aac9722c9310d2ef3465cbb` |
| Nuclear-only predecessor input | `8286f5397e1c5d5536043f154fb39d930032ceed2cf40a25435b669585e98727` |
| Actual ARM-written SRAM | `34bfc1bb7bd6d2f412d72c139408cfaeea3969f6ed8de2b7e230cb5d34031c85` |
| Save and restore result | `c414dec6e778863eb986cba6eaec8b87efdc05b47ac0bb289a8a287a0faeb6ea` |

### Combined physical ARM painting round trip

This follow-up used the same pinned `SCCT` ROM and read-only observer.
The starting SRAM was written by the paid ARM trial above, so this was
a continuation from a prepared economy. At frame 5,252, title L loaded
Moebius earned, Ocular pending (2,499 source ticks), and an empty Silo.

To repeat the proof, build `make sc64-ghost-covert-arm` with prepared
inputs and use a working copy of a save in that state. In the match,
press A to select the empty Silo, use B and the D-pad to find its missile
command, and confirm with A. Verify the 200/200 resource debit and wait
until the missile is armed while Ocular is still pending. Select the
Ghost, open its B menu and confirm the nuclear targeting command. Aim
near the Ghost, press A, then use START and B immediately to save while
the Ghost is still painting. Start a fresh emulator process with the
resulting SRAM and press L at the title. Compare the research masks,
receipt and remaining timer, Ghost energy, world frame and resources, and
the active strike's owner, target, origin, spotter, phase, age, timers,
and reserved missile slot. Check the new bank's header and checksum
and compare the prior bank byte for byte. Use controller input and
read-only observation; the proof needs no guest writes or save states.

The missile was armed at frame 7,362 with 811 Ocular ticks left. The
save and fresh load both had frame 7,520, 685 Ocular ticks, Moebius
earned, 48,928 Q8 Ghost energy, and 3,138/850 resources. The single
strike was in painting at age 1, targeting `(420, 408)` from Silo
`(672, 728)`, with Ghost slot 8 and reserved missile slot 28. The
writer retained the 15,832-byte payload, wrote tag `0x764f5442` at
sequence 3, and preserved the earlier bank byte for byte. The 5.25 GiB
disk guard passed before and after the run. The pinned local artifacts
are identified here by hash; they are not build or test dependencies.

| Painting receipt | SHA-256 |
| --- | --- |
| Controller inputs | `0c7fdc50c623210e5533a299550a9a6705657b1fec9e13c3d9feea2f02268f67` |
| Actual ARM-written SRAM | `ab2be6ed9fbf9e1ffb94ce254cd840e3c9953a3a0dfe883c1d891538ecdd1f43` |
| Save and restore result | `5f708021fa1084c18d43cad224546a93f86666cc804cc1eaf014899da70e0204` |

### Combined physical ARM descent and impact

The same `SCCT` ROM and observer started from the actual painting SRAM
above. Press L at the title and let the match run with neutral input.
The strike still had Ghost slot 8 at frame 7,942 (painting age 339) and
released it on frame 7,943 (descent age 340). To check the descent save,
use START then B once the Ghost is released but before impact. Resume
with START and observe the reference impact without reloading. Start a
fresh emulator with the new SRAM, press L at the title, and use neutral
input again. Compare the saved fields and sample both runs one frame
before impact, at impact, and one frame after it.

The save and fresh load matched at frame 7,951: Moebius earned, Ocular
pending for 340 source ticks, 51,688 Q8 Ghost energy, and 3,314/850
resources. The single strike matched in owner, age (346), phase, timers,
target, origin, absent spotter, and reservation bitmap (missile slot 28).
The other bank retained the prior painting save byte for byte. The new
bank verified as tag `0x764f5442`, sequence 4, with a 15,832-byte payload
and the expected paid, earned, and independent missile records.

In both runs the blast fired at frame 8,051 and released the missile
slot. Across the live unit and building rows, the observed HP and shield
losses and deaths on that frame matched: the blast killed the player's
45 HP Ghost and reduced the Science Facility from 850 to 710 HP and
the Covert Ops from 750 to 626 HP. On the following frame the impact
animation advanced without reactivating the strike or its reservation.
This compares the strike, its paired research state, and the damage on
the impact frame; it does not establish full simulation equality after
reload. No guest memory was written or emulator state restored. The
5.25 GiB guard passed at start and finish (6,106,324,992 and
6,102,024,192 bytes free).

| Descent receipt | SHA-256 |
| --- | --- |
| Controller inputs | `5023c84010e628761ce4d181078d31653b63d1b8632064831592d041421253e6` |
| Actual ARM-written SRAM | `98acc32fa1653c91d2e9bb077f2479f5167ba7d7593be83902bfba83475cf758` |
| Save, restore, and impact result | `1f91847f1cfa43b000c53b04c4b5b1d9702e724570d320b45d81248d588da689` |

### Checksum-valid newer ARM record falls back

The descent SRAM above held the genuine painting save in bank 0
(sequence 3) and the genuine descent save in bank 1 (sequence 4).
In a separate copy, change only the newer bank's Covert Ops slot 3
`queue_progress`, an unsigned 16-bit value at bit 103 of that packed
building row. Set it to 2,501 instead of 340: the packed field can
represent it, but a paid Ocular receipt may have at most 2,500 source
ticks. Recompute the newer payload's outer FNV-1a checksum. Only seven
bytes differ: three timer bytes at zero-based SRAM offsets
23,796–23,798 and four checksum bytes at 16,392–16,395. Both records
still pass their envelope checks. The newer record kept its sequence,
length, tag, earned marker, and descending missile; the older bank
remained byte-identical.

Start the same `SCCT` ROM in a fresh emulator with that copy and press
L on the title. The loader restored frame 7,520 with Ocular pending
for 685 ticks, Moebius earned, and the Ghost painting at age 1. The
lobby, resources, energy, paid receipt, full strike, and missile
reservation matched the independently observed painting save above.
Cloning SRAM after the load returned the same 32 KiB, including the
malformed newer bank. The input copy and original descent SRAM also
remained unchanged. No guest memory writes or emulator save states
were used; the existing disk guard passed before and after the run.

| Fallback receipt | SHA-256 |
| --- | --- |
| Malformed input and SRAM after title L | `edd6c0a6983157ad969b64a09ba6b9b7326b58f8a67a7c3ddf4603a4c61dc13c` |
| Restore and unchanged SRAM result | `6e9da77ff74ce0ddf41be9c83610fc59d457deb2504f90db985c2bcd901ec6fc` |

### Blank Blood Bath opening through Barracks

The same `SCCT` cart and read-only observer launched without any SRAM
input. In Custom Game, select Blood Bath (map 1), choose Terran, open
the third CPU row as Protoss, then close the first two CPU rows and
launch. The running lobby used mask 4 and starting positions 0 and 3.
At frame 0 the human had 50 minerals, no gas, one completed Command
Center, four SCVs already assigned to minerals, and no Ghost research.

Leave the SCVs mining, then open B and choose Barracks. Use the D-pad
to find a valid build site and confirm with A. The bank reached 154
minerals by frame 385. At frame 401, the placement at `(112, 120)`
gave SCV slot 0 a pending construction order and the bank was still 154.
The unfinished foundation appeared in building slot 2 at frame 429,
when the bank was exactly 150 minerals lower. At frame 1,941 the same
Barracks was complete at 1,000 HP, with the bank at 308 minerals and no
gas. No save was loaded or written, no guest memory was written, and
the 5.25 GiB disk guard passed at both ends.

| Blank Blood Bath receipt | SHA-256 |
| --- | --- |
| Controller inputs | `1e78300ea8c3d06cd9444ec134b79e65bfafd1dbfcb1a3118e0d836944453d15` |
| Opening, foundation, and completion result | `e9fd821c60639dae84eb65ba73cc7d73e198c73eead7c8db79d4894b26b60820` |

### Blank Blood Bath Refinery and first gas deposit

The same ROM started without SRAM and replayed the earlier controller
inputs, matching the completed Barracks and its wallet at frame 1,941.
Select the original builder, open B, choose Refinery (100 minerals),
and use A on the home geyser at `(32, 208)`. SCV slot 0 accepted the
order at frame 2,009. Building slot 5 held its foundation by frame
2,077. The sampled wallet went from 332 to 240 around the 100-mineral
construction charge, a net 92 because mining continued. The same
Refinery finished at frame 2,837 with 750 HP.

With the original SCV selected, press A on the completed Refinery.
The worker took the gas order for Refinery slot 5 at frame 2,840,
carried eight gas at frame 2,918 with the bank still at zero, and
deposited it by frame 2,958. Its cargo and gas flag cleared on deposit.
SCVs 1, 2, and 3 continued their mineral orders and the mineral bank
rose from 376 to 400 during the gas trip. No save or guest memory was
written. Disk guards passed at start and finish (15,631,990,784 and
15,630,901,248 bytes free).

| Blank gas receipt | SHA-256 |
| --- | --- |
| Controller inputs | `aa27ab6dcb68b3e3c87e1c9a6c0dc1de1781ac90cc090cf10503447421696753` |
| Refinery, cargo, and deposit result | `dbcfdcd678820ee0b102982bf93aa46189e4d90cdd8118b0100c054c46798819` |

### Blank Blood Bath Factory affordability

A blank replay repeated the Barracks and gas checkpoints. At frame
2,994, an ordinary Command Center training order paid 50 minerals for
a fifth SCV (408 to 358 minerals, with gas unchanged at eight). At
frame 4,564 the bank was 766 minerals and 104 gas, enough for a
Factory. The fifth SCV was alive and mining in slot 16. SCV 0 still
worked the Refinery and SCVs 1–3 still had mineral orders.

The fifth SCV disappeared from its slot during the following cursor
selection. The replay did not establish why and stopped before placing
a Factory. It produced no save. In the next attempt, select the newly
trained worker as soon as it appears and keep it available near the
intended site while the original four workers gather. If enemies reach
the base, use ordinary combat orders and verify the builder is still
alive before making the paid placement.

| Blank Factory affordability receipt | SHA-256 |
| --- | --- |
| Controller inputs | `b9e0597991f0e8197fe263cf81fe37a09f340936fdd81ed05f953a85371b866e` |
| Paid SCV and funded Factory checkpoint | `759b326120d31620074653872a85727e1654a9448ae212bdcd50819982ebd89d` |

### Blank Blood Bath Factory and baseline journal

The successful replay kept the same blank opening and first gas
deposit. The earlier controller trace pressed SELECT with an empty
selection, which opens Pause; in Pause, R is Restart. The corrected
controls skip that SELECT, verify play stays active, and compare the
SCV's direct and aligned memory reads.

The Command Center queued the fifth SCV for 50 minerals at frame
2,994, and the Barracks queued three Marines at 50 minerals each
between frames 3,013 and 3,017. The fifth SCV appeared by frame
3,371 and ordinary A selected it at frame 3,381 with 60 HP. Use the
B menu's Move command to stage it near `(152, 160)` before the
Factory is affordable. The original three mineral workers and gas
worker continued gathering; the bank reached 520 minerals and 104
gas by frame 4,570. All three Marines were alive at that observation.

With the staged SCV still selected, choose Factory from B and confirm
at `(152, 120)` with A. The game placed building slot 11 at frame
4,584 and charged exactly 200 minerals and 100 gas (520/104 to 320/4).
The same Factory was complete by frame 6,096 at 1,250 HP and the bank
was 616/92.

START then B wrote a 32 KiB SRAM image with an ordinary 15,832-byte
payload, tag `0x76435442`, sequence 1, and a valid checksum. A fresh
process used title L and matched the frame, resources, sparse lobby,
Factory, Barracks, Refinery, original workers' positions/orders/cargo,
and empty Ghost research. This is a baseline continuation from an
unseeded match; it has no paid Covert receipt or nuclear state yet. No
guest memory writes or emulator save states were used. Disk guards
passed (15,025,537,024 bytes free before and 15,021,035,520 after).

| Blank Factory receipt | SHA-256 |
| --- | --- |
| Controller inputs | `32c082db6c817b91021d9eda4d2b0747e986627052807ddb338a3260f8224b96` |
| Actual ARM-written SRAM | `e280778b66f70065b36306daa432c9247fd6b5d6612edeb7a59f73ae233b316b` |
| Build, save, and fresh load result | `eabc11398ae49ea4e194af1995dda4fe32614b8cee0eec7c0f73e66f71dce42c` |

### Blank-origin Blood Bath Starport and Science Facility

A fresh `SCCT` process used ordinary title L on the Factory SRAM
above and matched the frame 6,096 checkpoint at 616 minerals and 92
gas. Two ordinary Marine orders paid 50 minerals each. The original
three mineral SCVs and one gas SCV continued gathering. Reuse the
fifth builder and select Starport from B, move to `(176, 152)`, and
confirm with A once the wallet holds at least 150 minerals and 100
gas. The foundation in slot 17 charged the exact cost at frame
6,203 (540/100 to 390/0). The same Starport was complete at frame
7,523 with 1,300 HP.

Keep the same gas and mineral orders while waiting for Science
Facility (100/150), then select it from B and confirm a site with a
clear add-on footprint. At `(216, 168)`, the SCV reached the site
and paid the exact 100/150 at frame 8,769 (894/152 to 794/2).
Building slot 23 was complete by frame 9,909 with 850 HP and the
wallet at 1,002/66. The prospective Covert Ops socket `(264, 176)`
passed a separate terrain, resource, and current collision check;
this does not exercise the add-on command itself.

Ordinary START/B appended sequence 2 in the other SRAM bank with
tag `0x76435442` and a 15,832-byte payload. The original Factory
bank remained byte-identical. A fresh title-L load matched the frame,
wallet, sparse lobby, completed structures, fifth builder, original
workers' orders and cargo, and empty Ghost research. No guest
memory writes or emulator save states were used. The disk guard
passed (17,776,709,632 bytes free before and 17,695,813,632 after).

| Blank-origin Science Facility receipt | SHA-256 |
| --- | --- |
| Controller inputs | `bd8145313678465c42cedb25c9280dd0051a4637cb6745d1f10d9166b5f428e6` |
| Actual ARM-written SRAM | `0f6c861c0582911012c3cc0d0706c02ce3366e3afae2b6fede0c1789d53205ab` |
| Build, save, and fresh load result | `427a9bbaac20e596dc393bb5913cba15fe452f42478914fb73bc4868154331a5` |

### Blank-origin Blood Bath Covert Ops and Academy

A fresh `SCCT` process used title L on the Science Facility SRAM and
matched frame 9,909 at 1,002 minerals and 66 gas. Select the Science
Facility and choose Covert Ops in its B menu. The order paid 50/50
at frame 9,914 (1,002/66 to 952/16) and placed the owned add-on in
slot 24 at `(264, 176)`, the socket for Facility slot 23. With the
original mining and gas orders still active, the fifth SCV placed
an Academy at `(160, 192)` at frame 9,938, paying exactly 150/0.
At frame 11,438 the Academy was complete in slot 25 at 600 HP. The
same Covert Ops was also complete at the Science Facility's socket,
with 556 of 750 HP.

Supply was short of the room needed for Ghost production. An ordinary
SCV build order placed a Supply Depot in slot 27 at `(104, 168)` by
frame 11,525, charging exactly 100 minerals and no gas. At frame
12,285, while play was still active and the Depot was not observed
complete, the SCV's direct and aligned row reads agreed: slot 16 was
no longer alive and its HP was −4. The run did not identify an attacker.
It ended before a Ghost purchase and produced no new SRAM.

For the next attempt, build the needed Depot earlier or choose a
site closer to the Marine group. Keep a backup SCV available, use
ordinary orders to defend the builder, and save as soon as the core
infrastructure is complete so a later unit loss cannot erase the
proof of the preceding build sequence.

| Blank-origin Covert and Academy receipt | SHA-256 |
| --- | --- |
| Controller inputs | `f636a1af068a96427d397a00fe0027d6e6fb644c26c67ed27e820a56298b7801` |
| Paid structures, completions, and builder stop | `d65319d839fab21908b82a85b215fd4e0f72d7f89baf58609b28c5af095fd086` |

### Earlier Blood Bath Depot and Academy limit

A second replay repeated the same Science checkpoint and paid Covert
order. A clean R tap selected five Marines, and B Attack followed by
A sent them toward `(216, 216)` at frame 9,934. The fifth SCV placed
the earlier Depot at `(176, 216)` and paid 100 minerals at frame
9,960 (976/24 to 876/24). Slot 25 completed at frame 10,720 with
500 HP. The same SCV then paid 150 minerals for Academy slot 26 at
`(160, 192)` at frame 10,741 (1,020/72 to 870/72).

At frame 11,541 the match was still active and the Academy was not
observed complete. Direct and aligned bytes agreed that SCV slot 16
was dead at −2 HP. The run did not identify an attacker. It stopped
before the planned infrastructure save or Ghost production. The
earlier Depot succeeded; serializing both structures delayed the
start of the Academy by about 800 updates compared with the first
replay.

The pinned preview's CPU planner can assign a replacement SCV to
an abandoned foundation, but its ordinary human A-click path has no
construction resume command. Its repair hooks are campaign gated and
require complete structures. A safer continuation on this ROM is to
order the Academy and nearby Depot in parallel, temporarily divert
one mineral SCV, and save as soon as the Depot and Covert Ops are
complete. Restore the diverted worker to minerals before saving and
observe the Academy separately. A player-accessible resume command
would be a separate engine change.

| Earlier Depot receipt | SHA-256 |
| --- | --- |
| Controller inputs | `cb0636f31d698138b3b922e2b52a8f192d7a3d81444cb10089acf7b6381335d8` |
| Depot completion, paid Academy, and builder stop | `194f74b22f2afdcf028150a59dca40e7270cbfd66b841350b802ad91a09d75eb` |

### Parallel Blood Bath construction and paid Ghost

Another run started from the same verified Science Facility SRAM.
The Covert Ops cost 50/50 at frame 9,914. SCV 16 paid 150 minerals
for Academy slot 25 at frame 9,938; SCV 3 paid 100 minerals for Depot
slot 26 at frame 10,139. Both paid foundations were under construction
at that point with different SCVs. Five Marines took an ordinary
attack order toward `(216, 216)` at frame 10,156.

By frame 10,888 the Depot was complete at 500 HP and the attached
Covert Ops was complete at 750 HP. An ordinary A click on the
original mineral patch put SCV 3 back on its previous mineral order
at frame 10,972. START/B then wrote the intermediate baseline
checkpoint at 854 minerals and 80 gas. The Academy was still
under construction at 1,035 progress and 432 HP. The new bank had
tag `0x76435442`, sequence 3, a valid checksum and a 15,832-byte
payload. The prior Science Facility bank was byte-identical. This
run did not independently reload the intermediate save.

After START resumed the same world, Academy slot 25 completed at
frame 11,442 with 600 HP. R at Barracks slot 2 selected Ghost, and A
queued it at frame 11,478. The wallet went from 950 minerals/112 gas
to 925/37, the exact 25/75 charge. During the bounded wait for birth,
the replay stopped because at least one original gather order or
worker no longer matched the harness's predicate. It did not identify
which worker changed or why. The Ghost queue is verified; its birth,
starting energy, and a fresh Ghost save and load remain unverified.
The original Science input and the new infrastructure save were
retained separately.

| Parallel construction receipt | SHA-256 |
| --- | --- |
| Controller inputs | `2ac99df70977d3812399298da27fb093366c60ef4447db7cdf0ef9dd8c341495` |
| ARM-written infrastructure SRAM | `82075f29dc1a4c0ddc2257ded34ec4ed6d1f9f3cc62993cb6c0d5c165028a191` |
| Parallel builds, checkpoint, and Ghost payment result | `032dfa33cb8ad0f731075129293941d10ad9bc1694d7529bd3eed7e77124ffa4` |

### Fresh parallel-infrastructure reload

A fresh `SCCT` process loaded the actual sequence 3 save through title
L and matched frame 10,972, the 854/80 wallet, the sparse lobby, the
completed Depot and attached Covert Ops, the original gather orders
and cargo, and the Academy's unfinished 1,035 progress and 432 HP.
SCV 16 was still building that Academy. The replay read each worker
separately, including the builder.

An ordinary Move order regrouped all five existing Marines toward
`(128, 80)` at frame 11,001. SCV 3 paid exactly 100 minerals for a
Bunker at `(112, 160)` at frame 11,050 (862/88 to 762/88). The
reloaded Academy completed at frame 11,440 with 572 of 600 HP.
The attached Covert Ops still qualified and the Barracks offered
Ghost through R. The replay then stopped because its combined queue
item and wallet check after A failed. It did not log which of those
fields differed. There was no new queued or live Ghost save; the
earlier paid Ghost result above came from a separate continuation
with different defense orders.

| Infrastructure reload receipt | SHA-256 |
| --- | --- |
| Controller inputs | `9f5e56627f7308fcb9415d3c7e50a607a57c6e3081db38cf1f8de6e50200d725` |
| Fresh load, defensive orders, and Academy completion result | `7e167d32d9b91c6cb529c321d6ac59745daaa85a1a9b0bccb49795a91e7a1406` |

### Exact Blood Bath Ghost purchase and queued SRAM

The next fresh title-L run replayed the successful purchase controls
from the sequence 3 infrastructure save without the defense detour.
Leave the world neutral until the Academy finishes, then select the
Barracks at `(112, 120)`, use a clean R tap to select Ghost, and press
A to train. Both sides of that last A were recorded. At frame 11,477,
Barracks slot 2 was selected with 1,000 HP, an empty queue, and type
28 selected. Supply was 10/36 and the wallet was 950/112. At frame
11,478 the wallet was 925/37, the queue contained exactly type 28 at
progress 1, and the game reported training with no refusal.

Ordinary START/B exported a 32 KiB queued-Ghost save. The harness
stopped at its save verification, so it did not resume training or
reload that save. An independent offline check of the retained bytes
confirmed the original sequence 3 bank was unchanged. The new bank
was valid tag `0x76435442`, sequence 4, with the exact frame/wallet
and Barracks queue above. Its 15,852-byte payload includes a 20-byte
`IMP1` trailer with two delayed impacts; the previously observed
empty-tail size of 15,832 bytes does not apply during this fight.
The artifact contains a paid Ghost in the queue; no Ghost birth was
observed.

| Exact paid Ghost receipt | SHA-256 |
| --- | --- |
| Controller inputs | `817fde9931a607faf44a0950660df1b7a9e65b0e1b91fa996ed750c54c4ec10b` |
| Tracked wallet, queue, and UI result | `dc2b9234daa52e2dc3a7e275b5832324ecc4aef133511d087b25fbfa2a4a71b3` |
| ARM-written queued Ghost SRAM | `bb33c53e5a6189056738743ceed0e6e6709186e4e1d35fbe4389b6d71ed27d8d` |

The next milestones below follow this preview's custom-game catalog
and costs (minerals/gas). They have not been replayed on the match
that started from blank:

1. Start from the checked sequence 4 queued-Ghost SRAM, verify the
   fresh title load, then observe the Ghost through birth. Track the
   producer, queued product, active defenses and individual workers
   separately.
   The opening Barracks blocks the
   first Command Center's socket at `(112, 128)`. Build a second
   Command Center with a clear socket and keep it free of a Comsat
   so the completed Science Facility and Covert Ops can unlock a
   Nuclear Silo (100/100) there.
2. Build enough Supply Depots (100/0 each) for the Ghost and missile.
   Arm the completed Silo from its B menu (200/200, eight supply
   and 1,500 source ticks). Pay for Ocular from the attached Covert
   Ops (100/100) and save while it's pending. The next persistence
   proof should reach ordinary title L from that newly written SRAM
   and compare the paid receipt and armed or active missile.

The combined ARM save checks cover painting, descent, and one
malformed-receipt fallback in a prepared Terran custom game on map 0
with one Terran computer. The separate blank Blood Bath checks show a
completed Barracks and Refinery, gas deposits, and a Factory, Starport,
and Science Facility built and restored over a baseline journal. They
also show an attached Covert Ops, a completed Academy and Supply Depot,
and a paid Ghost in one continuous branch with a separately verified
intermediate SRAM reload and a checked queued-Ghost save. Remaining
gates include reloading that queue, completing Ghost production,
qualifying the Covert journal in the blank-origin match,
ARM rejection without a
valid predecessor, mixed and multi-opponent journal coverage, descent
with other Silos still armed, composition with Carrier and other
research journals, campaign payer provenance, measured load stack
high-water, and hardware timing.
