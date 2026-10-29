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
make test-sc64-p8-request-settlement
```

## Caller-selected settlement service

The authored `sc64_p8_request_settlement.h` models three explicit caller
outcomes: RETAIN, POP_BASE, and POP_EXTENDED. It does not infer an outcome from
success, failure, or production state. Native code demonstrates more than one
path: `0x8004B820` selects extended accounting then pops; `0x8004B850` selects
base accounting then pops; at `0x8004FA18`, one branch returns without popping
while another calls `0x8004B850`. The meaning of every branch predicate is not
fully established, so these observations do not support a universal
failure-retains/completion-pops rule.

In `funcs_30.c`, `0x8004FA18` calls `0x8004CDEC` at `0x8004FAA0`. A nonzero
return reaches `0x8004FAB0` (`B850`) and exits; zero reaches `0x8004FAC0`, calls
`0x80124D1C`, and a zero result exits at `0x8004FAFC` without popping. A
nonzero result continues through the `0x26`/`0x2A` paths and calls `0x8004B820`
at `0x8004FAF4`. This establishes distinct transitions, not the gameplay
meaning of either predicate. Private source hashes: `funcs_27.c`
`5a5c0f12993836e0e81ff44cfbf8513440e5e020774463faf715fc038aa71a95`,
`funcs_29.c`
`73d1695f81f8513cbed168807cb7b0931e9db8ed43f02b263e088a75a886aecb`, and
`funcs_30.c`
`0cf263484f368bd83ba060c42c782620fbe27f201e7540f846f35301144995c0`.

For either pop wrapper, `0x8004AE7C` reads the head raw unit at request offset
`0x2EA`, obtains costs through `0x8004AAA8`, and subtracts from six words in a
per-player AI record. The record base is `0x800F0E70 + player * 0x4E0`; its six
words are at offsets `0,4,8,0xC,0x10,0x14`. The first three subtract with
saturation. The extended wrapper also subtracts the same values from the last
three words with 32-bit wraparound. Raw ids 44, 103, 62, 68 and 63 skip this
accounting but still pop. The role of these six words relative to player
resource banks is unknown.

The pure service takes the caller's outcome and explicit cost triplet. It
requires queue and accounting objects for every call, requires costs only for
a nonskipped pop, and validates before mutation. Its tests use synthetic
requests; a separate ignored differential harness compares it to the local
translated native queue/wrapper routines with a stubbed cost helper. This is a
settlement primitive only. Script command meaning, eligibility, actor census,
actual cost production, caller predicates, AI cadence, save behavior and
gameplay integration remain unresolved. The native pass-entry call
`0x8004CD30(player, 1)` sets a state flag; it does not reset this heap. No
campaign success, GBA memory budget or frame-rate qualification is claimed.

The private differential used the local `funcs_27.c` implementations of
insert `0x8004AB20`, pop `0x8004ACA0`, accounting
`0x8004AE7C`, and wrappers `0x8004B820`/`0x8004B850`. A test stub supplied
explicit costs at the native helper's output pointers. Over 50,000 seeded
interleaved operations, the test compared every live heap entry, count, all
six AI-record words, and surrounding canaries after each operation. Before
each pop, both sides received the same six-word ledger sampled from
`0`, `1`, `INT32_MAX`, `0x80000000`, `UINT32_MAX`, `255`, and `256`; costs used
the same word boundaries and byte samples `0`, `1`, `127`, `128`, and `255`.
The run observed 9,746 RETAIN outcomes, 19,736 pops (12,331 skipped-accounting
raws), 7,405 cost-helper calls, 4,218 pops with at least one saturating
underflow, and 2,173 extended pops with at least one wrapping underflow. The
first harness attempt used signed 32-bit
register fields; its comparison result was invalid for the translated MIPS
register operations. That failed attempt is preserved under the ignored
`failed-int32-comparison` directory. The passing run uses 64-bit register
fields and the selected native handler bodies verbatim (the extracted bodies
match their source-byte sequence). UBSan's shift check was disabled only for
this private native translation because its `lui` translation uses signed
left-shift expressions; the authored service fixture ran with full address
and undefined-behavior sanitizers. No translated native code or game data is
tracked.

The native request-admission block flag is now implemented as a caller-owned
pure service in [the P8 AI admission audit](../2026-10-28-p8-ai-admission/README.md).
It reproduces only the proven cap/count predicate; it does not satisfy the
remaining production, save or world-loop integration work above.
