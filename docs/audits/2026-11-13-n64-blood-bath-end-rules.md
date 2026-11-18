# StarCraft 64 Blood Bath: defeat, victory and surviving units

Both observed single-player game types use the same end triggers: a player
loses when its **registered building count reaches zero**, and wins when
there are **zero eligible opposing player slots**. In these modes defeat
hands remaining actors to the neutral owner and detaches them from their
former player's AI. A town hall is not the sole building that counts, and
mobile units do not keep a defeated slot in the match.

This is the result of decoding the live trigger lists and tracing their
native handlers, using three authenticated opening RAM samples per mode.
The paired launches and their visual lobby evidence are in the
[Blood Bath opening audit](2026-11-12-n64-blood-bath-opening.md#paired-blood-bath-launches).
We did not drive either match to a result screen in these captures.

| Single-player mode | Observer manifest SHA-256 | Opening opponent counts derived from the native rule, CPU 0 / CPU 1 / CPU 2 / human 3 |
| --- | --- | --- |
| Melee | `7e245d3378b968b3144b6205d1353b4059792ab678f66db4270210b5e6569594` | 1 / 1 / 1 / 3 |
| Free for All | `4538e843914374adc280be510da10d8368bb045ba5553e1cc9aaf3295ce95260` | 3 / 3 / 3 / 3 |

## Live triggers and threshold

The owned USA ROM has normalized SHA-256
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
Its Blood Bath CHK (`008/04F`, SHA-256
`d94fe779016064030ed693351a64c8adbb5eb0c1f50aab50ddc6f7bb8f58201d`)
includes authored `Command(Any Unit)` records. These are not the live
single-player end conditions. The original `TRIG` bytes remain in RAM
alongside different live records, so importing the authored conditions
alone would yield the wrong rule.

At each of the six openings, every active owner had precisely these two
records in its live list at `0x800cd214 + 12 * owner`, in this order:

| Live condition | Threshold | Result action |
| --- | --- | --- |
| Opcode 14, selector 13 (current player) | Comparator 1 (at most), amount 0 | Opcode 1, Victory |
| Opcode 2 (Command), selector 13, unit group 231 (Buildings) | Comparator 1 (at most), amount 0 | Opcode 2, Defeat |

Both conditions call the unsigned at-most comparison at `0x800862f0`.
For raw group 231 with this comparison, `0x800880ac` reads the current
owner's broad building count at `0x8010ac80 + 4 * owner`. The counter
uses the building group bit `0x10` in the native trigger unit
classification table at `0x800cbf90`; `0x80086f90` updates it. The
captures classify Command Centers, Supply Depots, Barracks, Hatcheries,
Nexuses and Pylons as buildings; SCVs use the separate men group. Each
starting owner had one building in this counter. The threshold does not
use the separate building counter selected by the other comparison
path. The count path is established; no capture exercised an interrupted
construction, a flying building, or a morph when it was the last structure.

## What qualifies as an opponent

Opcode 14 enters `0x800865b0` and counts opponents via `0x800887c4`.
For current player `p`, the routine considers `p` and each eligible
owner `i` allied with `p` in both directions (relation byte 2). For
each such `i` it marks any other eligible owner `j` with whom `i`
does not share relation 2 in both directions. It counts each marked
owner once. The relation table is at `0x800b92a4`, with twelve entries
per row. The helper only considers competitive slots below 8 in the
observed branch; it filters player kinds, and the iterator
(`0x8007eb88` / `0x8007e9d4`, using `0x8008c8d0`) excludes active
kinds 1 and 2 with published terminal status 1 or 2. The defeat handler
also changes those player kinds to values outside the active iterator.
The condition never counts individual enemy actors.

The same condition yields different counts in the two modes because
the three CPU slots open reciprocally allied in Melee and hostile in
Free for All. In both, the human starts hostile to all three. The
counts in the table above apply this routine to the captured owner and
relation tables; no outcome was inferred from a screenshot.

## Defeat publication and cleanup

Action 2 (`0x80080690`) sets pending result 2 via `0x8008cd58`;
pending result 3 or 4 is preserved. The result pass beginning at
`0x8008d768` publishes the result at `0x800b936c + owner` if the
published byte is still zero. On result 2 it calls `0x8008cb44` and
then the defeat finalizer at `0x8008c930`. In both capture sets,
`0x800afefc` and `0x800b01d0` are zero and `0x800b01c8` is 1, so
the relevant branches change CPU kind 1 to 11 (human kind 2 to 10) and
then take the owner-wide actor transfer loop at `0x8008cb04`.
The cleanup does not branch on the Melee/Free for All selector.

The finalizer walks the defeated owner's actor chain, headed at
`0x80114660 + 4 * owner`, using each actor's next link at offset
`0x6c`. It saves the next link before calling `0x80137828` on the
current actor. The path through `0x80137578` and `0x801356c0` changes
the actor's owner byte (offset `0x4c`) to 11 and moves it to owner 11's
linked list. The transfer calls `0x80058d20` to detach the old AI
record and clears the actor AI pointer at offset `0x124`; the later
registration hook at `0x8007d2dc` returns immediately for owners 8 or
higher. For ordinary actors with actor-status bit 1 clear, it also sets
secondary order `0x17` and resets its order fields when the order changes.
This is the same initial order seen on neutral resources. Owner 11
already owns all 60 neutral mineral and geyser actors in the opening
captures, has relation 1 in both directions with every slot, and is
outside the victory iterator. The source path is a neutral transfer,
not a blanket kill of surviving actors.

## Port implication and remaining witness

The GBA [skirmish result check](../../starcraft/race_adapter.h) already
counts human buildings rather than bases or troops. It totals buildings
across all CPUs, though, so it never eliminates one CPU or detaches that
CPU's troops while another opponent still has a building. A parity
change should retire defeated owners individually, transfer surviving
actors to a noncompetitive neutral state, and derive victory from the
remaining opposing owners. Preserve that state across save and load;
campaign objectives remain mission-specific.

The next ordinary-input witness is a multi-CPU match where an opponent
loses its last building while it still has a mobile force and another
opponent survives. Observe the result byte, owner-chain transfer, and
subsequent orders and targetability before qualifying the GBA behavior.
The [linked survivor follow-up](2026-11-13-n64-blood-bath-linked-survivors.md)
traces loaded cargo and records a later sample without an elimination.
The [result cadence follow-up](2026-11-13-n64-blood-bath-result-cadence.md)
traces scan and publication order and specifies a direct-kill controller
witness. Also check last buildings in transitional states, simultaneous end
conditions, and whether other maps and modes install the same records.
No emulator session, native ROM, snapshot, or extracted content is
part of this source change.
