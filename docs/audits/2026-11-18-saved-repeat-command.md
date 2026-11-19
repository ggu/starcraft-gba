# Preserve the repeat command when loading a race save

Press Start to open pause, then B to save. The title's L shortcut loads the
game in a fresh process. The selected building and menu highlight clear.
The command menu stays closed and the existing unit-selection fields
keep their format-specific behavior. With a unit selected, B highlights
the remembered command if its row is available. A confirms that command.

The normal race writer uses the existing tagged word 31. It stores only the
last command in bits 16–23. The selected-building and highlighted-row
fields are zero. Writing the default command clears the entire word,
including when the writer reuses its buffer. Legacy zero words load the
default command. The normal reader accepts the existing full-context tag
and ignores the transient fields. The opt-in full-context profile keeps its
own selection and highlight behavior. The same bounded tagged-word check
runs during preflight before applying the candidate.

## Outpost physical replay

Terran defeated one Zerg CPU using the retained Outpost controller tape on
the normal `8c15f275` build. All 95 observations matched. The replay ended
at simulation frame 41,951 after 42,257 physical input calls. Terran had 22
units, eight buildings, 362 minerals and 1,846 gas. The full pass averaged
614,714 emulated main-loop cycles per call (27.29 updates/s), including
presentation and UI waits.

A Start/B save at call 18,626 (frame 18,379) kept 68 live units, 19 buildings,
cursor, camera and field mode identical across an independent mGBA boot.
Sampled gameplay also matched through call 18,746. At call 18,789 the retained
tape had issued Attack Move to 22 units. Each loaded unit instead had Move.
The tape opened B then pressed A. The old build highlighted Move, while the
uninterrupted session highlighted the last used command, Attack.

One Down press after B corrects the old build's text menu while gameplay
is paused by the menu. With that extra press, all 56 remaining action
checks matched and the map finished on the original victory frame. The
candidate restores the Attack row after the fresh load. Its unchanged
tape matches the same 56 action checks and the same victory. Victory
commits its normal new journal bank.

## Journal and build checks

All four ARM products pass the existing ROM/header and RAM link guards.
The ASan/UBSan fixture uses the ordinary lobby before checking the production
writer and reader for each of three human races in every product (12
cases). It verifies the zero and tagged words, a closed menu and empty
selection after reload, and the repeat row reopened by B.

| ARM product | Map | Human | CPU seats |
| --- | --- | --- | --- |
| Terran | Blood Bath | Terran | Zerg / Protoss / Random |
| Z10 | Outpost | Zerg | Closed / Closed / Protoss |
| P10 | Blood Bath | Protoss | Terran / Closed / Zerg |
| Showcase | Outpost | Zerg | Protoss / Terran / Zerg |

All four own-writer/reader pairs restore the remembered command and clear
the selected town hall and highlighted row. Saved and loaded worlds match,
as do the next 180 ordinary updates. Terran also passes four cross-loads.
The previous regular reader accepts the new save and opens its default
command. The new reader accepts the previous regular writer's legacy zero
word. Both directions with the prior full-context preview recover the
remembered command from the same tag. The new reader ignores the full
writer's nonzero saved town hall and highlight. SRAM bytes match the
input after each of the eight load and continuation checks.

`make test-sc64-repeat-command` runs the four prepared HOST fixtures.
The Outpost replay exercises one human skirmish win on a normal
cartridge and one mid-match save. The sampled ARM builds cover both maps
and multiple opponents. A human Blood Bath victory and GBA SP timing are
still open. The N64's remembered command value remains unverified. The
[native T1 title-load capture](2026-11-13-n64-t1-save-selection.md) shows
the selected Command Center clearing.

The regenerated Wasteland controller also wins on the candidate's normal
Terran ARM build. An ordinary pause save, restart and load keeps the
gameplay state at frame 6,214 and restores last command 1 with an empty
selection and a closed menu. The saved highlight is 1 and the loaded
highlight is 0. All ten recorded checkpoints match the HOST controller.
The win is at frame 10,120 with Raynor rescued, ten Marines and five SCVs.

| Receipt | SHA-256 |
| --- | --- |
| Outpost input tape | `ddc541b30061fd0f2c3292bf96f56d7c1fb5a87aa7427a60637deddc2a74c43a` |
| Normal whole match | `cfc0cad617b63b5ad63265ea61e7a25fa9bc6fc71e131013253342b2959c799a` |
| First sampled resume divergence | `263d8462466d333a69f4499dc4db4ebf701c02248d3b4aaa387385bd494f907c` |
| Normal fresh load with Down correction | `402d5d3ac87cc4962912110fff89df1abecca41254bd69f840ab4dd3bfb524e8` |
| Candidate fresh load through victory | `457846e7e6eb2a9d7038864236e9dd75814779d7a26dbf7abf4a10937dc695d8` |
| Four-product and cross-load ARM matrix | `040e0bf33c516a1dbf38ed5b0a2f4d1ad3b6935432e915504cc60d873e0b2bc3` |
| Wasteland ARM save/restart/load and victory | `bd4e45c8309dc4ca8d576c35abdb6f255ad0c3871446b5c96477e7dbf5942a8d` |

## Composed canonical builds

The four canonical builds with the terrain and defense defaults pass the
same ARM memory guards and 12 sanitizer cases. The Terran build repeats
the unchanged Outpost win after a fresh load at call 18,626. Its 56
remaining action checks and terminal frame 41,951 match. All eight ARM
cross-load cases also pass on the composed cartridges.

From the title screen, physical controls launched all ten chapters in
each of Terran, Z10 and P10. The same builds and Showcase completed 72
skirmish openings covering both maps, every human race and each opponent
count. Each opening checked the selected CPU races, start positions and
at least 100 play updates. Campaign checks cover launch and opening play.

| Composed receipt | SHA-256 |
| --- | --- |
| Outpost fresh-load win | `c7d3e7639843f335be956f1dd5c231af5ef3525db503f111e3623e5bc0995d55` |
| ARM load matrix | `dea5eb6b088e6d1790dd1fe3ae8c67559b91acc10ff294d6cf11c597976a1722` |
| Physical campaign and skirmish openings | `f8644030ebaa75ae0ea73e128b591e09757f1b6367c5bd96da5cf44bf7e86922` |

## Early Protoss campaign continuation

The normal P10 build with repeat-command persistence and the verified
target-index options wins P2 and P3 from the same ordinary pause cuts
that previously diverged. Both use the byte-identical archived input
tapes after title L in a fresh mGBA process.

| Mission | Save call / frame | Victory frame | Matched checkpoints | Briefing / field pages |
| --- | ---: | ---: | ---: | ---: |
| P2 | 440 / 372 | 33,803 | 36 | 14 / 6 |
| P3 | 24,581 / 23,993 | 45,951 | 47 | 9 / 3 |

P2 restores six selected unit slots before continuing and wins with the
hero alive. P3 matches its saved world, keeps last command 1 and clears
the menu highlight. Both hostile structure banks are gone at its win.
The original input journal stays byte-identical in both tests.

| Local evidence | SHA-256 |
| --- | --- |
| P2 pause and fresh load to victory | `957883f4a9c86e5b73f5bc93d301cfe59f522d5ef6b78ac100e4f9cc5cde0463` |
| P3 pause and fresh load to victory | `784776b6ec4f7e92309b07aa7414bbef78932f273a9f73468a2b7401f62a1298` |
