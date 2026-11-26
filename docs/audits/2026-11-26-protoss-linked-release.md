# Protoss P1–P10 linked replay, 2026-11-26

The headless mGBA ordinary-input chain reached victory in all ten chapters. Each victory SRAM was reloaded in a fresh process and its readback matched byte-for-byte. The subsequent chapter used the preceding readback, including the P3-to-P4 bridge described below. Recorded input hashes were independently rechecked against the available files. Testing used one recorded controller route in mGBA.

| Chapter | Victory frame | Evidence |
| --- | ---: | --- |
| P1 | 27,764 | `p1/result.json` |
| P2 | 39,217 | `p2/result.json` |
| P3 | 48,801 | `p3-result.json` |
| P4 | 84,220 | `p4-paired-rescue/result.json` |
| P5 | 5,057 | `p5/result.json` |
| P6 | 51,746 | `p6/result.json` |
| P7 | 41,224 | `p7/result.json` |
| P8 | 93,065 | `p8/result.json` |
| P9 | 101,649 | `finale/result.json`, `finale.log` |
| P10 | 53,225 | `finale/result.json`, `finale.log` |

The complete chain and P4 receipts are in the private qualification archive `protoss-linked-c96-receipts.tar.gz`, SHA-256 `5af21ac3abd02fdc40b0df1edecb6761c426e0d680deb7ad642933deff2b68b0`. Paths in the table refer to `chain/` in that archive except P4, which is under `p4-paired-rescue/`. The archived files were verified against their source files. P9 and P10 each matched the recorded checkpoints and their fresh result and SRAM.

The actual P1–P3 and P5–P10 replay ROM came from release `c96a88ca` and has SHA-256 `a86f81611a7807565c32a1ea81ef662a66beb74f95ded71af2e316bdd9fe7cc5`; its ELF was `fb95f54710f1c20057edf478f295167b91315f5dab0a2c14a8872cd767426b7c`. P4's earlier receipt pins the same ROM SHA and a different ELF (`a01104bbf0d256f9f0bd13fde323009c86ffd36cd1fb971030a03e95cd0d7e6b`). Its recorded input P3 SRAM SHA `09ed33e7311476ca976ecdb3d1443f3bc4da84930dc9b0445df5f59cbd5247d6` matches this chain's P3 readback; its P4 fresh readback is the actual P5 input. The original P4 trial ROM and ELF were not available to independently rehash; their identities are based on the archived receipt.

This ROM is the preceding facility production build, not the later #978 packaged Protoss ROM (`9143f0ace34b31472677391cbc1b0b9fca86b2c4fdbb883e040caf1d1350806a`). The chain therefore does not establish P1–P10 gameplay on the #978 ROM.
