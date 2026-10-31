# Pylon power scan predicate order (2026-10-31)

The power query scans building slots in the same ascending order as before. It now rejects non-Pylon types before checking owner and completion. All fields are read live; no Pylon list, result cache, timing change, or range change is introduced. In particular, a powered cannon still checks power before changing its cooldown.

## Paired P8 measurement

An instrumented mGBA runner resumed the same prepared P8 save in baseline `60734e56e7a768197f92208422de5f766bcf52db` and candidate `af7d6b79287502a16218133a14ad2b6d24794a80`. The input save had SHA-256 `19c733603712b9f093a435d36da9a38c434c8b52058aca933f22ffab3f12fc5e`; the initial state was frame 31676 with 103 units and 122 buildings. The runner SHA-256 was `506fb4aecaf74b820f77c43da4ac2251868c074b8a3526e98891e21aea2b06ae`.

| Artifact | Baseline SHA-256 | Candidate SHA-256 |
| --- | --- | --- |
| ROM | `54cfbe2da8169767424cc71172b1b6319d7be508fb2c58dec62225f7f734cf68` | `9300c76fff108f92fc6b6a4196b00d1847af5ebf643edb958582bb36b87477d1` |
| ELF | `ddd9c75990f1ba0e08b34e486adfb75be8356f689658b8ce2869e5aaf5112eb4` | `4d98f66abd820a3146d2c131078e184af17035b171f824a1c80be163dc9bb13a` |

All 901 captured state frames and nine pixel checkpoints matched. Over 900 measured frames, mean update cycles were 1,158,896.82 versus 1,134,550.66 (−24,346.16, −2.10%); mean full-frame cycles were 1,678,587.16 versus 1,650,812.59 (−27,774.58, −1.65%). Update p95 was 2,584,409 versus 2,562,085 cycles; full-frame p95 was 3,089,253 versus 3,089,306 cycles.

The paired result is one P8 save and is not a skirmish matrix, an FPS guarantee, or a measurement on physical hardware. It does not establish that every scene improves. The private inputs and artifacts are not included in the source tree. Local result JSON SHA-256: `066b25c8f1f67a16cf76987727801b1067042bfccc090b3e58e89652cfc35fab`.

The P8 full-frame maximum rose from 5,562,636 to 5,588,258 cycles in the paired sample, despite the lower mean.

## Paired P7 skirmish measurement

The normal P7 build at `af7d6b7` was compared with the existing `8453909` runtime baseline using a natural skirmish save. The baseline ROM and ELF SHA-256 values were `0e1cd2c7c45ecb735b186a7a5ea40f1cc1761d5d21c6deabc9046b2769e33e8a` and `2a88314a8274197f4cc6903476904cf2db0213cf88533cf175e5c851cd54b90e`; the candidate values were `06fa97d12d5b9b375ce56ce754487a2f44003eb893a6c2dc2ecd8ad5819d9b6a` and `1f460176ad20ab6780e8518f0df6271ca4fa305c1a3564df4ca76774a6bc58af`. The save SHA-256 was `fea24ab44cbaf0688348c0508f05bd67e6f6a8d1c07357cd237e15005b182cf1`.

All 901 captured state frames and nine pixel checkpoints matched. Across 900 measured frames, mean update cycles were 657,894.52 versus 657,155.24, and mean full-frame cycles were 1,054,739.09 versus 1,053,802.64. Full-frame p95 was 1,123,777 versus 1,123,603; p99 rose from 1,403,996 to 1,404,213. This is one skirmish save, not all races, AI counts, or maps. Local result JSON SHA-256: `4c3538322b9835671a6c2599ab0cc9c6e88f63c6bdd0e12527aed17bf9e2734e`.

## Checks

A clean, unprepared `af7d6b7` worktree passed `./dev build` and `./dev test`. The prepared `test-sc64-skirmish-runtime` passed Terran, Zerg, Protoss and Showcase profiles. Prepared unified and P6 `sc64_probe_cannon` binaries both failed at the menu-row-count assertion in `select_cannon_row` before reaching their power and placement scenarios. Both failures were reproduced on baseline `60734e5` with the same prepared inputs and flags. The full three-profile target could not be run because the legacy Z6 probe bank was not present. No private input was generated for these checks.

Normal P8 baseline and candidate both ended BSS at `0x02038cbc` and IWRAM at `0x030049e0`; normal P7 baseline and candidate both ended BSS at `0x020382f4` and IWRAM at `0x030049e0`. Source-boundary validation passed with the audit staged.
