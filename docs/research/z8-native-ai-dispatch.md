# Native AI interpreter: bounded static audit

This audit follows native code in the owned StarCraft 64 USA image, not the
PC opcode names alone. It describes static control flow, not an assertion of
complete N64 AI behavior or game-loop timing. No instruction bytes or private
program operands are reproduced here.

## Provenance and mapping

The archive member is a 33,554,432-byte big-endian z64 image, normalized SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. The pinned public
[NTSC-U symbol recipe](https://github.com/n0madic/StarCraft64-recomp/blob/425f409e02edacf897354cd0872c61ce67a9067a/recipe/ntsc-u/sc64.syms.toml)
at revision `425f409e02edacf897354cd0872c61ce67a9067a` maps the resident ROM
region starting at `0x1000` to `0x80000400`, size `0xD5640`. The ROM was
read directly from the supplied archive; the relevant resident code was
disassembled in memory. The independent decoded catalog and its leaf hash are
documented in `z8-native-ai-policy-programs.md`.

## Instance creation and dispatch

The action wrapper at `0x80081CC8` takes the script ID from action offset
`0x10`, obtains an owner and location, and calls `0x8005EAF0`; the sibling
`0x80081C90` passes a null location. `0x8005EAF0` calls lookup `0x8005BD50`
and instance initializer `0x8005BE18`. The latter stores the resolved code
offset at instance `+0x08`, a zero counter at `+0x0C`, owner at `+0x10`, a
location rectangle at `+0x14`, and its signed midpoint at `+0x1C/+0x20`.
A null location initializes the rectangle and midpoint to zero.

The loop at `0x8005EBD8` visits instances, decrements the word at `+0x0C`, and
when the result is `-1` either removes a flagged instance or calls
`0x8005C450`. The cadence at which this loop is invoked is not established.
The interpreter selects one of two program bases based on an instance flag,
fetches a byte at base plus `+0x08`, increments the offset, checks the opcode
is below 113, and dispatches through the table at `0x80002310`.

| Opcode | Native target | Static behavior |
| --- | --- | --- |
| 0 | `0x8005C830` | Reads a little-endian 16-bit value and replaces the instance offset with it; continues dispatch immediately. |
| 2 | `0x8005C4CC` | Reads a little-endian 16-bit value, advances the offset past it, stores the value in `+0x0C`, and returns. |
| 36 | `0x8005E994` | Calls `0x8005EDC4` to remove the instance and returns. Out-of-range opcodes also reach this path. |
| 38 | `0x8005D7D0` | Calls `0x80059F7C` with the owner and stored midpoint, then continues dispatch. |

For opcode 2, a stored value N reaches the interpreter on the N+1th invocation
of the instance loop, provided no other code modifies the counter or instance.
This is a count of observed loop invocations, not a measured frame or time
unit. The immediate loop for opcode 0 also means a script can remain in the
interpreter until a yielding or terminating path; no host-side instruction
budget is proven by this static path.

## Opcode 38 limits

`0x80059F7C` obtains an owner-associated list through `0x801355D8`, traverses
links at object `+0x6C`, skips objects when `0x80128DA0` returns nonzero, and
passes objects whose type halfword at `+0x64` equals raw 74 or raw 61, plus
the stored midpoint, to `0x8016376C`. The names, complete eligibility
predicate, order effect, and behavior of that last callee are not established
by this read. In particular this evidence does not justify a hero-only or
idle-only equivalence claim.

The pinned recipe's game overlay has ROM base `0xE38E0`, virtual base
`0x8011FBF0`, and size `0x49130`. Its virtual end is `0x80168D20`.
Therefore `0x8016376C` is inside that overlay, at ROM offset `0x12745C`;
the recipe also names this 148-byte function explicitly. The earlier claim
that it needed an unknown overlay mapping was an arithmetic error.

Further static reads narrow the behavior without supplying semantic names:

- `0x80128DA0` returns nonzero if the object word at `+0x0C` is zero, or if
  its byte at `+0x4D` is zero. The scan rejects these objects; it does not
  compare that byte to an idle-order constant.
- `0x8016376C` calls a helper if object word `+0x124` is nonzero, computes a
  map-derived index from the saved midpoint through `0x8004A370`, selects an
  owner-indexed table through the object's byte at `+0x4C`, and calls
  `0x80163C04` on a 52-byte entry with the object and a final argument of 1.
- On successful allocation, `0x80163C04` stores value 4, the object pointer
  and the table-entry pointer in the new record, then stores that record in
  object `+0x124`. It obtains a map-derived point through `0x8004A6C4` and
  passes the object, a numeric order argument and that point to
  `0x80163800`. That helper has additional state checks, order selection
  and coordinate processing before its downstream calls.

This is not proof of a direct attack-move to the exact script midpoint. The
role of the map table, record, numeric orders, and downstream eligibility
still requires semantic cross-checking and native execution evidence.

## Update-chain evidence

The mapped direct-call chain reaches the instance loop through
`0x80023920` to `0x80058CFC`, then `0x80058AE0`, then `0x8005EBD8`.
The first function calls the wrapper unconditionally after another update
call. Its callers at `0x80023B08` and `0x80023F14` have their own control and
timing loops, so this does not establish a fixed number of script updates
per rendered frame or per second. Dynamic cadence remains unqualified.

A next qualification should identify callers and cadence of `0x8005EBD8`,
resolve the opcode-38 table/record/order roles, and trace ordinary native execution
at instance creation, wait entry/resumption and opcode 38. Keep native trace
and extracted program inputs private. Resource requests, attack groups,
priority, and the remaining opcodes remain open.
