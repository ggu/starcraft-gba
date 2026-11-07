# N64 order-timer evidence and optimization limits

This read-only reference investigation uses the owned normalized USA N64 ROM,
SHA-256 `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`,
in a private ares v148 build. It publishes observations, not ROM bytes, extracted
assets, machine-code listings, screenshots, save states or game traces.

## Same-process interpreter observation

The finite observer records CPU stores in explicitly whitelisted physical RAM
ranges. It runs only with the interpreter, has a 65,536-event limit, performs
no guest reads inside the store hook and writes no guest memory. Its successful
terminal metadata and exact length were checked independently. The executable
SHA-256 is `e48d28303fe050d83f304eb2dc3d6a4dbf18db0b2060187bbf2f63f49c85b7fa`.

Two 8 MiB snapshots from the same owned process bracket the trace at VI fields
22,200 and 22,637. Their observer CPU/cache state checks pass; the full RAM image is not an
atomic snapshot of asynchronous graphics work. Each snapshot
shows all four watched records with raw type 7, owner 1 and HP 60 times 256,
consistent with the selected SCV layout previously tested through ordinary
movement. No watched owner/type CPU store appears within the trace. There are
five untraced VI fields before the first event and seven after the last;
bookend equality does not rule out all intervening identity changes.

The strict parser accepts 1,481 store records over VI 22,205–22,630. Trace
SHA-256: `5e2c81a08ddf1ba4035866f5d499027175f1eb5e2e9f1b7b22c7cc0d11fe3f03`.

- The global countdown has 127 decrement stores and one separate reload store.
- In every one of the 126 complete intervals between consecutive decrement
  stores, each watched actor timer has exactly one decrement store.
- Each actor has 15–16 reload-to-eight stores, plus separate zero resets.
- One linked-list stagger is observed. Offline code initializes its countdown
  with ten and reloads it with 150; the observed reload agrees with that code.
- Offline dispatcher structure has four early-return order IDs and 36 numeric
  pre-handler IDs matching the pinned PC reference. Those pre-handlers run
  before the later gate; they are not all orders that bypass the gate.

A parent reader independently reproduced the store counts, all 126 interval
checks, bookend field values and watched identity non-writes. Its first check
incorrectly expected 127 total countdown stores; distinguishing 127 decrement
stores from the additional reload corrected the reader, without changing the
observer or data.

## Clock labels and retracted candidates

The earlier Z8 sampler called one word a native clock or step counter. In a
separate normal-JIT pause intervention, it advances while actor positions and
timers stay fixed. The older text-box audit also observed that counter moving
while mission elapsed time stopped. Active-play correlations with AI waits
remain observations; this word is not a universally pause-gated world clock.

Another pause/speed-correlated candidate was found in RNG-like increment,
multiply and return code. It is not a validated simulation clock either.
The separately sampled elapsed-seconds word holds during the pause intervention
and advances after resuming. Two elapsed values alone still do not establish
the relation between game updates, VI fields, game-speed settings and wall time.

## Consequences for the GBA engine

The observed coupling supports investigation of order dispatch frequency.
It does not establish all orders, hidden units, DMA identity changes, normal-JIT
behavior, or exactly one countdown invocation per active world update. Earlier
actor identities must not be carried between different emulator processes.

Do not apply a uniform nine-update skip or skip an entire actor update. Movement,
cooldowns, exceptional resets, pre-handlers and order decisions must be separated
and compared before changing their timing. In the meantime, optimize measured
GBA hot paths with paired state/pixel and ordinary-gameplay checks.

The task-wide snapshot ledger reserves 192 MiB of its 256 MiB budget after this
run; 64 MiB remains. All game data and exact observer capsules stay private.
See the [living backlog](../../PROJECT-TODO.md) for current priorities.
