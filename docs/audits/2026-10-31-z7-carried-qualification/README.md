# Complete Z7 prepared-profile host qualification

The complete `test-zerg-z7` target passed at frozen source
`6d5db14157fdf5791838420955acc44ba596d9a9`. This removes the earlier carried
controller failures for this seven-chapter vector. It does not promote the
canonical Z6 menu, qualify all 30 missions, or establish near-30-fps play.

## Reproduction and input scope

The normal Make target was used with its 109 verified private input entries
and the separately verified walk-word header marked old (`make -o PATH ...
test-zerg-z7`). This prevents Make from regenerating those fixed inputs.
Derived contact and indexed-weapon lookup headers were generated only in
this checkout's own build directory. No reference checkout or pinned input
was modified. Exact arguments and output remain under the ignored local
`.local/qualification/carried-all/` directory.

The run held the shared native compiler lock, required more than 2 GiB free,
and kept tracked sources frozen from compilation through the final checks.
It completed with exit 0 in 251.58 seconds. All 110 input/auxiliary receipt
entries matched before and after; the tracked tree remained clean and the
source revision unchanged. The complete log SHA-256 is
`b6cdeb467736ccd2ce9827ebe319cf0d85d7ca21c5420213fc9c4acfe6608533`.

## Chapter witnesses

All seven controllers reached their source victory conditions. The repairs
affect test strategies and input recording, not the production game engine.
Controllers observe internal world/map state, including hidden information;
they are not pixel-only or general-purpose AI. Selection, orders, economy
and construction actions use game input handling rather than direct world
writes. The host controllers' path queries may warm derived caches.

| Chapter | Final input tick | Retained trace SHA-256 |
| --- | ---: | --- |
| Z1 | 58145 | `54c70ea4509d3907071817375d85a5c71898079e735eb1d0163f1931ea3e46c1` |
| Z2 | 7812 | `1f025d45d4b2e1cc4780bc1b0106f8ad25735db23b7358eacfd3df9e6f523eaf` |
| Z3 | 121979 | `c588e33cdf744e782edfdbfbbdef73f56884b017f133d02708c813b58b601114` |
| Z4 | 47636 | `0328a9858cfefc20b54b3522e771ff73a7519a8126b2b143a0a4636884b72552` |
| Z5 | 48842 | `2319a1f468ddfea9e1a9ec4f744d51e015c2db75c2e0beee6486fe7c22e64f02` |
| Z6 | 119539 | `4c739a68730aa69d54ff930887044bbb8d0c48be1baea32672941676306b1cae` |
| Z7 | 179764 | `5121ed97ea0ea3d57697ce2c7fc57eac2082e3a457bb4ad98517f09ecf510489` |

Z1 cancellation and timeout site checks passed. The Z7 and Z6 generated
traces also passed their target's offline contract validators. The older Z3
scheduler validator retains its historical exact-final-state requirement;
it is not used to relabel the new Z3 trace. Its separate physical-input
qualification matches all 410 retained observations on the unchanged normal
Z7 ROM; see the [Z3 physical audit](../2026-10-31-z3-normal-physical/README.md).
The Z7 trace likewise matches the earlier complete physical-ROM witness.
Other chapters' host wins are not newly claimed as physical ARM replays.

## Profile and skirmish scope

The target also passed 234 startup/save configurations: two maps, three
human races, and every independently chosen AI-race combination for one to
three opponents. Its runtime skirmish matrix passed 18 terminal cases with
rotated opponent races (one human win and 17 losses). Those are terminal
runtime checks, not 234 complete matches or evidence that a passive human
controller can win every setup.

The campaign save, full-clear framebuffer comparisons, Scourge production
and combat, contact geometry, and Z7 last-seen/controller fixtures passed.
The UI-context extension remains disabled in this tested production vector.
Its separate experimental fresh-process continuation is documented in the
[save-context audit](../../Z7-SAVE-UI-AUDIT.md).

## Remaining limits

This is source-scoped host qualification with sanitizers, plus the separately
scoped normal-ROM Z3 and Z7 button replays. It does not replace full SRAM
continuation and hardware checks for every chapter. The carried Z6 controller
is not yet qualified on the different canonical Z6 vector; a controller-policy
compatibility follow-up is separate work.

The complete normal Z7 witness measured about 6.79 emulated main loops/s;
Z3 measured about 18.33. These include presentation and UI and are not
physical-device FPS. Performance remains a release hold. No canonical menu,
SD package, hosted Actions setting or actor capacity is changed here.
