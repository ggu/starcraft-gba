# Optional source VFX lookup index experiment

`tools/sc64_weapon_lookup_index.py` derives a separate experimental header from an already prepared source VFX bank. It preserves the bank's rows, pixels and fallback semantics, replacing only the image and animation lookup functions with bounded ROM tables. It rejects zero or inconsistent logical row counts, unsupported or multiline lookup formats, unsupported key domains, duplicate or unsorted keys, existing or source-alias output paths, and non-ignored repository destinations. It does not modify the source bank or enable a product flag. Select the derived header explicitly using the existing `SC64_WEAPON_SOURCE_BANK_HEADER` definition; no new art or private content belongs in source control.

The payload-preservation property independently hashes each complete original `sc64_weapon_source_*` array definition before and after transformation. The prepared bank had 13 original arrays, including profile, frame, pixel and LO payloads; all 13 definitions matched exactly, and the only new static arrays were the three lookup directories. The derived-header SHA-256 remained `d8f12875e21698ab82e2562238690c067c06bf7b1e65129abb58a5291640b896`. The private per-array receipt contains hashes only.

The asset-free sanitizer test exhaustively compares 65,536 image IDs and 16,777,216 image/animation keys on a synthetic sparse bank. With prepared private inputs, the same exhaustive comparison passed against all original resolver results for the actual 150-image, 423-animation bank, including holes. Its highest image ID was 741, maximum animation span six, and index tables occupied 2,968 ROM bytes. This verifies resolver results for that bank; other generated banks must pass the same comparison and bounds checks before use.

On shared source `3a0bd87`, an ordinary-menu Zerg/three-AI paired emulator probe with a custom diagnostic main (not a normal Make product ROM) used the same ordered feature vector, source and input tape; only the candidate selected the derived header. Over calls 8,021–9,020 total update/audio/render guest cycles changed from 716,368,112 to 700,461,947 (−2.2204%), and map rendering from 280,361,885 to 268,157,427 (−4.3531%). The sampled whole-loop maxima were 2,208,345 and 2,206,193 cycles. All 90 sampled actor/fog/RGB555/frame checkpoints matched; this is sampled parity, not complete-state equivalence. Both links retained BSS end `02036f44` and IWRAM end `03004cd8`; all 104 prepared input hashes matched the retained reference inventory after the experiment. This is one instrumented emulator workload, not a general FPS, hardware or worst-frame claim. Full four-product, default-flag, save/resume and representative workload qualification remain open.

The separate feasibility observation found the Mode-4 OBJ tile region already allocated to the terrain cache and, in P7, the projectile arena. Unit sprites also precede tracer, death VFX and HUD drawing in the software painter order, which a single OBJ layer cannot generally interleave with the bitmap. The experiment therefore leaves rendering and sprite assets unchanged.

## Clean source qualification

At source commit `a089333a4904f50eb564bd0bd6dae78bb74fd38a`, a clean detached worktree without prepared private inputs passed `python3 tools/check_source_tree.py` (1,225 indexed text files), `./dev doctor`, `./dev build`, and `./dev test`. The default test ran 786 Python cases with 23 optional skips, the separate eight-case suite, and the native sanitizer fixtures. The default ROM SHA-256 was `e612df7f0bd1417754e8665ca293bc53eedb194d55f93e2b996ea54ede1bd69c`. The default build does not enable this optional index.

## Current shared integration qualification

Port `127a255d5151f6d73f51da89482cbb5d63224f84` is based on shared
`4bd552953a310775ffc9baf31eb191993a77012b`, retaining the enabled #196
profile cache and optional #195 diagnostics. Current clean boundary, doctor,
default build and default test commands pass: 787 Python cases with 23 optional
skips, eight ancillary cases and the C fixtures. The original-art ROM hash
remains the value above. Independent source review and all five focused tests
pass. Canonical product defaults are still unchanged by this tool.

Using the existing `WEAPON_SOURCE_DIR` Make override to select the derived
header, all four actual normal ARM products build with both duration and
profile-index caches enabled. Dry runs schedule no asset generators. The
preprocessor confirms the selected indexed bank, all 104 original prepared
input hashes are unchanged, and ELF/ROM/map archives are retained privately.
The original three ROM verifier commands and a separate Showcase check pass;
all four also match their ELF payload outside the fixed boot header and have
all-FF padding. BSS/IWRAM endpoints exactly match #196:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `020370b8` | `03004c08` |
| Zerg Z6 | `02037124` | `03004cd8` |
| Protoss P7 | `020382e4` | `03004ab8` |
| Showcase | `0203df38` | `03004e38` |

The actual four-profile canonical tracer fixture also passes ASan/UBSan with
the indexed bank and both caches: 61 profiles per product, variant fallback,
slot lifecycle and long-duration cases. Completed native test executables
were retired only after hash inventories and an open-file check; ARM artifacts,
inputs and test evidence are preserved.

### Combined cache and index timing

A new paired custom-main Zerg/three-AI replay compiles every translation unit
from the current source. Both sides enable the canonical cache; only the
candidate changes the selected bank. Over calls 8,021–9,020, update/audio/render
cycles change from 708,822,986 to 693,086,989 (2.2200% lower), and map cycles
from 274,108,590 to 262,134,263. All 90 sampled actor/fog/RGB/frame checkpoints
match. Peak map cost falls from 559,913 to 545,578 cycles, but peak whole-loop
cost rises from 2,017,383 to 2,223,375 cycles. This is not a worst-frame win.

A separate instrumented paired rerun measures time inside `gfx_end`. Both
worst loop events occur at call 8,830 / simulation frame 8,760: baseline loop
2,017,634 with 345 presentation cycles; candidate loop 2,223,028 with 181,951
presentation cycles. Thus 181,606 of the reproduced 205,394-cycle difference
is inside presentation, with 23,788 cycles remaining elsewhere. All 90 samples
match in this rerun too. Instrumentation changes layout/timing: this supports
presentation as the main reproduced contributor, not exact attribution of
the original uninstrumented spike. Neither run proves near-30-fps hardware
performance or complete-state equivalence.

Per-translation-unit dependency, source, driver, tape, linker/audio and named
tool receipts match before/after each build; this is not an inventory of every
compiler library. The first benchmark setup failed before compilation because
assembly-only dependency output was empty; the corrected setup is recorded
separately. The presentation candidate also retained a disk-full failure and
then completed a distinct retry after retiring completed native test outputs.

The tool is ready as optional source-only infrastructure. Default activation,
indexed save/resume qualification, broader workload/worst-frame analysis and
normal-product ARM play-throughs remain separate gates. No Actions, debugger
ports, public binaries or SD writes were used.
