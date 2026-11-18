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
The initial head at full supply retains the previous refusal pending
its own native comparison. The opt-in flag stays off by default.

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
The observed N64 case remains two SCVs from one human Command Center.
Native empty-queue purchase at cap, capacity recovery, and other
classes of production require their own replay evidence.

- Z10 preview ROM SHA-256 `cf9ce65b47b52c9151f960bde49000758188b0d5c97b3858fcffe5b7575b1ee1`.
- Z10 controller receipt SHA-256 `553a7e13f2cb533d31e59f90ca108f7eef37c820775ceb188ebc6e52c4a1b13d`.
- Z10 rejected-tag and refund receipt SHA-256 `bb0c29066f0d50868d32e1ff159a095c5266445ebb3ed6c65dbf324c3b8dceaa`.
- Z10 selected waiting-bar receipt SHA-256 `0e0adbdb232da69eac0e52d975c0a2845e8f748a50ce3bbd73456624e93a9bb9`.
- P10 preview ROM SHA-256 `895d2420357ea4207b7309d9ebc0d0b7e91d725ea9fa7814da6f5fc856fc5f5a`.
- P10 research continuation receipt SHA-256 `d51695c0fc76b18d2c9c275705020cc15cca34f8d61f65807274e5a83fe8ffb8`.
