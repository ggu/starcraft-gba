# Human automatic target acquisition and fog

A private T9 ordinary-input diagnostic exposed player Wraiths damaging enemy
buildings whose centers were not currently visible. The controller had issued
Move orders rather than explicit attacks. Passive source-owner/type hit records
correlated the damage with human Wraith attacks; the ordinary idle acquisition
query did not filter the human visibility bitmap.

The authored fix adds current-visibility eligibility when choosing a *new*
human automatic target, before the existing weapon/detection/immunity checks.
It covers the spatial index, owner lists and fallback scan, defense-building
candidate paths, and P6 wall-trap acquisition. Non-human owners retain their
existing AI visibility model. Explicit remembered-target handling and released
attack resolution are separate paths and are not globally changed by the
acquisition predicate. Unit, building and save layouts do not grow.

## Reference and limits

The behavioral reference is pinned OpenBW revision
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8`,
[`bwgame.h` lines 1868–1893](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L1868-L1893):
default target acquisition checks visibility independently of attack eligibility.
The local inspected header SHA-256 is
`5ee316ca2c008c9252f7fde7444ca5dc7f35440549be2576dc37fa228fb31ae6`.
No reference implementation code is copied.

This GBA fix uses the engine's existing human center-cell visibility bitmap;
it does not establish complete PC/N64 vision, target priority or combat parity,
and does not add a separate vision model for every AI. A visibility-correct
campaign replay may differ from earlier traces that relied on the defect.
Those earlier traces cannot qualify a new campaign win.

## Qualification status

Work in progress: focused regressions and current-product checks are being
completed locally. No mission is enabled by this change. Three race ROMs,
map selection, all human races and one to three AIs remain the intended product
scope. Full campaign completion and steady 30 fps are not established.

No assets, generated game data, ROMs, saves, captures, private traces or native
decompilation sources are tracked. No Actions are dispatched.
