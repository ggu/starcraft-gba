# StarCraft 64 Blood Bath: when defeat takes effect

The single-player Blood Bath trigger loop evaluates all eligible owners
with live trigger lists before publishing results and transferring
survivors. Destruction of a last building changes the census; the
trigger loop uses that census on its next scan. A defeat published by a
scan does not remove that owner from another owner's Victory condition
in the same scan. This is a code trace backed by the retained
[Melee and Free for All openings](2026-11-13-n64-blood-bath-end-rules.md),
which did not reach a defeat.

## Countdown and result ordering

The game loop calls `0x8008daec` at `0x80023be4`. At `0x8008db2c`
the trigger dispatcher decrements the big-endian unsigned halfword at
`0x8010dbd2`. It scans when the decrement wraps from 0 to `0xffff`,
then reloads 30 at `0x8008db58`. With no intervening reset, there
are **31 executions of this decrement** between scans. The reset
helper at `0x8008ce00` writes 1 instead; it is called during trigger
initialization and by other paths. This count is in dispatcher calls
that reach the decrement, not video frames, observer steps, or an
unconditional time bound.

The dispatcher clears the eight pending result bytes at `0x8010dbe0`
when it starts the scan. It uses the active owner iterator
(`0x8007eb88` / `0x8007e9d4`) and calls `0x8008d240` on each owner
with a live list before calling the result pass at `0x8008dbe4`.
The opponent test uses the published result and kind, so another
owner's pending result 2 does not yet make it ineligible. The result
pass publishes at `0x8008d9a8` (`0x800b936c + owner`) and, for a new
result 2, calls the defeat finalizer at `0x8008d9b4`. This predicts
that a last-opponent kill can produce a defeat on one scan and a
Victory (pending and published result 3) on a later scan, even if
the loser has a lower slot number.
It does not settle simultaneous destruction, a player quitting, or
special result branches.

The eight retained cache-aware samples all contain identical bytes in
the dispatcher window `[0x8008daec, 0x8008dc04)` (SHA-256
`ee73099e07d30b2ca9e4768075d4121a94983e0df64bc8e2237a4d1cc6563b32`).
To reproduce the read, validate each 8 MiB RAM file against its
manifest, subtract `0x80000000` from each address, and use `ram.csv`
to match the observer steps. The halfword and the two eight-byte
result tables decode as follows:

| Retained set | Observer steps | Countdown, in the same order | Pending and published bytes |
| --- | --- | --- | --- |
| Authenticated Melee opening | 4,201 / 4,210 / 4,215 | 7 / 4 / 2 | All zero |
| Authenticated Free for All opening | 4,201 / 4,210 / 4,215 | 7 / 4 / 2 | All zero |
| [Later Free for All attempt](2026-11-13-n64-blood-bath-linked-survivors.md#what-the-controller-run-observed) | 4,201 / 9,201 | 7 / 2 | All zero |

The later attempt's overall manifest is incomplete. Its two verified
RAM files show no elimination. The readings show the live countdown
changing, but they do not bracket an underflow or measure time from a
building death to publication.

## Selection cleanup on transfer

The transfer path calls `0x80069564` at `0x801375b4`, before the
owner byte changes. The ordinary death path calls the same helper at
`0x801362a0`. The helper visits eight player rows; `0x80069484`
removes a matching actor pointer from the row at
`0x800c09c0 + 72 * player`, shifts subsequent pointers down, and
clears the tail. The helper also calls additional cleanup routines.
This explains why retaining an actor's address across defeat does not
imply retaining its selection list membership. It does not establish
whether the actor can be selected again, whether direct or automatic
targeting can acquire it afterward, or how a projectile already in
flight resolves.

## Ordinary controller witness to run

1. Reproduce the [paired Free for All launch](2026-11-12-n64-blood-bath-opening.md#paired-blood-bath-launches)
   with Terran human, CPU slots 0–2, fresh isolated saves, and the
   owned USA ROM. Keep the opening input through Start (step 3,695)
   and retain its lobby image and opening census. Use visual feedback
   to mine, train a small Marine force, and scout for an accessible
   CPU whose last registered building is grounded and whose workers
   or troops are still alive. Use a second CPU's nonzero building
   count as the control. Do not treat the retained idle extension as
   a combat tape.
2. Attack that building through the normal unit menu and cursor. Use
   ordinary direct weapon fire; avoid a spell, splash, or delayed
   projectile as the finishing hit. Before the final volley, record
   the victim's building count of 1, its building identity and health,
   and at least one living mobile actor outside the target footprint.
   Record an ordinary save before the attack if available. A fresh
   replay from the same lobby and a replay from the save are separate
   cases; compare their pre-attack states before treating them as
   interchangeable.
3. Log the countdown, counts, kinds, pending and published bytes, and
   survivor records with a bounded read-only trace from the final
   volley through the first two scans after the count becomes zero.
   To resolve same-scan order, record entry at the decrement, scan
   dispatch at `0x8008dbbc` with the current owner word at
   `0x8010dbd8`, publication at `0x8008d9a8`, and owner change at
   `0x801357a4`. Log only these events and the small fields needed
   for them; a full RAM dump each observer step is unnecessary.
   With sparse snapshots alone, report only the interval that the
   samples bound. For the survivor, record identity/type, owner,
   owner-chain membership, orders (`+0x4d` and `+0xa2`), AI pointer
   (`+0x124`), position and health. Validate both chain directions
   (`+0x68` and `+0x6c`) and cross-check the pre/post screen images.
4. Pass the partial elimination case only if the victim changes from
   kind 1 / published 0 to kind 11 / published 2 after the census
   reaches zero, the same living actor moves to owner 11 with its AI
   pointer cleared, and a CPU with a building stays kind 1 /
   published 0. Keep the human published result at 0 for this case.
   Check subsequent orders and try selecting and targeting the
   survivor with ordinary input; record each response separately.
   If no actor was selected before transfer, the run cannot verify
   removal from selection. For the separate last-opponent test,
   continue the match and verify that the scan which defeats the last
   CPU does not yet give the human Victory, then look for published
   result 3 on a later scan.
5. Freeze the actual controller plan, observer and ROM digests,
   observer configuration, and exact sample/event schedule. For the
   hybrid observer, retain the verified same-process transition and
   the cache-aware view. Run each short probe in a fresh directory
   with the existing disk, output and time guards; the idle wrapper's
   pinned input hash and expected sample schedule must be updated in
   an isolated copy for the new tape. Require a complete replay
   manifest and matching artifacts before calling either case
   witnessed. Retain the private capture locally and publish only
   the evidence hashes, observations and source notes.

This audit records no new emulator run and changes no game code.
