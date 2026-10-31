# Shared shape drawing and IWRAM qualification

Runtime `74b88c2` is based on shared `7b14aa8`. Imported profiles inherit
`GBA_SHAPES_ROM_ARM` through the existing shared import flags: line and
frame wrappers run from ROM in ARM state, while text and rectangle drawing
remain in IWRAM. Horizontal and vertical lines use the existing rectangle
writer after clipping; diagonal lines retain the old Bresenham path. End
pixels remain inclusive, reverse endpoints work, and axis clipping happens
before subtraction, including extreme integer endpoints. The original-art
default does not enable the flag and its ROM remains byte-identical to the
row-candidate default (SHA-256
`6e5556d7fbbab348ab54bdc34ea8a228077b2981817f27100a1dd9f9f90aea0d`).

This is primarily a memory improvement, not a frame-rate breakthrough.
Each of the four normal imported products reclaims 392 IWRAM bytes, with
unchanged BSS, actor limits, cadence, stack reserve, and linker guards.

| Product | BSS end | Prior IWRAM end | New IWRAM end |
| --- | --- | --- | --- |
| Terran | `020370b8` | `03004d90` | `03004c08` |
| Zerg Z6 | `02037124` | `03004e60` | `03004cd8` |
| Protoss P7 | `020382e4` | `03004c40` | `03004ab8` |
| Showcase | `0203df38` | `03004fc0` | `03004e38` |

## Exact runtime checks

- Clean indexed source with no private inputs: source boundary, doctor,
  default build and default test exit zero. The suite runs 778 Python cases
  (23 optional skips), the separate eight-test suite and C fixtures,
  including a 2,500-case poisoned-framebuffer shape differential.
- Normal `unified test-unified terran test-terran
  test-sc64-terran-full-campaign` passes as one sequential original Make
  aggregate. All nine emitted Terran controller traces match the exact
  b638b50 row candidate byte for byte; T3 emits none.
- Four normal Make ARM products pass header, ELF payload, padding, embedded
  audio and unchanged memory checks. They retain BC/CV5 and all other
  canonical capabilities; the Scarab runtime flag is absent. Captured
  compiler dependency passes tie the products to 104 unchanged prepared
  inputs. No private generators are invoked.
- Four profiles pass 72 ordinary-input host matches and 96 save
  continuations. Each of the 72 individual match traces matches the exact
  b638b50 parent. Aggregate files containing output paths are not used as
  state comparators.

The fixture-only child adds `make test-gfx-shapes-tile`, which passes 1,000
additional ASan/UBSan cases comparing the full tile buffer, dirty/drawn/art
masks and band flags, including zero color, scene clipping and art capture.
It changes no runtime source or existing compiler flags/recipes. The final
audit is also documentation only; broad qualification is scoped to the
compiled runtime above, with the new tile target verified separately.

## Fresh production timing and pixels

A fresh 7b14aa8 baseline and candidate replay use the original probe
`e81a735586cf310f1641cea1c149b349863eb25505333b7a52c66d278e5f8ca8`
and tape `0d79706afb77e58c958a6dd622a56843f611f4400bfa014bdbeb7db751f6ef2b`.
The original 104 inputs remain fixed. After binding source/output paths,
the ordered feature vectors differ only by the shape option; exact commands
are retained. No experimental Scarab gate or art is included.

Busy Zerg/three-AI calls 8,021–9,020 take **714,202,651 → 714,539,530** guest
cycles: +336,879, or **+0.04717%**. HUD cycles change
71,499,399 → 71,351,510; map cycles change
277,361,639 → 277,534,728. All 90 sampled selected-state/fog/displayed
RGB555/outcome checkpoints across 9,020 calls match, including ten in the
timed window. This is near-neutral/slightly slower in this one instrumented
scene; it is not a general speedup, all-state equivalence or hardware FPS.

A separate observational UI probe and normal input tape reaches title,
an actual drag-selection box (`box_state=2`) and pause (`screen=2`). All
90 sampled state/fog/RGB/outcome/box checkpoints match. Two 100-call bands
per state are near neutral, include presentation waits, and do not prove
frame-by-frame or hardware timing equivalence. The original busy probe is
unchanged. Private receipt SHA-256:
`ffa02f89fb638795512d2c83aa9e72f21c7f0e24a14905efc500e7afc3a95097`.

Earlier experiments under a separate Scarab-gated world vector do not
transfer to these normal products. Moving all text to ROM fit memory but
cost 1.06% in that workload and was rejected. Keeping text hot and moving
the shape wrappers was separately measured there; only the fresh normal
production result above supports this extraction.

This change enables no new actor service, campaign mission or AI policy.
Terran 10, Zerg 6 and Protoss 7 campaign menus and map/race/1–3-AI custom
selection remain. The full 30-mission/shared-roster/near-30-fps goal is still
unfinished. No assets, ROMs, saves or private captures are tracked; no
Actions or SD writes are performed.
