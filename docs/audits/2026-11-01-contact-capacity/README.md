# Z7 contact memo capacity experiment

A 16-entry contact memo did not produce a meaningful gain in the measured late Z7 window, while adding 144 bytes of EWRAM. The default remains four entries. The optional `SC64_CONTACT_MEMO_CAPACITY` override accepts 1–32 and changes only the existing array capacity and insertion bound; key comparison, invalidation and geometry are unchanged. This is an experiment, not a recommendation to enable 16. The existing memo oracle reports calls, hits, misses and unhooked world changes; it does not measure distinct keys or total invalidations.

Both normal builds used source `ca582c3f390539b1d79c5fe1078a290701fc3ef9`, based on `99059f550ebacd8c6ee85cc4a8309083bc04f0cb`, and the same flags and inputs except `-DSC64_CONTACT_MEMO_CAPACITY=16` for the candidate. The default-four ROM is byte-identical to the current 990 baseline, SHA-256 `2fbc76b01c59f957912cc40b1fe79d28bb1b72088d94cacd0349dea3eb1d6efe`; the 16-entry ROM is `af622fff2e7b697618ef717141424d6eebaba3a233162bc883d3c9f8bdf7c6d8`. Both used literal `-g`, the standard production timers and unchanged RAM guards, with no added performance instrumentation. IWRAM ended at `0x03004f10` in both; BSS ended at `0x0203d6f8` and `0x0203d788` respectively. Both builds and replays held `/tmp/sc64-native-build-20261031.lock`, checked more than 2 GiB free inside the lock, verified 109 pinned inputs before and after, and froze tracked source. The authored contact table SHA-256 was `e7993e3efec0582088f4b466368dbf3990cd4896602b01747d76515207b091ce`.

Each build ran the same physical-button strict trace through updates 50001–50900. Both passed the same 135 named checkpoints through 50396; all nine 100-update samples of named game state, displayed mode-4 frame and palette matched. Route hit/build deltas also matched (2242/77). These samples do not prove full-state equality at every update or correctness across modes. The strict trace SHA-256 was `5121ed97ea0ea3d57697ce2c7fc57eac2082e3a457bb4ad98517f09ecf510489`. The physical backend used no guest state writes.

| Cycles per update | Four entries mean / p95 | Sixteen entries mean / p95 |
| --- | ---: | ---: |
| Main loop | 2,105,632.65 / 4,182,488 | 2,105,632.67 / 4,182,497 |
| Update | 1,444,042.07 / 3,460,279 | 1,443,933.64 / 3,460,285 |
| Orders | 846,134.31 / 2,854,556 | 846,025.80 / 2,854,546 |

The 108-cycle mean orders difference is about 0.013%, with no observable main-loop improvement. Stage counters can overlap; this is a bounded emulator comparison, not a hardware FPS qualification. No broader capacity sweep or full-mission qualification was attempted.

Local result receipts are `/tmp/contact-capacity-base-50000-20261101/result.json` (SHA-256 `d3d79f8e1ce2a13a3a76209b2cac51e92ea0069fdcc94f9547a46d16635a41fa`) and `/tmp/contact-capacity-capacity16-50000-20261101/result.json` (`371519dec40f67a4a24c4010fa128be7dcdb5852c1cf30fd3bbe790f29ed749a`). Their sample files hash to `d00fb2fa01b1cb6acc32bb89309aab40030dc523cd1fc4ae8a3160e0cdd5f5ca` and `90313c16429530efcca95d2dfa5c7cbc0f186b53eac35256e12901d53484d57c` respectively. The replay script hashes to `55a74d3431bc5043f6d211d8cacdca968da0d489961380c9027ca175648311d4`; its lock wrapper hashes to `112c90b5ce5aa8902815bcf440ecbe02756190357f4a48ee3baf03d6c07c1196`. Temporary evidence and generated inputs are not committed.
