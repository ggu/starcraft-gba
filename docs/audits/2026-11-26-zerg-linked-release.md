# Zerg Z1–Z10 linked replay, 2026-11-26

The normal Zerg10 cartridge at release `c96a88ca` completed all ten chapters from blank SRAM using recorded ordinary button inputs in headless mGBA. Every victory was freshly reloaded and re-exported byte-for-byte. Each chapter consumed an earned predecessor journal, including the byte-identical bridge to the earlier Z8 run described below. This qualifies one recorded route in emulation, not physical hardware.

| Chapter | Victory frame | Evidence |
| --- | ---: | --- |
| Z1 | 50,669 | `z1/result.json`, `z1.log` |
| Z2 | 10,209 | `z2/result.json` |
| Z3 | 68,639 | `z3/result.json`, `z3.log` |
| Z4 | 64,147 | `z4/stages.json`, `z4.log` |
| Z5 | 18,974 | `z5/result.json` |
| Z6 | 212,428 | `z6/result.json`, `z6.log` |
| Z7 | 267,585 | `z7/result.json`, `z7.log` |
| Z8 | 70,229 | `z8/result.json` |
| Z9 | 67,745 | `z9/result.json` |
| Z10 | 57,633 | `z10/result.json` |

The receipts, journals, private replay scripts and principal input tapes are in the private qualification archive `zerg-linked-c96-receipts.tar.gz`, SHA-256 `07f9acffa94d4ea81797869098312fe530097bfa6b602306a875c806b319808f`. Table paths are under `chain/` in the archive. All 170 archived files were independently checked against their source bytes; 18 available absolute-path input hashes pinned in the receipts were also checked with no mismatches. ROM SHA-256 is `1dab164dbc409a63ec6db61ed2b3c93de165007a47444f674159ec367c7a776c`; the matching release ELF is `17cdfd31c2595102cc4bc825b01828c26a09e572e5e01ba1038e8b379ea232f2`.

Z3 matched 32 recorded stage saves. Z5 matched 14 and Z6 matched 57, including fresh-process save and reload between stages; these are staged continuations, not a single uninterrupted emulator session. Z6's original final sequence left the mission active, so the qualified route includes the separately authored ordinary-input reinforcement and four recovery attacks. It won with the expected six completion bits and fresh readback SHA-256 `5485c06c84a64a26001ae3a80daf4466bdf9c3ba3ecd7cde96371ca195287b04`.

Z7 was subsequently replayed in full from that newly earned Z6 readback; all 206 sampled observations matched the prior control, with victory and fresh readback SHA-256 `05c8bf28416eef8c52cfcd9d59901e31b1ffe6f74598a023a321201c26939639`. That is byte-identical to the earned Z7 journal used to start the earlier release-ROM Z8 replay, so the separately run portions link exactly. The Z8 and Z9 fresh readbacks likewise match the respective subsequent seeds. Z8, Z9 and Z10 matched 29, 30 and 26 recorded checkpoints respectively; Z10's final fresh SRAM SHA-256 is `1123eb7c9e8977228e33d24db85f14953a202da2a4c731bfdb4b6d940768b954`.
