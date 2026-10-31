# Z8 Nydus Canal source art

The recovered Z8 sprite bank had no completed-building frame for raw 134, so
`draw_building_art` used its generic rectangle fallback. The generated
construction bank already contains raw-134 stages 0–2 and their source image
frames, and the Z8 compile vector enables source construction art.

The opt-in `--include-z8-buildings` sprite-import option adds a separate
`SC64_Z8_RUNTIME`-gated Nydus frame bank and raw-134 building lookup. Existing
Z7 and other sprite-generation options remain unchanged. The normal renderer
already asks the building bank for raw 134; no gameplay or building rules are
changed. The Z8 prepared-input test target checks the actual renderer binding,
completed-frame raster and three construction-stage bindings.

From the verified USA source ROM (SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) and pinned extractor commit
`09b57f787a247107d1de82b8c5eab2734667ea1d`, raw 134 resolves through flingy
29, sprite 174 and image 86 to GRP `00D/064` (SHA-1
`a8cd65fb894eb479dcfde5a6861520f84458b86b`). Its five frames are retained;
the source Init frame is 0, with 64×64 canvas and a 36×36 visible rectangle at
(14,11). The importer uses the existing source Init-layer composition rules;
image 87's modifier-10 shadow remains deferred, as in the established sprite
pipeline. This does not claim animated Nydus working states or shadow fidelity.

To regenerate a Z8 bank, invoke `tools/sc64_race_sprites.py` with the existing
Z8 sprite options and `--include-z8-buildings`, writing both `--out` and
`--header` under ignored local paths. The generated header and previews contain
private source art and are not committed. A stale bank without the new option
continues to use the legacy fallback until regenerated; `test-zerg-z8` checks
for the new binding.

The recovered Z8 bank was reproduced with the following option set; add
`--rom` and `--remap` pointing to the locally verified ROM and the existing
Backwater palette remap:

```sh
python3 tools/sc64_race_sprites.py --rom "$VERIFIED_ROM" --remap "$PALETTE_REMAP" \
  --include-zp-campaign-actors --include-z5-actors --include-z5-doodads \
  --include-z6-actors --include-z6-buildings --include-p6-actors \
  --include-p7-actors --prepack-unit-words --include-z8-buildings \
  --out .local/sc64/zerg-z8/generated/sprites \
  --header .local/sc64/zerg-z8/generated/sc64_race_sprites_generated.h
```

At source `b1b87f7`, regeneration without the new option reproduced the
recovered header SHA-256
`2324f47a46b3e4b1c3f18d7df8fa480df1ea7c8e208e86939c0cf77da6b65e37`.
The option-on private header SHA-256 was
`7785f22869f80d478c954200dc1d53ad862127023fdf2677ec983e239c2f580e`.
The prepared `test-zerg-z8` passed, including an actual completed-building
draw whose 807 opaque pixels matched the generated source frame and a check
of all three construction stages. The existing Z8 fixture also passed its
39-instance, two-link, save-continuation and cloak checks. An isolated host
framebuffer capture was inspected locally; it is not an in-mission hardware
capture or a full Z8 playthrough.

The Z8 ROM built and verified at SHA-256
`7bda2e79dca43edc9c9c1ea5fb8532b265785cc9f14337d87d540cbaa4fd1a97`,
with EWRAM `0203dbb0` and IWRAM `03004400`, unchanged from the no-art Z8
baseline. All 109 shared input hashes and the Z8 bank, descriptor and program
hashes matched before and after qualification. The exact source passed the
clean unprepared doctor, source-boundary, default build and full default test;
the latter includes 921 Python tests (25 skipped) and native/host fixtures.
