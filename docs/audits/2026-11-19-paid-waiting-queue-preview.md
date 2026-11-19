# Paid waiting unit queues

The [native Blood Bath comparison](2026-11-19-n64-active-scv-cancel.md)
shows that an affordable SCV can enter a Command Center's waiting
slot at 10/10. Its predecessor keeps training. When the predecessor
finishes, the next SCV is at the front with an empty progress bar.

The preview compiles under `SC64_PAID_WAITING_UNITS` with the prepared
standard transport profiles, including P10's normal research journal.
It applies to eligible human ordinary Terran and Protoss training
queues. A paid tail counts toward the actor-pool and mission product
obligations at admission. Only the started head reserves supply.
An admitted head starts at zero, including the interval before its
first production update. A promoted head checks the current owner
and product faction's available supply. It can wait at zero visible
progress until a normal update opens room, without paying again.
Existing Egg, morph, computer and hard-200 admission still apply.
The initial head at full supply is refused, matching the independent
[native idle-producer comparison](2026-11-19-n64-active-scv-cancel.md#idle-command-center-at-1010).
The opt-in flag stays off by default.

The waiting marker fits the existing building row and is authorized
by outer SRAM namespace `0x4f`. The payload keeps its ordinary
transport or research layout. Use a separate save file with the
preview. A preview can load its compatible older bank. An older
executable can still select a retained compatible bank, and its
writer protects a foreign committed bank from overwrite. Preview
records can eventually occupy both banks.

## Ordinary ARM inputs

The Z10 preview and existing Cancel Last control run in mGBA with
read-only state observation and ordinary physical buttons. The
control exports a 9/10 Blood Bath save with an empty Command Center.
The preview loads that save and purchases two SCVs in succession.

| Frame | Visible supply | Command Center |
| ---: | ---: | --- |
| 1,903 | 9/10 | Empty after loading the existing save |
| 1,907 | 10/10 | First SCV in progress |
| 1,909 | 10/10 | First SCV in progress, second paid SCV waiting |
| 2,289 | 10/10 | Second SCV at the front, waiting for supply |
| 2,334 | 10/10 | Same paid head after a new-process title load |
| 2,424 | 10/10 | Head still waiting after 90 more updates |

Compared with a matched neutral replay at frame 1,909, the extra
purchase costs exactly 50 minerals and leaves the first SCV's timer
unchanged. The regular control reads the preceding compatible bank
when given the preview's two-bank SRAM. The preview also falls back
when an isolated copy of the newer record is mislabelled as a legacy
bank with its original valid payload checksum. Cancelling the waiting
order after a fresh load refunds 50 minerals compared with idle at
the same frame. Supply stays at 10/10 in both runs. At frame 2,336,
all 416 displayed pixels of the selected Command Center bar are the
empty shade.

A separate P10 ARM tape pays
for a Forge shields upgrade, pauses and saves with 4,999 ticks left,
and reloads the job from the new outer tag in a fresh process. The
upgrade completes through ordinary continuation. Both profiles pass
the existing link RAM guards.

The ASan/UBSan production fixture exercises cancellation, active-zero
reservation, recovery when room returns, two producers contending
for one slot, AI and Zerg morph accounting, and the hard cap.
The flag-off supply-ledger fixture checks the original accounting.
The observed N64 case covers ordinary SCVs from one human Command
Center, including automatic restart once a new Supply Depot completes.
Other producer classes and contention between native producers require
their own replay evidence.

- Z10 preview ROM SHA-256 `cf9ce65b47b52c9151f960bde49000758188b0d5c97b3858fcffe5b7575b1ee1`.
- Z10 controller receipt SHA-256 `553a7e13f2cb533d31e59f90ca108f7eef37c820775ceb188ebc6e52c4a1b13d`.
- Z10 rejected-tag and refund receipt SHA-256 `bb0c29066f0d50868d32e1ff159a095c5266445ebb3ed6c65dbf324c3b8dceaa`.
- Z10 selected waiting-bar receipt SHA-256 `0e0adbdb232da69eac0e52d975c0a2845e8f748a50ce3bbd73456624e93a9bb9`.
- P10 preview ROM SHA-256 `895d2420357ea4207b7309d9ebc0d0b7e91d725ea9fa7814da6f5fc856fc5f5a`.
- P10 research continuation receipt SHA-256 `d51695c0fc76b18d2c9c275705020cc15cca34f8d61f65807274e5a83fe8ffb8`.

## Empty and waiting producers on ARM

The Z10 preview reloads the waiting-head save, cancels the one paid
job, and tries Train at 10/10 with an empty queue. A neutral replay
at the same update has the same wallet, supply and empty queue.
Only the train attempt displays the supply refusal.

A separate branch of the same save builds a Supply Depot using the
ordinary placement menu and cursor. The head remains at zero while
the Depot is under construction. At frame 3,115 the cap is still 10.
At frame 3,125 the Depot has brought it to 18 and the queued SCV has
six ticks of progress. That SCV completes by frame 3,535 and brings
live supply to 11/18. No Train input occurs after the Depot command.
The [matching native Depot replay](2026-11-19-n64-active-scv-cancel.md#resume-the-paid-scv-after-a-supply-depot)
also advances the existing paid SCV from zero to completion after the
cap rises to 18, with no new Train input or charge.

- Preview empty-queue refusal receipt SHA-256 `6309376ff4492e27b3125678424e0d5c7078ea05b7627fe651c7f11371a125e5`.
- Preview Depot continuation receipt SHA-256 `0b1080bf567a13b0a7b49e0095e463e86be6b224cc3f3846ba12e1aeb365339d`.

## Canonical Terran profile

The opt-in Terran link passes the canonical ROM verifier with the
prepared release inputs. Ordinary Blood Bath controls with one Zerg
opponent reproduce the cap transaction on this cartridge. The regular
Terran ROM exports a 9/10 save at frame 1,903. The preview loads it,
then admits the second SCV at 10/10 at frame 1,909. The matched neutral
control confirms a 50-mineral charge and identical first-head progress.

The promoted head waits at frame 2,289 and saves at frame 2,334. Two
fresh preview processes restore the waiting state, with all 416 sampled
selected-bar pixels in the empty shade. Canceling at frame 2,338 refunds
50 minerals relative to neutral input and leaves the HUD at 10/10.
The regular Terran executable falls back to the retained 9/10 bank.
The new Terran tag is `0x764f5442`, both exported banks pass their
payload checksum checks, and the retained bank is byte-identical.

- Terran preview ROM SHA-256 `6ce5842168af1690b5e1606edba198786d5c1898eb85aba62b290285dc7731f4`.
- Terran build receipt SHA-256 `bdfc9d5b8a0630b50b519bbe4252ec742692974c9314c7c118742f0aec61d806`.
- Terran controller and save receipt SHA-256 `374efd5a3e36749200ff0dce1e6cf24a0f8053f7fae15ed59a3be168d8346b77`.

## Timing on matched work

The regular and preview Z10 ROMs replay identical ordinary inputs.
A single paid SCV reaches supply cap and finishes in the quiet case.
The three-opponent case starts with the human as Zerg and computer
seats as Zerg, Terran and Protoss. All 32 sampled states and display
cuts match across the pairs, along with every sampled route counter.

| Guest cycles per update | Regular mean | Preview mean | Regular p95 | Preview p95 |
| --- | ---: | ---: | ---: | ---: |
| 450 updates around SCV training | 561,410 | 561,403 | 602,676 | 603,611 |
| 1,000 three-opponent battle updates | 802,530 | 803,906 | 1,044,874 | 1,043,048 |

The quiet mean is effectively flat and near 30 emulated updates/s.
The sampled battle mean rises 0.17% and remains above the 30 fps
budget. A separate instruction trace sees one blocked-head attempt
on each of 120 quiet updates. It averages 10,784 cycles including
10,659 in the live supply scan, around 1.92% of the loop. Its paired
control matches all loop and orders timings and all five world and
display cuts. A pass-local reuse of the scan would help only when
several heads are waiting. Capture that case before changing the
order in which producers claim newly available supply.

- Matched timing summary SHA-256 `0733fc78186070260478b440affc812a809758517c07f3742d364e3000406b9f`.
- Blocked-head instruction trace SHA-256 `cd683ca46654171dda894491f46639b8a5253cef1592bfd3ffe37c90d453c681`.
