# Mineral continuation route queries

When a worker's mineral patch runs out, `worker_continue_minerals` first
checks the route to the nearest eligible replacement. Distance is measured
from the depleted patch, with ascending mineral slots breaking ties. If the
nearest patch is unreachable, the worker scans the remaining slots in the
original order and checks closer routes as it finds them. Eligibility still
uses the 192-pixel radius and the player's exploration. The change adds no
save fields and applies in the shared engine.

The sanitized mineral depletion fixture tests final cargo delivery and
continuation for all three races, pending orders, fog, range and unreachable
patches. The new detour cases add multiple reachable replacements and a
blocked minimum followed by equally distant alternatives. In the stock
P10 test the first case built three route fields. With the nearest patch
first it builds one. The blocked case builds two fields in both versions
and selects the lower of the two tied mineral slots.

## Verification

The normal Terran, Z10, P10, and Showcase cartridges pass the ROM verifier
and retain their control IWRAM and EWRAM endpoints. The mineral depletion
fixture passes in the Terran, Z7, P7, P10, and Showcase configurations.
The P10 route fixture also runs uninterrupted through mineral retargeting,
pickup, and deposit with a clear route and with a wall that forces a detour.

### Z8 normal replay

The control is the normal Z10 cartridge from `d464e1f6` (PR #605). The
comparison used identical private input files and the ordinary Z8 controller
tape with SHA-256
`b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb`.
Across 46,487 calls, all 47 gameplay checkpoints, 24 displayed-page, palette
and visibility samples, and 19 host checkpoints matched. Both runs won at
game frame 45,468 with zero faults. This was an uninterrupted run.

The call 22,253 hotspot occurred at game frame 21,667. On the earlier
`6720a2fb` Z10 cartridge, instruction sampling identified 16 route field
builds inside two calls to `worker_continue_minerals`. Those builds used
13,954,604 cycles. On the `d464e1f6` control and candidate cartridges, the
same call measured as follows.

| Metric | Control cycles | Candidate cycles | Change |
| --- | ---: | ---: | ---: |
| Call 22,253 loop | 15,352,990 | 1,861,434 | −87.9% |
| Call 22,253 orders | 14,672,677 | 1,187,512 | −91.9% |
| Calls 31,000–31,200 mean | 1,733,705 | 1,590,447 | −8.3% |
| Calls 31,000–31,200 p95 | 2,467,438 | 2,101,584 | −14.8% |
| Full run mean | 1,040,994 | 1,037,604 | −0.33% |

The original Z10 ROM SHA-256 is
`2fe3f4c8642d49fb61c20ca8e3db53876a551a738ff6149bf3cc7058f0f7a25c`.
The replayed candidate SHA-256 is
`d2633b10d0c890ec1a410d6899b35476f0aaa90787c2a0e4692c9274f034cdaa`.
The promoted shared implementation produces a byte-identical Z10 ROM.
The Z10 selector's stack frame grows by eight bytes. The linker's IWRAM
and EWRAM endpoints remain `0x03004d88` and `0x02037ef8`.
The route cache can warm in a different order. Future target-selection
changes should compare the full tape and sample the same hotspot.
