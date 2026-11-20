# IWRAM queue for full route fields

`SC64_ROUTE_IWRAM_FIFO_PROBE` is an opt-in experiment on release `603755a7`.
Runtime `d0f1da61` keeps the dense direction grid, complete eager fills and
packed fields. It gives the existing neighbor loop a 512-entry stack ring in
IWRAM and puts batch control in IWRAM. The original EWRAM queue still holds
seeds and provides the overflow path. No persistent working RAM is added.

Each batch reads a contiguous part of the live FIFO, with room for up to four
enqueues per pop. Three temporary entries beyond the end let a batch append
across the physical wrap. Expansions visit left, right, up and down, and mark
each discovered cell immediately. A full ring copies only its pending entries
in dequeue order and resumes the same dense state in EWRAM. The original
failure and field publication rules still apply if that queue reaches its cap.

## Exactness and small ARM timings

`make test-sc64-route-iwram-fifo` passes with AddressSanitizer and UBSan. All
386,646 point queries and their cache state and build-count receipts match
with the probe off, at 512 entries, and with 16 entries to force overflow.
The two open wide grids wrap and match the independent point BFS. The facility
test also matches packed bytes, validity, cache and diagnostics, including the
wide full field used for blocked origins. A synthetic 256-seed frontier forces
the 512-entry ring to spill after 85 pops and produces the same complete
16,384-cell dense grid. The small-ring spill fixture resumes after two pops.

Seven small ARM cases match all packed bytes. The queue bank is in EWRAM for
the stock lane and the ring is on the IWRAM stack for the candidate. Each row
is the median of three fills, with VBlank IRQ noise gated during timing.

| Grid and goal | Released cycles | Ring cycles |
| --- | ---: | ---: |
| Retained P10, 9528 | 883,022 | 847,035 |
| Retained P10, 9530 | 882,895 | 847,301 |
| Retained P10, 9656 | 882,146 | 846,144 |
| Open 128×128, center | 1,070,836 | 1,022,664 |
| Segmented 96×128 | 797,440 | 764,327 |
| Open 96×128, edge | 805,066 | 772,240 |
| Artificial 256-seed overflow | 986,038 | 1,000,427 |

The three P10 fields each visit 12,836 cells. The initial split that placed
only the neighbor loop in IWRAM slowed those fields by 2.95–3.05%.
The final split saves 4.03–4.08% on them and costs 1.46% on the artificial
overflow. The microbench receipt SHA-256 is `979f9689b02b277adb12e2f66da6c26f5abc8e121059e5d65565d800b467331b`.

## Z8 on the normal Z10 cartridge

The original physical-input probe replays the same Z8 pause SRAM and inputs
from call 17,122 through 33,900, including three dialogue acknowledgments.
The released word-pack ROM SHA-256 is `970cd016b85715f89aca663de16ab5f56ccff2396d594132d9e42901a5f798c9`.
Its retained replay receipt is `d3d2093650788010fb4eac8fc687157e3bd407843c6cdabf32f900e270669f9a`.
Candidate `090434abd4b0c1b1876ae4c36f76740664541f36e54161f5ca7a83e5ed31643a`
uses the same 40 frozen asset inputs and adds only the route probe to the
runtime recipe. The 17 complete checkpoints match, including 200 resident
packed field hashes, world state, palette and both video pages. Frame and
route counts agree on all 203 timed updates.

| Emulated cycles | Released | Ring |
| --- | ---: | ---: |
| Peak update 33,800, game frame 32,984 | 12,092,659 | 11,683,277 |
| Orders on that update | 11,113,960 | 10,700,830 |
| Mean of 52 updates with new logical route builds | 2,780,392 | 2,721,800 |
| 95th percentile of those 52 updates | 8,503,712 | 8,208,551 |
| Mean of all 203 timed updates | 1,678,389 | 1,663,508 |
| 95th percentile of all 203 timed updates | 3,276,408 | 3,199,600 |

The peak uses 15 new logical route builds. The maximum paired increase among
the sampled updates is 10,913 cycles at call 26,356. The physical-input
comparison receipt is `db6370709b6c065d4a3feab30be5364414bfb3cfd2b97d1a888a3f177dc24979`.

## Memory and rollout

The candidate normal Z10 link passes the GBA header, payload and RAM guards.
IWRAM ends at `0x03004dc8` with 568 bytes before the `0x03005000` guard. It
uses 64 bytes more than the released Z10 image. EWRAM ends unchanged at
`0x02037f78`.

The normal Showcase recipe for released ROM `9c2e62a0d7240a72b4414709d1a7709ec594ab5d9567e017d1ba96afa4db668d`
also links with the single probe flag and its 33 frozen inputs. The GBA
header and ELF payload match, and both queue kernels are in IWRAM with the
overflow driver in ROM. IWRAM ends at `0x03004ff0`, 56 bytes beyond the
released build and **16 bytes** before the guard. EWRAM ends unchanged at
`0x02039068`. The Showcase link receipt SHA-256 is `a07353e8b155bfa686db43fb077acd9f3e1fb3182acc5cf3cd6e015979e3d15f`.

Compiler stack records give a 1,148-byte worst route suffix including the
1,056-byte ring wrapper, 56-byte ring controller and 36-byte batch loop.
The EWRAM continuation goes through a 48-byte driver in place of the ring
controller. The point and depot fill callers add 88 and 96 bytes respectively.
The miniature ARM stack canary peaks at 1,380 bytes including its callers.
In the normal replay, read-only memory inspection sees system stack writes
as deep as 4,556 bytes, with the lowest 256 guard bytes unchanged. That is
an observation of writes and does not bound every normal stack path.

The banked IRQ stack is independent of the route ring. It runs from
`0x03007fa0` down toward the system stack starting at `0x03007e00`. The
sampled-audio path accounts for 296 of the 416 bytes, including BIOS saves,
the IRQ callback, mixer frame, decoder, and Thumb switch helper. The normal
Z8 replay observes the same 296-byte depth, leaving 120 bytes between the
observed IRQ depth and the system-stack top.

Keep the probe opt-in. The artificial spill regression and 16-byte Showcase
link margin still matter. Showcase stack and timing and the other race ROM
checks remain open.
The Z8 replay and small P10 fills do not establish physical hardware frame
rate or qualify the default.
