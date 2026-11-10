# Experimental all-custom research and Flash composition

This is a disabled-by-default experiment, not a normal release or a hardware qualification. The final tested source is `340ecd449436f26c331ed795cc14f017feb7ba76`. It combines all-custom admission, the normal Z9/Z10 HUD storage flag, the physical matrix driver, and current documentation. Normal products do not enable research/Q8/Flash.

Six experimental ARM targets linked at `6f7892a01337df545a1d379a545ab309bb8dff48` using the optional `tests/sc64-research-q8-arm-custom/Makefile`, the existing frozen audio bank, unchanged linker guards, and 593 checked private inputs. Relative to this link source, `340ecd44` changes only documentation and the new driver/test; `common`, `starcraft`, root Makefile and the existing test recipe are identical. IWRAM and EWRAM margins below are bytes remaining before the existing guard thresholds, not measured stack high-water.

| Profile | IWRAM margin | EWRAM margin | ELF SHA-256 | Map SHA-256 |
|---|---:|---:|---|---|
| Terran | 2,856 | 28,336 | `ed4c4bb7c38096dd95f92630087887f229e4e39311c2866457c9522ef5e9ee00` | `35647fcad6094f58f4d49c9cc83430ca0507ac902e12e297f8cd5eeeb79e20e6` |
| Z9 | 1,936 | 12,588 | `a1555f91092c5be290eda17ade83c56921bb9bfd74d8c776c3b28d91e4ecd14c` | `c3c6d9a134e60194574fd8a32766147dadc0a56835f0ac58cee36a69895fae89` |
| Z10 | 1,936 | 12,588 | `424d35b9e5a71d16355fb189b705ed81f631f6a0e6b4c60b5be321a7e001ce39` | `c63ee909a7f5eccb41e79a387230ab6c78080f6809e06e301a8c776634ffaa68` |
| Z7 | 1,024 | 2,292 | `1170908a0e3241325dfba2fe5c0b47764aaf727c2acbbda94d46d103cfa5285b` | `0de16b832d1cf33250166cfad1656257422d69143789094c71b6558e9eb150ab` |
| P10 | 2,056 | 19,324 | `d6a0f2e7572bc75c25363ba8884bec162174bed7ab805d2ba407ae0b3a8a7ed1` | `355e5e5c312b17c9e666c1590fe57fdfb739de98ad05927098c0f5be76b93c62` |
| Showcase | 1,464 | 28,676 | `19bad2a3b6d9b5c7b06637901e395060ca268b8538d29c712ecd1532d8522545` | `1af778996fafdc8a0b0471c706234252eaf7c5e1e91e334832afbd4fff31bab2` |

Z9/Z10 normal ROMs have 13,244 bytes EWRAM margin; that is a different vector from the experimental 12,588-byte margin above. The P10 experimental ROM was byte-identical to the earlier `d7e7554e` physical-matrix ROM after the same header fix, SHA-256 `b65b068ff5e28610ae009782c56f8276d72c3d0131aaa22db16e21879791f28f`. All 28,999 defined symbol address/size/type entries also matched. An initial comparison of the uncorrected raw binary against the header-fixed hash failed; it was retained as a comparison-procedure error, not a link failure.

The [physical matrix](../2026-11-07-q8-custom-physical-matrix/README.md) passed 234 early guest openings; its saved research levels and Q8 debt were zero. Separate HOST checks cover 234 custom records and 234 genuine predecessor records. These results do not establish active research, nonzero combat fractions, all campaigns or hardware behavior for the matrix.

At `340ecd44`, a clean asset-free checkout passed source boundary, `./dev doctor`, `./dev build` and `./dev test` under the shared native lock. The test took 461.895 seconds and included 1,034 Python tests (26 skipped) and compiled checks; no ASan/UBSan diagnostics were found. Private logs, inputs and saves remain outside source control. Campaign and legacy transitions, repairs and aliases, failure recovery, runtime stack, full combat and physical cartridge behavior remain release gates.

The retained six-link receipt SHA-256 is `dbe3fa68ebfd6829a8680741a59a34aaa40e0d7c01325318d75dd556fc55483a`; the clean default receipt is `cb3c120d45392af1f86478e5d5651bbbc1eaffbf88ab53e7de6f4fc93a0a1ba1`. An independent check of all 234 actual guest save hashes, journal checksums, roster identities and named continuations produced receipt `925f4aa66f2726eca470dfefb9f9ebb22e94a5c36b559c112e8c4f4d5fbc40d7`.

A later [paid Forge witness](../2026-11-07-q8-paid-research-physical/README.md) uses the same P10 product and adds three focused paid/context/deadline contracts alongside the seven existing supervisor contracts. Its controller and tests are new files; the game, build definitions and existing tests remain identical to `340ecd44`. The full default suite result above belongs to `340ecd44`; the new files have their separate focused and actual-guest qualification. Damage debt stayed zero in that witness too.
