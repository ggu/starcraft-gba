# Cross-race Spider Mine geometry — 2026-10-30

## Failure and correction

The native weapon fixture exposed a mine that could select a Zealot but could
not damage it. The original T4/T5 geometry helpers cover their mission rosters;
neither resolved the stock cross-race victim. The same assertion failed on the
preceding shared runtime and original weapon bank, independently of the later
profile-bounds renderer optimization.

Runtime source `63a31afef46ac52e6612ff8f4489b7f677d78076` applies the isolated
`8e4d708` fix to shared parent `9ea7b505e08cc121662b73b76e00786f1450983c`.
It preserves the audited T4 lookup first, then uses the existing pinned stock
unit dimensions for raw IDs 0–227. Inclusive right/bottom dimensions become
half-open query edges by adding one. Unknown IDs retain the old T5 fallback.
All 228 rows were independently compared with the pinned BWAPI UnitType source;
there were no mismatches.

The change affects rectangle resolution for contact and splash. It does not
change owner, flying, hover, burrow, invincibility or targeting policy, mine
timers, damage constants, save fields, pool sizes or reserves. Queries use the
actors' current positions. Splash still visits the existing Unit pool; this
does not implement a separate building-splash policy.

## Qualification on the combined runtime

- A clean indexed checkout without private inputs passes source boundary,
  `./dev doctor`, `./dev build` and `./dev test`, with explicit exit-zero
  receipts. Tests report 717 Python cases (23 optional skips), a separate
  eight-case suite and the C fixtures. The default ROM SHA-256 remains
  `6e5556d7fbbab348ab54bdc34ea8a228077b2981817f27100a1dd9f9f90aea0d`.
- `make test-sc64-t5-mines` passes under ASan/UBSan, including all stock-row
  mappings outside T4, exact edge distances and real adapter contact/splash
  against a Zealot with the original full-packet assertion.
- All four canonical native-weapon fixtures pass using their actual ordered
  profile flags and the prepared v6 bank. All check 66 source profiles, 61
  projectile-art profiles, eight headings and full tracer lifetimes. Only the
  actual Terran vector defines `SC64_T5_RUNTIME`; the original mine/Zealot
  damage assertion runs and passes there. The other products pass their
  applicable cases, not a forced-T5 variant.
- Unified build/test and the full ten-mission Terran host aggregate pass.
  These are host controller and save checks, not new ordinary-menu ARM or
  physical-hardware completion runs.
- Four normal Make ROM builds pass existing guards, header checks and
  independent ELF-payload, FF-padding and embedded-audio checks. No private
  generators run. All 104 captured input hashes remain unchanged; the v6
  weapon header SHA-256 is
  `0e03fcc54e860ab7d7a9904bcacdd47077045754294193ea7759dc9c949d9845`.
- Zerg, Protoss and Showcase ROMs are byte-identical to the qualified #185
  products. Terran's unpadded payload grows by 96 bytes. An independent symbol
  comparison confirms identical BSS/IWRAM ends for every product:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `020370b8` | `03004ec0` |
| Zerg | `02037124` | `03004fd0` |
| Protoss P7 | `020382e4` | `03004c48` |
| Showcase | `0203df38` | `03004fc8` |

Private command, source/input hash, executable and log receipts are retained in
the integration worktree under `.local/build/mine-arm-v1/` and
`.local/build/mine-host-v1/`; clean default receipts are in the repository-local
`.local/mine-default-qualification-v1/`. No game graphics, generated banks,
ROMs, saves or private traces are included in this source audit.

## Scope

The existing mine service is still guarded by the Terran-specific T5 runtime.
This correction does not claim a shared playable mine lifecycle in the other
products. Moving that service across profiles requires its own ordinary-input,
save and memory qualification rather than silently enabling mission flags.

This is a correctness qualification, with no new frame-rate or hardware claim.
The earlier renderer timing remains separately scoped. Common roster services,
campaign completion and the near-30-fps goal remain unfinished. No Actions or
SD writes were performed.
