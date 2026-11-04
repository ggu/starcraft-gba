# Late-Zerg integration in progress

This branch preserves existing Z9/Z10 controller work on current main. It does
not promote the default Zerg ROM, qualify Z10, or establish N64 AI parity.

The imported source commits end at `654df95d`; three later working files were
snapshotted without modifying the original worktree. The snapshot hashes and
private evidence remain local. The controller drives HOME, campaign menus and
gameplay through `game_update(held, pressed)`; live state chooses its inputs.
No controller resource top-up, actor placement or objective bypass is intended.

## Reference audit

`tools/sc64_ai_programs.py --mission Z9` and `--mission Z10` now reproduce the
bounded static program reports using the authenticated ROM and pinned extractor,
without changing the tool's Z8 default. The extra opcode layouts are
`start_areatown` (4, no operands) and `value_area` (42, no operands), checked
against [the pinned PyMS AIBIN reference](https://github.com/poiuyqwert/PyMS/blob/bfc5d3aad0b5614a5aff72c223f8efa00afddfa4/PyMS/FileFormats/AIBIN.py).
This is format evidence; it does not establish interpreter semantics.

The staged policy uses fixed first/gap/group values:

| Mission / source town | First | Gap | Group |
| --- | ---: | ---: | ---: |
| Z9 / Zer9 | 6253 | 6563 | 4 |
| Z9 / Z9ly + Z9ry | 0 | 0 | 0 |
| Z9 / Z9lo + Z9ro | 628 | 3750 | 7 |
| Z10 / Z10a + Z10b | 6253 | 9598 | 10 |
| Z10 / Z10c + Z10d | 6253 | 10000 | 4 |
| Z10 / Z10e | 10003 | 10625 | 4 |

These are provisional authored approximations, not the original scripts' attack
sequences. For example, Zer9's first pass requests groups of 1, 2, 4, 8 and 3;
Z10a requests 1, 2, 3, 8, 12, 12, 6, 10 and 25. The original programs also repeat
subsequences through branches. One average does not preserve those sequences,
unit composition, production dependencies or attack readiness.

The existing private N64 diagnostic logs include a run marked `patched overlord`.
That run cannot establish untouched ordinary-input campaign balance or timing.
Do not use it as qualification evidence. Use independently identified ordinary
input observations for future native comparisons.

## Required before promotion

- Reproduce Z9's controller, host replay, ARM replay and genuine pause-menu SRAM
  continuation on current source and verified inputs. Earlier retained outputs
  are historical evidence only.
- Obtain an ordinary-input Z10 win, replay and SRAM continuation without changing
  objectives, resource balances or success checks to make the tape win.
- Check carried missions, source boundary, default build/tests and unchanged ARM
  memory guards. Late candidates have very little free EWRAM.
- Explain or replace the scalar wave derivation with traceable reference facts;
  distinguish authored approximations from observed N64 behavior.
- Measure ARM timing and keep the 30-fps target separate from a gameplay win.

`PX_DIRECT_LOG` removes the replay probe's private 240-byte line buffer by writing
to mGBA's existing debug-output buffer. It is probe-only, does not write game
world/SRAM, and does not relax production linker reserves.
