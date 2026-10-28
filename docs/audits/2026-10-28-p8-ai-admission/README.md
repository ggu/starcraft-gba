# P8 AI request-admission gate

This source-only step implements the bounded admission predicate before the
native P8 request heap. It does not wire P8 into the game loop, mission menu,
world adapter or saves. It does not claim that native counts represent only
completed actors, or that the complete P8 AI is implemented.

## Native evidence

The local StarCraft 64 USA reference ROM is identified by SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b` (SHA-256
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`). Static
evidence comes from the private translated native handlers; no ROM bytes or
native code are included in this repository. The `funcs_27.c` reference file
SHA-256 is
`5a5c0f12993836e0e81ff44cfbf8513440e5e020774463faf715fc038aa71a95`.

- `8004AB20`, source entry `funcs_27.c:728`, calls `8004CDEC(player, rawUnit)`
  for categories 1, 2, 4 and 8. At `0x8004AB8C`, any nonzero gate result
  returns without inserting; zero continues into the heap insertion.
- `8004CDEC`, source entry `funcs_29.c:417`, reads one byte from
  `0x800F0E70 + player * 0x4E0 + 0x1F8 + rawUnit`. The result is a block flag:
  cap 0 returns zero, cap `0xFF` returns one, and other caps return one when
  the count is greater than or equal to the cap.
- `8001666C`, source entry `funcs_6.c:3011`, reads the per-player/type count
  halfword at `0x800B1D44 + player * 2 + rawUnit * 24 + 0x1F5C`. For raw type
  5, `8004CDEC` adds the count for raw type 30 before comparing.

The private differential harness compiled the exact local C bodies for
`8004AB20`, `8004CDEC` and `8001666C`, then compared the helper with the native
gate across cap zero, `0xFF`, finite below/at/above cases, raw type 5's extra
type-30 count and the widened `65535 + 1` sum. It also called the native
`8004AB20` insertion handler to show that nonzero blocks insertion, zero
admits it, and category 3 bypasses this gate. The harness and native code stay
in ignored local build output.

## Authored service

`starcraft/sc64_p8_ai_admission.h` accepts a caller-selected player's cap and
count rows, so it does not need a player-indexed runtime binding. It bounds
both rows and the raw-unit selector. Malformed rows, selectors outside the
provided extent, or a missing type-30 count for raw type 5 return `ERROR`;
callers must treat every nonzero result as blocked. Raw type 5's two `uint16_t`
counts are summed as `uint32_t`, matching the native 32-bit compare and
avoiding 16-bit wrap.

Run `make test-sc64-p8-ai-admission` for source-only synthetic boundary cases.
The fixture checks special caps, finite threshold edges, type-5 aggregation,
sum overflow resistance and fail-closed selector errors. It has no mission
assets or game-runtime dependency.

## Remaining P8 work

This only answers the queue's admission gate. The meaning of native census
counts for incomplete production, production completion and failure
retention, resource identities/debit/refund, AI pass cadence, save encoding,
and P8 world-loop wiring remain separate work. The gate must not be used as
evidence for those policies.
