# P10 wide point routing and nearest depot selection

Two separate builds of the normal P10 cartridge were compared with the
exact November 13 KODAK build using the same physical HOME tape. Each
candidate changes one flag. Both wins match all 20 checkpoints, ten
briefing pages, 14 authored dialogue acknowledgments and the terminal
mission state at frame 46,057 after 47,194 calls.

| Build | Mean guest cycles per loop | Emulated loops/s | Change from control |
| --- | ---: | ---: | ---: |
| KODAK control | 1,907,674 | 8.79 | — |
| Exact wide point FIFO | 1,904,767 | 8.81 | −0.15% |
| Nearest legal depot first | 1,868,620 | 8.98 | −2.05% |

The [P10 profile](2026-11-13-p10-command-census.md) attributes the worst
ordinary stalls to depot facility fills. The wide point FIFO does not
replace those fills. Applying Z10's nearest-first selection policy to P10
shortens both separately traced early depot stalls:

| Call | KODAK control | Nearest first |
| --- | ---: | ---: |
| 9,674 | 42,470,903 | 2,628,100 |
| 11,045 | 45,253,308 | 2,294,920 |

In a paired 11,300-call prefix, all 21 state and video observations
match except the route-cache fingerprint and its build/hit counters.
The selection policy deliberately changes which routes are requested.
The matching win proves this tape, not cache equivalence or another
campaign or custom game. Keep P10's default unchanged until broader
coverage; qualify the byte-exact facility fill separately. The result
autosave is a separate stall and remains about 76.5M cycles in each run.

Both links passed their existing RAM guards and ROM header checks. The
nearest-first candidate retains the control's IWRAM and BSS boundaries.
The wide comparison uses the normal release vector plus
`SC64_ROUTE_LINEAR_FLOOD_PROBE`. Nearest-first uses the same release
vector plus `SC64_DEPOT_FIRST_NEAREST_PROBE`. Both options were measured
with the command census and packed mineral options off.

- Control ROM: `7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310`.
- Wide point FIFO ROM: `fff97c485f48b5ad5e529eee422559e361656610dbd2da97ba90f05c659dcbab`.
- Nearest-first ROM: `91cdf370c585d4a376fbd701bdf57d46e82092c7ff6c78d36c4f70814569bd9f`.
- Full receipts (control / wide / nearest): `5f6456ec24ab2c81df2bd0c97d8237e1a8f5c523aa971b93d462ef9b2b7e1660` /
  `12b9c98a9e95b4bcf1a5aa8d16b4f1dd3212c08515bd32de195ad75b33dcdcfa` /
  `91ec25c2d7ea3c4f1ed9d9ce0c312817a10c39fb854006772a1b154c2112bcb6`.
