# Wide facility-field consumer census

On the combined armed-table and persistent-Command-cache P10 cartridge,
a separate observational build logged point wrapper misses, full point
fills and facility rebuilds throughout the ordinary saved tail, calls
37,705–47,194. The instrumented build matches the uninstrumented control
at 68 sampled world, both-page, palette and fog cuts, all 9,489 frame,
screen and route-counter rows, victory and SRAM. The event buffers were
read every call and the observer checked that no 128-entry ring overflowed.
Instrumentation changes timing; these are counts, not speed measurements.

There were 278 facility rebuilds across seven depot keys. Of these, 166
selected an existing same-key partial field because it could not answer
the requested origin; all 166 replaced that same field. For 161 of those,
the preceding recorded request for that key was in the tape and had a
different origin; the other five had no preceding recorded request in the
tape. No reentry repeated the immediately preceding origin for its key.
There were 44 distinct key/origin pairs among the 166 partial reentries;
122 occurrences repeat a pair seen earlier, but intervening requests or
changes mean this is not a count of provably redundant identical work.
The 278 rebuilds occurred on 110 updates, with up to 14 rebuilds and seven
keys in one update. By contrast, there were 41 point-miss wrapper entries,
no parked-field hits or complete point victims, and 12 full point fills.
Ten of the latter came from prefix fill and two from certificate completion.

A hypothetical same-update least-recently-used park with one, two, four or
seven keys would encounter 53, 69, 72 or 76 of the 166 partial reentries,
respectively, assuming every preceding frontier was parkable, still bound
to the same field and geometry, and not invalidated. These are upper bounds
on reuse events, not saved fills or cycles. The existing 64×64 facility
frontier already keeps pending FIFO order and remaining walkability, but
its lifetime is one update and its shape and queue-scratch layout are
restricted to 64×64. The P10 cartridge uses 128×128 here. The wide point
tail retention occupies the upper queue scratch; the wide padded linear
flood, its queue and the retained walkability grid leave no demonstrated
non-overlapping area for a wide persistent frontier.

A first bounded experiment can use one update-local, statically allocated
wide frontier, retaining the same field pointer, key, geometry, walkability
and FIFO pending order. A 128×128 visited bitset costs 2,048 bytes, a
128-entry pending queue costs 256 bytes, and metadata adds about 24 bytes;
four such records cost about 9.1 KiB. The combined P10 static EWRAM guard
has 11,984 bytes of headroom, so even four records leave only roughly 2.6
KiB and require actual link and runtime qualification. Capacity 128 is a
fallback boundary, not a proven maximum: on overflow or any mismatch,
forget the frontier and run the stock rebuild. Do not move the stop test:
the origin is checked after each complete FIFO pop, preserving the same
seed order, neighbor order, packed directions and tie results. Reset all
records before or after `update_orders`, on topology/layout changes and
before shared scratch or save operations if any scratch is borrowed.

Start with `tests/sc64_facility_fifo.c` and the existing facility-frontier
fixtures, extending them to wide geometry with two alternating origins,
interleaved depot keys, field eviction, topology changes, blocked and
unreachable origins, and forced queue-cap overflow; compare every packed
field and next step to a fresh stock fill. Then use this exact P10 tail,
including per-call route state and both pages, before claiming savings.
The 76-event upper bound is per-update only; retaining a frontier across
updates would need explicit ownership and invalidation beyond this design.

Local instrumentation patch and scripts are in
`/Volumes/Storage/sc64-route-next-20261122/build/route-research`. The full
result SHA-256 is
`46635e64af61dbb691a4caf1abf5eb53fc778fa3475ce6fc2279261b43f6b76a`;
the instrumented ROM SHA-256 is
`8aa6b4299ad1aaf0c8c84516300496fbde26cd3e7db54efa5c464f90569354b2`.
