# Experimental Scarab control fit on current route candidate

The candidate merges the private human-control, cleanup and icon checkpoints onto fresh-cell route candidate `7e3132811554a9ade569bf38b001109fa4c1d8b1`, retaining both histories. All Scarab activation flags remain OFF in ordinary products. The route candidate's own qualification and promotion remain separate.

The first full-probe Terran link on merge `ce2f8a8` failed the unchanged linker guard: IWRAM ended at `03005008`, past the required `<03005000`; BSS ended `020370d0`. The failed map and exact command were retained. The subsequent experimental parent-step wrapper is explicitly `noinline` and runs cooldown and paid production in the same order, once on the existing source cadence. Exact before/after Terran game-object measurements put `update_orders` at `0xcc0`/`0xae0` bytes in IWRAM; `game_update` stayed `0x262e`, and the new `0x158`-byte wrapper is in ROM `.text`. IWRAM object size fell from `0x3860` to `0x3680`. An initial diagnostic object command incorrectly retained an assembly input alongside `-c`; its failure was retained separately and is not evidence for the valid comparison.

At source `5c1e57ecb85f5b07691214851900806ab1b9b08e`, full experimental links with the shared bulk-grid and fresh-cell flags passed the existing guards for all four products:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `020370d0` | `03004df8` |
| Zerg | `02037144` | `03004ed8` |
| Protoss | `020382fc` | `03004c00` |
| Showcase | `0203df50` | `03004c78` |

These guard results are not performance, hardware or production-admission claims. Exact argv, ELF hashes, maps and the failed link are retained locally; no guard or actor cap was weakened.

## Art actually selected

The four measured vectors select their standard race-sprite banks and standard facing/walk banks, none of which contains a raw85 Scarab body or facing/walk entry. A visible flying child therefore takes the generic procedural body fallback. All four select the existing death bank with raw85 mapped to image 444's ten-frame death program; the experimental removal path invokes its death effect. The two existing command-icon banks contain a nonempty 13×13 raw85 portrait, used by the gated `BUILD SCARAB` menu and card mappings. The additive body and facing closures are available privately but were not selected in these links; their matched walk sidecar, real rendered behavior and visual qualification remain production holds. No proprietary asset or image is committed.

Factory admission stays closed. Missing parent-removal paths, malformed graph recovery, loaded transport/cargo support, capacity upgrades, complete native pathing and flight/save support remain open. The staged save path still refuses live outside children and pending packets; it is not whole-world atomicity proof.

At the same runtime source, eight focused ASan/UBSan fixture compiles and 48 runs passed across all four vectors and both maps: the human menu/icon/paid-training case and five parent-cleanup cases per map. The cleanup cases include normal damage during training, stored ammo and flight, STOP, and an explicitly injected unsupported transported row; they do not prove ordinary Reaver boarding. To limit disk use, the first three compiles retained `-g` and the remaining five used `-g0`; all retained the same sanitizer and gameplay defines, and exact commands/results were saved. A controlled interruption and the already completed records were retained before the debug-info-only resume. All 104 frozen prepared-input hashes matched afterwards. The full default suite for this child and new native/hardware runs were not performed; the parent route candidate has its own separate qualification.

## Current shared carry-forward

Merge `0092234cad239829b9a267d056d2fbed17d90a08` adds current shared `e23038d3b99b2ccf536f89887ffebab845c340ad` while retaining this history. Compared with the runtime source `5c1e57e` used for the ARM and focused host runs above, its only changes are three audit Markdown files. All four prepared product flag vectors compare equal when evaluated with the same audio inputs. The unprepared clean worktree omits private audio as intended.

On the frozen merge, clean unprepared `./dev build && ./dev test` passed: 852 Python tests, 23 optional skips, the additional 5- and 8-test groups, and the native checks. The default ROM verifier passed and the source boundary check passed 1,355 indexed text files. This documentation-only follow-up records that result; it does not activate the experimental flags or close the art, lifecycle, save and gameplay-admission holds above.
