# P8 production-request heap

`starcraft/sc64_p8_request_queue.h` provides bounded storage for the native
AI production-request heap. It is not yet bound to a mission or included by
any product. P8 remains unavailable in the campaign menu.

The caller owns each queue. Entries contain an eight-bit priority, eight-bit
category, sixteen-bit raw unit identifier and opaque thirty-two-bit payload.
The payload is a value or handle, not a host pointer. The queue neither
allocates actors nor pays resources. Its 63-entry limit is separate from the
native AI object's 100 script-request records.

Native insert (`0x8004AB20`) and pop (`0x8004ACA0`) establish these rules:

- Higher priority wins. Equal entries do not bubble upward on insertion;
  removal chooses the right child when child priorities tie. Ordering is
  not FIFO and is not stable.
- A full insertion replaces physical leaf 62 and sifts upward without
  increasing the count. It can discard a request other than the lowest
  priority. Rejecting a full insertion would change source behavior.
- Categories 1, 2, 4 and 8 consult an external eligibility gate before any
  insertion. This API receives that decision explicitly; it does not
  substitute a generic predicate for the native game-state query.
- Native callers must not pop an empty heap. The public wrapper instead
  reports EMPTY without mutation at that boundary. Invalid counts also
  fail without mutation.

These conclusions were checked against translated handlers from the local
N64 reference whose ROM SHA-1 is
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. The relevant `funcs_27.c`
SHA-256 was
`5a5c0f12993836e0e81ff44cfbf8513440e5e020774463faf715fc038aa71a95`.
Native code, game data and execution captures remain private. The committed
fixture uses only authored synthetic requests and asserts the native-observed
six-entry and saturated low/high replacement vectors, heap invariants,
opaque field preservation and defensive boundaries. A further private
differential run compared counts and all live entry fields after 100,000
interleaved synthetic insert/pop operations, including ties, eligibility skips
and saturated replacements, with no differences after correcting the category
gate to include 1 and 2. The deterministic seed was `0x6d2b79f5`. Empty native
pops were excluded while the wrapper's defensive behavior was checked separately.

Run the asset-free sanitizer fixture with:

```sh
make test-sc64-p8-request-queue
```

Integration still needs the mission's actual script commands, eligibility
predicate, actor census, resource accounting, retry/completion transitions,
AI cadence and save encoding. A failed production attempt must retain its
request; a valid completion path may then pop it. The counter accessor and
heap are not evidence that a request has completed. The native pass-entry
call `0x8004CD30(player, 1)` sets a state flag; it does not reset this heap.
No campaign success, GBA memory budget or frame-rate qualification is claimed
by this inactive service.
