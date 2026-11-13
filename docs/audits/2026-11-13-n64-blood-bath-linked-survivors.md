# StarCraft 64 Blood Bath: loaded passengers and linked actors on defeat

The native [Blood Bath end rules](2026-11-13-n64-blood-bath-end-rules.md)
retire a CPU when its last registered building is gone. The ownership
path distinguishes a loaded passenger from an ordinary mobile survivor:
it reaches the same neutral owner write, but skips the ordinary
type-specific order dispatch. This is a code trace from the owned USA
ROM. The controller run below did not reach an elimination.

## What the controller run observed

A bounded repeat of the four-player Free for All launch used the same
Terran human and three named CPUs. The read-only ares observer sent
neutral input after Start, used the same in-process snapshot checkpoint
at step 4,200, and made no arbitrary guest-memory writes. Its step-4,201
cache-aware RAM sample was byte-identical to the authenticated paired
opening. The observer stopped at its 180-second wall limit after the
step-9,201 sample. The planned step-14,000 sample did not complete, so the
manifest marks the overall run incomplete; both retained RAM digests
match the manifest and the input hashes were unchanged.

| Observer step | Registered buildings, CPU 0 / CPU 1 / CPU 2 / human 3 | Published results, same order | Neutral chain |
| --- | --- | --- | --- |
| 4,201 | 1 / 1 / 1 / 1 | 0 / 0 / 0 / 0 | 60 mineral and geyser actors |
| 9,201 | 1 / 2 / 2 / 1 | 0 / 0 / 0 / 0 | Same 60 actors by resource type and count |

The plan SHA-256 is
`06a55a2629bc188568c5a18fa793e1db1287fcb075566a32f9b14adb939ddae0`;
the incomplete manifest SHA-256 is
`1563d420e32a90d18f86b946f7e67dfbcb742dbf7e91935930ae9e6d885f9d8d`.
The step-9,201 RAM SHA-256 is
`385915ecf8fe47fedfcff15af9363af796cb5461a3fcb4f09b4b678df5534f89`.
All three CPU slots remained active at both samples. There were no loaded
passengers in those samples.

## What the native code establishes

The same cache-aware RAM view from the two authenticated Blood Bath
openings contains the relevant code and lookup tables. The single-player
finalizer at `0x8008cb04` takes the defeated owner's chain and saves each
next pointer before calling `0x80137828`. That function passes owner 11
to `0x80137578`, which invokes the relink at `0x801356c0`. The owner
byte is written at `0x801357a4` and the actor is inserted into the
neutral chain by `0x8013581c`. There is no owner-wide check that defers
defeat when a linked actor survives.

- **Loaded passenger.** Boarding at `0x8014e40c` writes the passenger's
  transport pointer at actor offset `0x80`, its encoded ID into one of
  the transport's halfwords beginning at `0xac`, and sets passenger
  status bit 25 at `0xd8`. The normal unload path at `0x8014e744`
  calls `0x8014e5a8`, which clears those links and bits 25/26. The
  death path at `0x80136110` also calls separate cargo cleanup at
  `0x80135950`; this routine distinguishes a loaded actor from a
  transport and gives a Bunker a chance to eject passengers. The defeat
  wrapper takes the owner transfer instead. After the relink it tests
  bit 25 at `0x80137778` and skips the type-specific order dispatch
  for a loaded actor. It still requests secondary order `0x17` at
  `0x80137870` if status bit 1 is clear. The secondary order setter
  changes `0xa2` and clears its own fields on a change; it does not
  clear the passenger's `0x80` link or the transport's slot. The
  direct transfer path therefore supports retagging without using the
  ordinary unload or death paths.
- **Other linked state.** The relink follows actor offset `0x70` and
  recursively transfers a linked actor with native type flag `0x10`
  (`0x80135894`–`0x801358c8`). This is a different link from transport
  cargo. Before relinking, the native transfer explicitly clears the
  five-entry production queue through `0x80124fe4` for raw types
  72/82 (Carrier/Gantrithor) and 83/81 (Reaver/Warbringer). This trace
  identifies the queue reset; it does not establish the disposition of
  every existing Interceptor or Scarab.
- **Orders.** The forced `0x17` at actor offset `0xa2` is a
  secondary order. The transfer also dispatches a type-specific
  primary order for actors that pass its guards, including the loaded
  bit check. In both opening samples the neutral default order table
  at `0x800c91d8` has 2 for SCV/Drone/Probe, 50 for Carrier, and
  58 for Reaver. These are dispatch inputs, not observations of what
  the actors do on later simulation updates.

For the GBA prototype, this gives a bounded first case: retire a live
transport and its already loaded same-owner passengers together. Preserve
each passenger's transported order and slot reference so it stays out
of movement and collision; cancel pending board and unload orders.
Permit the transported order only for a validated neutral cargo graph
in marked Blood Bath saves. The existing cargo and save checks must
still validate both endpoints, capacity, and coincident positions.
Dropships and Shuttles are simpler first cases than Overlords, whose
transport capability is currently stored in the original owner's
research record. A Bunker is a building and is outside this live
transport survivor case.

An ordinary-input match must still exercise a CPU's defeat with another
opponent active, observe a linked pair across the transition, and check
later simulation and targeting. In particular, the code trace does not
qualify neutral Overlord cargo after the owner changes, surviving
fighter links, Egg progression, or in-flight spell behavior. No native
ROM, RAM, frame, or extracted code is included here.
