# Native mineral service queues

Two neutral-input continuations of the ordinary USA N64 Blood Bath Free for
All opening retraced mineral service in all four seats. Each restored the
same step-75 ares snapshot and used only observer hotkeys. ROM, starting
state, emulator and input shim hashes were checked before and after. The
two observers ended at native updates 171 and 211. Coordinates below are
native world pixels. Intervals are bounded by snapshots of game updates.

| Seat and patch | Waiting while occupied | Observed pickup and next service |
| --- | --- | --- |
| 3 Terran, patch 19 at (1984, 272) | SCV 64 works, SCV 63 waits. | In (128, 144], 64 takes eight ore, and 63 enters work with 69 timer ticks remaining at the later cut. |
| 1 Terran, patch 49 at (256, 1968) | SCV 74 works. The list runs from head 76 to tail 75. | In (115, 128], 74 takes eight ore. Tail 75 enters work with 67 ticks remaining. Head 76 still waits. |
| 0 Protoss, patch 53 at (1760, 1968) | Probe 81 works. The list runs from head 80 to tail 79. | In (115, 128], 81 takes eight ore. Tail 79 enters work with 67 ticks remaining. Head 80 still waits. |
| 2 Zerg, patch 14 at (352, 144) | Drones 70 and 71 approach together at step 75. In (86, 98], 71 starts work and 70 enters the wait list. | In (156, 170], 71 takes eight ore, and 70 enters work with 71 ticks remaining at the later cut. |

Each patch had 1,500 minerals during the initial wait and 1,492 by its
handoff cut. The outgoing worker held eight ore. The next worker had zero
cargo. Waiters remained in order 86, phase 2, with a zero work timer.
Workers in service were in order 87, phase 5. At update 115, the Terran
and Protoss work timers on patches 49 and 53 were both zero, but the
active claims and full reserves remained until the order poll. Ownership
therefore lasts through the pickup transition. Native work starts at
75 ticks at `0x80132f20`. The complete trip also includes the order
poll, travel and depot credit. [Blood Bath income comparison](2026-11-21-blood-bath-income-windows.md)

## Native service and depletion

The mineral actor has remaining ore at offset `0xcc`, active count at
`0xcf` and the head of its waiting list at `0xd0`. A worker has a work
flag at `0xca`, cargo at `0xcb`, the claimed resource at `0xcc`, and
previous and next wait links at `0xd0` and `0xd4`. The current order
target at `0x5c` can already point at the mineral before the worker
enters that list.

The stock routine at `0x80132324` takes a claim only when the patch's
active count is zero. The release routine at `0x801323b4` clears the
claim, updates the count, and tries to grant service to a waiter. The
selection at `0x80132088` walks forward from the stored head and retains
the last eligible worker through its scan. Both two-waiter samples prove
tail-first service on the stored newest-first list. Interrupted and
ineligible waiters need their own live cuts.

The mineral debit at `0x801332ac` has a separate final-reserve removal
path. Order 85 uses a remembered point at worker offsets `0xc0` and
`0xc2` when available before calling the mineral search at
`0x80148b60`. The scan starts with a 384-source-pixel bound, checks fog
and path-region constraints, and can run after the old target becomes
invalid. This is stock-code evidence. The retained ordinary samples
cover service on stocked patches. The closest retained live patch found
in the search had 20 ore in a defeat-suppressed Blood Bath state.
The final pickup, same-cluster target choice, route and first deposit
from a naturally depleted patch remain open in an ordinary session.

## Contested service later in Blood Bath

Twenty-five neutral snapshots from the retained ordinary
Blood Bath match cover native updates 19,488–19,613.
At update 19,553, mineral patch 11 holds 172 ore with
one active claim. Owner-2 SCV 120 is working in order 87,
phase 5. SCV 130 has the same resource claim, is waiting
in order 86, phase 2, has zero cargo, and is the head of
the wait list.

At update 19,558, the patch has 164 ore and the head is
empty. SCV 130 is now working in order 87, phase 5, and
SCV 120 has released the patch. The active count is one
at both endpoints. This brackets the queue removal,
reserve debit and handoff within five native updates.
The samples leave the intermediate active count and the
bank credit unresolved.

| Local receipt | SHA-256 |
| --- | --- |
| Sampled native records | `32e5ca1e79a78cc8b2d7a3373cf25908e7890b85105ec264478efdcd2c9cc7ae` |
| Retained waiting snapshot | `4ba6a4e9ba72574b7257cb1ec30edef8d6750cb3e81b0c5f8b592e373cedd3de` |
| Final native snapshot | `2b3ad6ca5c24a278687c198a105aa101649faf6cde5a5eee0368238f204e9116` |

## GBA follow-up

### Ordinary predecessor save with simultaneous service

Start an ordinary Custom Game on Blood Bath in the
normal Terran transport ROM. Double-tap A on an opening
SCV to select four workers, then order them to the
visible mineral patch 12. At game frame 125 two empty
SCVs target that patch in the same mining order, with
remaining timers of 31 and 44. A third is still arriving
and the fourth has cargo.

An ordinary pause save and a separate fresh HOME load
restore both work records at frame 125. After 100 neutral
updates the uninterrupted and resumed runs match all
recorded live-unit and building fields, mineral bytes
and player and race wallets. This is a predecessor
save fixture for maintaining concurrent legacy service
while introducing a new explicit policy.

| Local receipt | SHA-256 |
| --- | --- |
| Ordinary Terran ROM | `f2b98f3e36f03999743304366285a3f09b5b64b78dac6e71a9658b437c09d97a` |
| Contested patch and fresh HOME receipt | `b129dedbbae6cb558ca5c8160fae56dca2f4baeb0bcaa285c87d01da9627bf5e` |
| Legacy two-timer SRAM | `67663cce0b7c3e38c8a49bbfdb46e1a6cb79bbf4da9be8936b6a25fa6a552759` |
| Physical-button driver | `1c79baedf55e2078dbc8aab6aa71e714caf149fc9c807c9675d757c29e5c18e6` |

### Shared policy design

The shared GBA gather path starts an individual mineral wait at contact.
Multiple ready workers can start timers on a single patch. Its
`worker_continue_minerals` separately handles depleted destinations
using distance from the old field, visibility and reachability. Gate any
prototype on a fresh-match policy. Give one eligible worker service per
patch across owners, preserve the existing pickup and credit amounts,
and keep a waiter at zero timer without creating cargo. Distinguish a
waiter from a worker still approaching the same field. Keep gas behavior
and existing campaign timings in the comparison.

A current save can contain multiple positive timers on one patch.
Preserve the original continuation for those saves. Carry the policy
through a validated mode marker or a versioned record before activating
it for new matches. Keep the meaning of existing nonzero timers and the
pending explicit order. A transient occupancy scan can reconstruct one
active claim from a new-policy state when the timer and pickup finish
on the same update. Matching the native poll window also needs to retain
the claim at zero. Matching native FIFO across fresh loads requires
retaining queue order. Preserve that order
explicitly if the prototype promises it.

The optional patch-contention flag currently reconstructs
a claim from in-range nonzero timers each update, favoring
the lowest eligible slot when timers overlap. A waiting
worker may seek a free alternative every 16 updates.
The flag has no persisted mode marker or wait-list order.
The native service and the ordinary predecessor fixture
provide separate acceptance cases for the next policy.

Use two or more same-patch workers in each race. Check a save with one
worker in service and two waiters, then continue through the exact
handoff, deposit, and later pickup. Add cancellation and exhaustion cuts.
Use the predecessor save above to verify both legacy timers. Compare uninterrupted
and fresh HOME continuations on every normal product before activation.
Check the normal opening and AI economy timings separately.

### Queue order in existing-width save rows

Test a new-policy row discriminator before changing the save size.
An eligible queued worker has zero live cargo. Under an authenticated
new save tag, serialized values 9–248 can carry that worker's rank
1–240 on its patch. Decode and validate the ranks in a private
candidate, then restore the zero cargo value before the normal cargo
and mission checks. The work timer remains independent, including
when a worker has reached zero but still owns the service slot.

An asset-free fixture with synthetic actor facts passes all 240
values with timers 0, 1, 31, 44 and 45 in the compact, ZP,
derived and P7 typed codecs. All 4,800 row round trips reproduce
the codec inputs and normalization restores the original zero cargo.
The fixture exercises individual row widths. It follows the derived
codec's requirement for the omitted weapon field. Worker
classification and the complete journal still need integration tests.

Qualify the membership rules for empty workers, including approaches,
cancellation, job replacement and slot reuse. Reject mismatched
targets, duplicate ranks, gaps and inconsistent head state before
publishing the candidate. Give a malformed newer record the same
older-bank fallback as other journal failures. Keep the recorded
concurrent service when loading a legacy match. Derive the FIFO
only from a validated new-format record.

The local sanitizer fixture SHA-256 is
`42264e05567bbf0be489cc7c9be6d14b027c3cfb440abfc69849769a387448af`.

### Standalone FIFO and journal

The opt-in `SC64_MINERAL_FIFO` stage implements arrival-ordered mineral
service on the asset-free standalone engine. A patch advances only the
head worker's timer. Same-update arrivals enter in unit iteration order.
The next eligible worker starts service on the update after pickup.
Cancellation, movement away, depletion and slot reuse clear old queue
membership. The queue pass walks each patch list once and removes stale
entries as it passes them.

New matches written by the stage have a distinct journal tag. The
existing unit rows carry ranks only in scratch before writing. Load
validates and reconstructs the queues before committing, then restores
normal cargo values. An older save keeps its concurrent work timers
and continues to write the older tag. This stage keeps the existing
timer length, pickup amount, and deposit path.

Run `make test-sc64-mineral-fifo` for the standalone and Frontier
sanitizer cases. They cover arrival order, service handoff, cancellation,
depletion, slot reuse and a 50-update continuation. The journal checks
also cover fallback after a checksum-valid duplicate rank or a rank
on a gas order, and atomic rejection when neither bank can be loaded.

An authored ARM fixture writes a save with one active miner and two
waiters. A fresh mGBA process loads the actual 32 KiB SRAM image through
the engine loader, restores the order, and matches 50 uninterrupted
updates. The compared state includes units, buildings, mineral fields,
queue links, frame and wallets.

In the same ARM fixture, 32 orders passes with eight miners on
separate patches cost 129,707 cycles under the older policy and
255,419 cycles under the FIFO. Unit and mineral states match.
The added cost is about 3,929 cycles per update in that workload.
An earlier queue implementation took 261,923 cycles. This measures
the authored orders workload.

Integrate the distinct mode and rank normalization into the packed
race journals next. Validate the packed fallback and publication
paths, gas and depot transitions, payout and depletion, and shared
performance across the normal race and Showcase builds before
activating the mode there.

| Local receipt | SHA-256 |
| --- | --- |
| Authored ARM fixture | `f23ef06bdbbf093760bb786868d402633b49e057c4c32ad4d1abb1a830d51159` |
| ARM continuation and timing | `83c7e46bca413c4c131c82767b813bfe7d8beba5a3bb1cd4676cc228623abc48` |
| Standalone SRAM image | `bf05ad30a2b9f4054f9008f371a37ddfb62b185d51c1dc70fc851ef955d4181d` |

## Evidence

Only metadata and digests appear here. Native states and input observers
stay local.

| Item | SHA-256 |
| --- | --- |
| USA N64 ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| Ordinary step-75 snapshot | `a81f2410a1be7e199c63b46352a451ae9c93633481389a8c20c93c8c7c2cd988` |
| Step-75 normalized RDRAM | `54e8ca57b3fd01efca731b46d516eefc56cbcc139ed436995d20039121bde73e` |
| Terran and Protoss replay receipt | `02dc531bacec5fc24e0e963ed5235620c7226132f9c43f2aa8959c82e58abb7f` |
| Zerg replay receipt | `1c364e19f126e8cd8af6f397a7112d51891f85925aae73eebc9c509697865cf2` |
| Claim, ROM `0xf6014`, 144 bytes | `ae884dcd06442bb8a7ad26a8c281be30b2b615b620bd1607fa98830767cff12d` |
| Waiter selection, ROM `0xf5d78`, 164 bytes | `c6269aed0afac99fed4f508542dcdba37881dd9445c2c03e23ac9a7bc142d8e0` |
| Release, ROM `0xf60a4`, 200 bytes | `125a94312cdb5ed83e70670913fb54abecb2205385a019524c3e7ff5035c4aa8` |
| Work timer and pickup transition, ROM `0xf6c10`, 132 bytes | `638c3524acce1b96d95f4e620a1d7cadbe7337c52c98c04fa4802d57b180db39` |
| Debit and removal, ROM `0xf6f9c`, 172 bytes | `e02069bc316718839fdfc89586f1fcababc0d77830255a453b4ea6b81835eef1` |
| Mineral search entry, ROM `0x10c850`, 168 bytes | `2d6fa65d3273eede3ca3f57476e8ee31be3c684818e589c9242ea4f5f2becfc1` |
