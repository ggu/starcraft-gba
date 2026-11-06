# Early Protoss physical save points on P10 product

Seven bounded early save points were exercised separately through ordinary menu/buttons, each on the same unmodified packaged P10 product. A fresh mGBA process exported a genuine 32 KiB SRAM after pause-menu B; another fresh process loaded it through title-screen L. The saved and restored selected-state fingerprints matched; so did the uninterrupted and restored continuations after 120 physical updates. Exported SRAM was unchanged by loading.

| Mission | Global mission | Saved frame | Frame after 120 physical updates | SRAM SHA-256 |
| --- | ---: | ---: | ---: | --- |
| P1 | 2 | 121 | 241 | `45ef7a1de7050f9226e4ef72c26f047cbaf63e1e8abfc7bd720c9b9f4c57e1c5` |
| P2 | 3 | 10 | 130 | `96a71d203d53571bc8654486ae463d46bc8dffd009adffb7ada5208efba9f3d1` |
| P3 | 22 | 121 | 241 | `42cdc65d731af236aea46ad130e3f5b2ad64972868ae2b2ea9301b3790e20b71` |
| P4 | 23 | 121 | 241 | `fbfedaf98a8f2771ffbb8ef6596d9c809b939f6eafc00fac35f92a3330752c34` |
| P5 | 24 | 121 | 241 | `65473a9edb4f4b332ccb53858c9ed9dec2fa473522d50ae6d1320f121a7725f8` |
| P6 | 25 | 121 | 241 | `e974f20a66daf714978ee64ef21af5fcbb1c82307c51fefb870ba0a87b64816a` |
| P7 | 26 | 3 | 93 | `8962971da1fdba7448112c6d56036d0fda4841e2cbc6e3bc3301f1f422c784a7` |

P2 and P7 required an ordinary A acknowledgment after START was blocked by an opening modal; their save points are especially early. P7 advanced only 90 simulation frames during the 120 physical updates because some updates remained modal. These are opening checks only: fingerprints cover selected live unit/building fields, resources, map resources, frame and mission/mode, not every actor, objective, trigger clock or source-owned state. They do not establish a winning continuation, later or arbitrary save points, physical-GBA behavior, visual parity or native mission equivalence. No save rejection was found at these seven points.

The ROM SHA-256 is `ff94a2ea9f98404c97f44a8b6b7bf557b3572c761de530d867032eeb8f76b10b`, paired ELF `f961647217516de367b2f2e28b1b82dee4e744e3bef7f9c7b0f46e4100f55587` and pinned physical runner `4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2`. All 392 hashes in the 94bc release package input manifest matched before and after each run. The driver source is `tools/sc64_early_protoss_save_probe.py`; private SRAM, process logs and full receipts remain in the isolated `.local/early-protoss-audit` directory and are not committed. The packaged P10 binary predates the audit's `c27bb48` source-only status merges; its exact binary identity, rather than that source head, is the test subject.

Two earlier diagnostic attempts are retained: an initial P1 driver wrongly assumed consecutive global mission IDs and stopped before save; the first P2 attempt could not open pause while a modal was active and also stopped before save. Neither was an engine save rejection. The corrected driver checks the selected front-end mission and permits bounded ordinary acknowledgments.

A useful next test is the retained ordinary-input P1 controller tape (`8b24ad830e33a19f2c6f0ba0300f54090d410955fa95cbc43a8c0fb64fe9225d`, 9,289 calls), first verified against this exact product and then interrupted at a nontrivial save point for fresh-process continuation through an actual win. The historical tape is from an older profile and is not itself proof of a current-product win.
