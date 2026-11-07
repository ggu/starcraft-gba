# Nydus building capacity follows the selected codec

The Nydus adapter imposed a six-bit building-type limit even when `RaceSave`
selected the existing seven-bit compact building codec. Full late-Zerg plus
Battlecruiser flags contain 65 building types and failed this stale assertion.
The corrected contract retains the four-bit order limit and the derived
codec's 64-type limit; compact/P7 typed rows retain their existing 128-type
limit. No codec bit layout, tag, actor cap, save reserve or normal feature
flag changes. This does not activate Scarabs, research or other staged services.

At runtime source `0bf28792fd1562fc9b0a0ccfd83620e13fadcb66`, a strict
ASan/UBSan actual-engine HOST fixture compiled full BC plus the Z10 canonical
flags, retaining worker cargo and Nydus. Its authored custom world saved
building type 64 (Physics Lab, raw 118, 587/600 HP) through the real writer.
A separate process loaded that type and HP and saved the decoded state again.
This is not ordinary construction, campaign victory, native or hardware proof.
The compact codec and Nydus rules fixtures passed separately. Wide/component
receipt SHA-256: `1abf8befa97b3d8c5878a6d7700556055aebab61e79f765d43fbf7c264acd4c3`.

A genuine predecessor was compiled from unchanged source
`074785878dcec3ac03ee80be94a3c72c88a90f60` with the earlier ENERGY/YAMATO-only
BC closure. It wrote its own HOST journal containing a damaged Factory. The
new full-BC reader loaded and resaved it in a fresh process, without retagging
or altering the input journal. This covers that authored early custom world,
not arbitrary older gameplay. The unmodified pre-fix source also failed the
same full-BC wide fixture at the original Nydus assertion as expected.

Reproduce with prepared private inputs (no asset fetching is part of a target):

```sh
make -f Makefile -f tests/sc64-nydus-wide/Makefile \
  SC64_AUDIO_DIR=/path/to/prepared/audio test-sc64-nydus-wide
make -f Makefile -f tests/sc64-nydus-wide/Makefile \
  SC64_AUDIO_DIR=/path/to/prepared/audio \
  NYDUS_WIDE_LEGACY_SOURCE=/path/to/unchanged/07478587/starcraft/game.c \
  test-sc64-nydus-wide-predecessor
```

The public predecessor recipe was separately tested at `d951be74`; its only
change from the tested runtime is the optional recipe. Its receipt SHA-256 is
`60305e7ec3a14c69e374e227471d1587105180577abcac678d4da46c52c08258`.
The old narrow source has a pre-existing unused-parameter warning, explicitly
suppressed only in that predecessor compilation. Initial fixture race-name,
warning-option ordering, supervisor path and normal-link output-prefix failures
were retained privately; none qualifies a product. Prepared input hashes stayed
unchanged (593 HOST inputs; 595 normal-build inputs).

All six **current** normal vectors linked with compiler dependency audits and
byte-identical ROMs to the qualified `34c101e0` package/legacy Z7 control:

| Product | EWRAM margin | IWRAM margin |
| --- | ---: | ---: |
| T10 | 28,884 | 2,944 |
| Z7 | 2,948 | 1,168 |
| Z9 | 13,244 | 2,072 |
| Z10 preview | 13,244 | 2,072 |
| P10 | 19,992 | 2,056 |
| Showcase | 29,336 | 1,464 |

Normal-matrix receipt SHA-256:
`c166500d34673d62e28c6f19ac557edc5b3e092b71fcc2579ab8ea42b559fad7`.
A separate clean, asset-free checkout passed source boundary, doctor, default
build and full `./dev test`: 1,023 Python tests, 26 skips, auxiliary and compiled
checks, no sanitizer diagnostics; full test took 452.892 seconds. Receipt:
`ee5ec8e3e8f10d40d542f17e0c23776dd639f6dcc1ae55959a7b322f21210287`.
Prepared `make unified` also passed, receipt
`9a4a3ed83259e5ff06bfecb69556fd30cb7a306aa6e910e136f4d75ff6d22742`.
Full `test-unified` and Terran checks lack
`.local/sc64/bc/generated/sc64_assets_generated.h`; no input was fetched or
generated to fill that gap. Checks were local, serialized with the native lock,
at least 4 GiB free and a 256 MiB per-file limit. Link margins are not stack
high-water evidence. New full-BC ARM gameplay remains a separate qualification;
the already-qualified normal game bytes are unchanged.
