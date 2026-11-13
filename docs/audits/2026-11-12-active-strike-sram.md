# Active Nuclear Strike SRAM probe

The opt-in `SC64_NUKE_ACTIVE_JOURNAL` extends the authenticated Silo
journal to a painted or descending strike. The ROM still has the same
16 KiB record limit. Launch reserves an actual Unit slot, and every normal
unit allocation and batch capacity check excludes it. The ordinary actor
world sees an empty slot; selection, targeting, sight and victory rules do
not accidentally treat the missile as a normal fighting unit. A full pool
refuses launch without consuming the armed missile. Abort or impact releases
that exact slot.

The writer encodes a dedicated raw 14 actor in the reserved row under the
existing authenticated `0x4b` tag. Its location fields store launch origin
and strike point, with existing fields for the current phase, source age,
and runtime track index. That track index preserves update order if two
impacts share a tick. A painting missile also stores a verified live
same-owner Ghost. Descent carries no Ghost reference. The reader checks
the checksum, tag, phase, age, ownership, geometry and canonical fields,
then removes the missile from the detached actor data before ordinary
save migrations. Publication restores the exact slot and track. There is
no journal tail. An out-of-range approach still refuses a save; the brief
visual effect after a resolved impact is transient.

`make test-sc64-nuke-strike-journal` runs fresh writer and reader processes
on Terran, Z10, P10, and Showcase with ASan and UBSan. Painting and
independent descent each resume at the expected tick and cause exactly one
impact. Killing a restored painting Ghost causes none. A checksum-valid
duplicate flight is rejected with the earlier valid save available. The
tests also cover full-slot launch refusal, capacity during the strike, and
exact slot reuse after abort. P10 research survives alongside an active
strike. The journal remains disabled in normal ROM targets.

`make test-sc64-nuke-journal-compat` also verifies the transition in both
directions: the active-strike reader restores older queued and armed Silo
records unchanged, and the Silo-only reader falls back to the previous
record when the newest holds a raw 14 strike.
