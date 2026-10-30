# Gated Scarab Unit adapter

This source-only adapter is not included by `game.c`, is not wired into UI or
orders and does not enable Reaver admission. It maps the standalone lifecycle
and field packers onto actual `Unit` slots in a host fixture. The identity gate
is `SC64_SCARAB_RUNTIME`; normal products retain their previous flags.

For a freshly initialized Reaver, the adapter records the bounded queue and
active slot in the existing proposed field format. It takes owner-bank payment
at enqueue, allocates an actual first-free unit only when the paid front entry
starts, retries if the pool is full, and marks the allocated actor training or
inside on explicit source ticks. A training Scarab starts at integer 2 HP with
20 maximum HP, 10 shields, armor 0 and invincibility. Undamaged source Q8 HP
is reconstructed from progress and stored as integer floor; externally mutated
training HP is rejected by the prototype rather than silently overwritten. The fixture checks
real actor-slot use, owner and raw-owner checks, capacity, bank-limit rejection,
individual and whole-queue cancellation, lost-child retry and STOP signaling.
The adapter's 60-tick cooldown step is explicit; it does not infer GBA-frame
cadence. Its launch operation only changes child state: positioning, collision,
movement, impact, actual self-destruction and source-clock integration remain
unimplemented.

A checked parent loss retires attached training/inside children without a
refund and changes outside/self-destruct children to detached parent 255 while
preserving owner, raw owner, target and flight timers. This follows the pinned
OpenBW inside-versus-outside distinction; the real death path must eventually
perform the proper cleanup for every engine reference. Parent-loss and child-
loss hooks must run before slot reuse. Slot and owner alone cannot detect every
same-owner slot generation reuse, so these checks are not a save validator.

No generic world-presence, selection, hit-testing, collision, target eligibility
or actor-update loop is changed here. Until those hooks are implemented, an
inside/training state is only a field value; the existing engine does not yet
hide or protect such a child as a complete gameplay feature. Likewise STOP
signaling does not itself destroy a launched child, and generic STOP still
requires a Reaver-specific field-preservation integration. Save serialization,
legacy migration, duplicate/cross-owner/generation validation, source-accurate
transfer, and failure atomicity across all real death/resource hooks remain
required before these fields are used in persisted product worlds. Neither
`carry` nor `work_timer` is generally free: this prototype is scoped to explicit
initialized Reaver/Scarab identities under the gate.

The current allocator relies on the existing synchronous `find_unit_slot` /
`spawn_unit` sequence choosing the same first-free slot, and on lifecycle
outputs being representable by the checked parent packer. Future spawn hooks
that allocate or retire actors during that sequence must revisit callback
transactionality; the helper cannot roll back arbitrary external side effects.

## Local fixture

`tests/sc64_scarab_unit_adapter_game.c` includes the real `game.c` and this
adapter. Four local ASan/UBSan fixture builds used the actual Terran, Zerg, P7
and Showcase profile compiler vectors plus the explicit Scarab gate; P7 used an
isolated source-only fact table generated with the same feature fingerprint.
No imported bank was generated or modified. The test directly constructs
controlled worlds and calls adapter operations: it is not an ordinary-menu
replay, full save/codec test, gameplay claim or ARM performance result.
