# Queued Interceptors on skirmish retirement

The USA N64 owner-transfer routine clears a Carrier-class production queue
before changing its owner. At `0x801376bc` it calls `0x80124fe4`, which
visits all five entries through the queue cancellation handler
`0x80124e50`. An unstarted raw-73 entry takes the refund branch at
`0x80124f20`. The resource helper at `0x80006948` adds that unit type's
price to the current owner's wallet. The live raw-73 table has a clear
low property bit and a price of 25 minerals and zero gas. A training
actor at the front uses a separate handler.

The ROM windows match normalized RAM from native Blood Bath step 9,053.
The owned USA ROM SHA-256 is
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
The state and RAM digests are
`fc037935a847bf9ba1c4e557658603b040104f34c0aa30fe04bfcf2a52e02e38` and
`93fe4eaa1d0d975507f71e713ea0aac21558f4e31b02fab86def90f6ce59ddcb`.
This capture authenticates the code and tables. A live native
Carrier queue at the moment of owner transfer remains an open witness.

| Native ROM window | Bytes | SHA-256 |
| --- | ---: | --- |
| Owner transfer at `0xfb344` | 124 | `4b94e65f39d98ce2e7c8686e3ae6f42d147141d7f1fa1a2102c3d212256a17f9` |
| Queue cancellation at `0xe8cd4` | 132 | `8085893a1455f66aeee42e321dc7d87cce9c878b32e696b322749f303d8335d7` |
| Refund branch at `0xe8be8` | 48 | `09f50e6a99497ca4190752d805960d00376c249cd4a8531a7ef9e450f3b42875` |
| Resource credit at `0x7548` | 96 | `2a29e364a965486ddaaa751c5e29771d94717c5198c97ae1ef2fa04de82807c8` |

The opt-in [elimination preview](2026-11-18-blood-bath-elimination-arm.md)
now accepts a valid Carrier-class queue with only unstarted orders and
fully docked children. It validates the whole departing roster and
hangar graph before refunding the original owner and clearing the
paid-order count. Docked actors keep their slots, parent and ranks.
Training, launched and detached children still defer the transfer.

The P10 sanitizer fixture pays for two pending orders, one on an empty
Carrier and one on a Carrier with two docked children. It checks that
bad cargo and duplicate hangar ranks leave the roster, paid queues and
wallet intact. The successful transfer refunds exactly 50 minerals to
the retired CPU, then preserves the neutral ownership, child links and
wallet through Pause save and fresh HOME load. The active training
fixture verifies that a blocked transfer also leaves a pending paid
order intact. The P10 receipt SHA-256 is
`ae8adc4b404de847d7273fc63e60334f3c72d78a111f31c5afbde26b70e199c2`.

The full expanded Z10 Carrier HOST fixture stops earlier, at the
detached-load preflight assertion in `check_elimination`. The same
recipe and assertion fail on the unchanged preview `96af35cd` before
the queue-specific tests. With the fixture entry point limited to
the three Carrier retirement cases, the same prepared Z10 flags pass
the wallet, malformed roster, active training and Pause/HOME checks.

| Z10 HOST receipt | SHA-256 |
| --- | --- |
| Unchanged preview, full fixture | `5d88435f9ac8625448c9d6094440b2e49ff5418a143232ed4c5a1777c0fdfa42` |
| Candidate, three Carrier cases | `8620dc0408f455f1595955ebac018442325d98a3ae64bfbfcb1236b018192692` |

The patch applies cleanly in a three-way source check with #667. The
refund uses the existing Interceptor price and the same preflight graph
validation that will enforce the composed per-type capacities. Verify
the merged profile flags, an eight-slot Gantrithor fixture and the
retirement-to-victory path before qualifying a normal build.
