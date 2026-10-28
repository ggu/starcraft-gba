# Z7 CONTACT mask geometry table experiment

Status: selected for the canonical Z7 candidate only; other profiles and the
historical Z7 probes leave it off. The full-trace host replay, bounded paired
ARM timing run, and fresh-process ARM continuation have passed. This is not a
30 FPS or full-mission average claim.

This experiment replaces the inner eight-direction circle arithmetic in
`SC64_CONTACT_CACHE` blocked steering with a ROM lookup. It does not change
collision eligibility, candidate filtering, path choice order, or save data.
`SC64_CONTACT_MASK_TABLE` requires `SC64_CONTACT_CACHE` and
`SC64_ZP_CAMPAIGN_ACTORS`; builds without the flag retain the existing arithmetic
loop.

The authored generator, `tools/sc64_contact_mask_table_gen.py`, writes the C
header into an ignored build directory. It uses no native game assets or
reference data. The generated array has 887,250 bytes: 14 radii (6–19), 15 stock
steps (0–14), and a 65×65 offset domain (−32…32 for each axis). The existing
strict axis check admits only offsets smaller than `radius + speed`; the largest
supported sum is 33, so every admitted integer offset fits this table. Radius,
speed, and offset values outside the table bounds use the original arithmetic
predicate.

## Evidence

- Host range scan over unit type IDs −16…511 found radii 6…11 in the selected Z7
  profile. The table’s radius limit of 19 remains conservative for the authored
  Z/P actor-bound domain. Enumeration of 228 stock rows × 1,024 phases × 240
  slots × both diagonal states found a maximum step of 14.
- The asset-free generated-table oracle compared all 887,250 entries against an
  independent integer arithmetic implementation. It passed. The CONTACT
  differential fixture also passed 3,289,104 step checks, 506,016 mask checks,
  and 171,024 stock-motion checks with no mismatches under ASan/UBSan.
- The selected CONTACT release vector was expanded with `make -n`, audio enabled
  against the prepared format-2 bank, and the native-input prerequisite safely
  overridden for the dry run. The table build adds only the table gate and its
  private include path. Its 32 MiB ROM link passes the guards: BSS end
  `0x0203d7e0` (2,080 bytes spare), IWRAM end `0x03004d28` (728 bytes spare).
  The 887,250-byte table resides in ROM. Its fixed header, logo, SRAM signature,
  and checksum were verified; it was not run in an emulator.
- The 365-checkpoint host replay and parent-owned bounded ARM timing run are
  validated. The strict continuation reached frame 205,764 with victory,
  50 live player actors, zero remaining Force 1 buildings, and no fault. It
  resumed from the prepared generation-36 UI save and covered the 31 remaining
  checkpoints; it is not a current-source full-prefix replay.
- On a separate paired 2,000-call ordinary suffix (1,547 advancing PLAY
  frames), the table candidate used 5,765,637,383 measured emulator cycles
  versus 9,537,458,884 for the arithmetic baseline, a 39.55% reduction on that
  bounded workload. Six state/RGB/VFX records matched exactly and both runs
  retained the same painted-stack margin. This does not establish average
  full-mission performance or hardware frame rate.
- The fresh-process continuation's 36,068 input calls also matched all 31
  remaining checkpoints and the strict terminal predicates. Its recorded
  update-plus-draw total was 89,669,608,280 cycles versus 161,885,185,038 for
  the arithmetic continuation (44.61% lower on this continuation). The table
  run reported 3,656 bytes of painted-stack high-water use and a 256-byte
  reserved gap. This begins from a verified pre-PR93 actual UI save, so it is
  continuation evidence rather than a current-source full-prefix comparison.
  It corresponds to about 6.27 advancing simulation frames/s on this tail,
  below the near-30-fps target; there is no physical-GBA timing result.

The asset-free table generator oracle is available as
`make test-sc64-contact-mask-table`. This target generates only the mathematical
header under ignored `build/generated/`; it does not import or rewrite shared
asset banks. The Z7 campaign test target runs the same oracle alongside its
table-enabled canonical fixtures. The runtime flag is enabled for the canonical
Z7 candidate and its matching fixtures only; historical Z7 probes, Z6, P7, and
default profiles keep their prior flag vectors. The selected table-on and
table-off replay pair changed only this kernel selection; state, framebuffer,
and VFX records remained exact at every reported checkpoint.
