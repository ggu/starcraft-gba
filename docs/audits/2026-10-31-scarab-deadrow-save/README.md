# Experimental post-impact Scarab save

This default-OFF child of `b6c332d` addresses the observed post-combat save
failure caused by a dead Scarab retaining its outside-phase fields. The writer
first validates the dead actor's exact Scarab identity and flight fields with
the existing bridge, then zeroes only its local serialization temporary.
Neither the live Unit row nor any other world state is normalized. Existing
live outside and self-destruct children remain unsupported, as do pending
Scarab packets. The reader has not been relaxed: an encoded outside row,
including a dead one, is still rejected before candidate commit. A parent
still referencing a dead training child fails the existing graph validation.

The synthetic host fixture issues an ordinary paid Reaver attack against a
naturally durable building, waits for the child to die and its shared packet
to drain, and saves without reusing the dead slot. On all four profiles and
both maps, a fresh process loads and continues 50 updates. At save and after
continuation, separately written snapshots compare byte-for-byte for every live
Unit and Building row (dead rows are excluded from this comparison), player,
resource and mineral arrays, frame, RNG, map and geometry. The writer also
compares the complete live Unit and Building arrays before and after saving.
The reader exports its in-memory SRAM after load for an exact byte comparison.
These are scoped synthetic host scenarios, not a whole-world or hardware
continuity claim. The initial P7/Showcase test used an artificially enlarged
Marine maximum HP that their strict codecs reject; that failed setup and the
corrected naturally durable target are both retained in ignored receipts.

For all four profiles and both maps, a valid live outside child created by
normal paid training and launch is refused without changing the child, parent
or SRAM. Malformed dead identities and carry fields, a dead child still
referenced by its parent, and a pending tagged packet are refused with SRAM
unchanged. A checksum-valid new-tag dead-outside row is rejected by the
unchanged reader; an older valid hidden-child record wins, and after that
predecessor is removed the rejection leaves sampled actor/building state,
resources, frame, RNG and SRAM unchanged. Existing valid hidden rows are
used as the fallback. These checks do not prove universal load atomicity.

All four full-probe ARM links pass unchanged guards: Terran
`020370d0/03004e68`, Zerg `02037144/03004f68`, P7 `020382fc/03004c38`,
and Showcase `0203df50/03004ff0`, matching `b6c332d`; Showcase has 16 bytes
of IWRAM headroom. No new BSS is added. Source assets stay ignored.

The opt-in `make -j1 test-scarab-deadrow` runs the four prepared-input
fixtures and both maps; individual profile targets are available. Recipes do
not invoke private extraction or asset-generation prerequisites and generate
P7 roster facts only in a new ignored run folder. Outside-child and pending
packet persistence, general save compatibility and feature admission remain
open. No native N64 or hardware-performance claim follows from this stage.

The complete opt-in target passed for all four profiles and both maps: eight
paid attacks, sixteen compared writer/reader snapshots and eight negative
matrices. The first target run was interrupted after its negative fixture was
strengthened; the partial output is retained separately. The 104 frozen input
hashes matched after the tests. Detailed argv, run outputs and SRAM/snapshot
files remain in ignored worktree directories.
