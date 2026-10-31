# Z7 contact geometry diagnostic

This count-only diagnostic measured the geometry applications in one late Z7 physical-input window; it does not change gameplay or measure production FPS. The measured source was `0d0f40d042689f58a6cb7f0d80bf7785f1a95967`, based on diagnostic source `5e5b89f142383d30a76df370827041c2b2a29703` and P8 production source `99059f550ebacd8c6ee85cc4a8309083bc04f0cb`. `SC64_DIAGNOSTIC_CONTACT_SPLIT` adds noinline ROM counters. They classify valid-class geometry applications after the axis rejection, and compare the mask immediately before and after the geometry operation. The invalid-class reference fallback is outside these counters.

| Geometry applications per update | Mean | p95 | p99 | Maximum |
| --- | ---: | ---: | ---: | ---: |
| Zero offset, nonzero speed | 878.490 | 1098 | 1224 | 1287 |
| Nonzero offset, adds mask bits | 60.918 | 74 | 80 | 88 |
| Nonzero offset, no new bits | 286.538 | 485 | 611 | 621 |

The categories sum to 1225.946 applications per update in this window; zero offset represents 71.66%. The zero-offset count excludes speed zero. For a positive speed and nonzero direction, the old squared distance at zero offset is zero and the new squared distance is positive, so the existing non-increasing-distance condition cannot set a bit. This observation motivates a possible fast rejection; it is not an implementation or proof across all gameplay paths. The nonzero redundant category includes geometry that is clear and geometry whose blocked directions were already in the mask; it does not by itself establish repeated identical candidates.

The replay used physical button input and the same strict trace as the current normal Z7 baseline. It passed 135 named checkpoints through update 50396 and sampled updates 50001–50900. At all nine 100-update samples, named state and mode-4 frame and palette hashes matched the uninstrumented current 990 baseline ROM `2fbc76b01c59f957912cc40b1fe79d28bb1b72088d94cacd0349dea3eb1d6efe`. These checks do not establish full-state equality on every update. The strict trace SHA-256 was `5121ed97ea0ea3d57697ce2c7fc57eac2082e3a457bb4ad98517f09ecf510489`; no guest-state writes were used.

The normal-placement instrumented build retained the production contact table/cache/memo, and passed unchanged linker guards with `__iwram_end=0x03004fa8`, `__bss_end=0x0203dcb8`, and the mask at `0x03003474`. Instrumented ROM SHA-256 was `3f05de1a2c3c4f3f70d94f01138cd3d989c9bae64642a03bbbad3000df8410d5`; ELF SHA-256 was `55c6021156463d2a731342416b9dc6e4dd3e052af94ee8e534f43205ba56bb0c`. Build and replay held `/tmp/sc64-native-build-20261031.lock`, checked over 2 GiB free inside it, verified the 109 pinned inputs before and after, and froze tracked source. The authored table hash was `e7993e3efec0582088f4b466368dbf3990cd4896602b01747d76515207b091ce`.

Local evidence: `/tmp/contact-geometry-50000-20261101/result.json` SHA-256 `42b1056936745e7ed710bad2c5be4d40c8a9451be3fa98ea0d86d68fe25e2554`; `samples.json` SHA-256 `ec55ea65be02cbbcac2b41e9de4f74fc41dc4ab841e6b5c09578ea73b7880285`; replay script SHA-256 `5c956c008f62c07e73776f7a1e16260d8dc4cd53bc69c63478dfefff27bb1c49`; lock wrapper SHA-256 `07f95fd51133b8013e857c15f9b030c5d7d51b97e8a3b625089795cd329527d6`. Temporary files and proprietary inputs are not committed.
