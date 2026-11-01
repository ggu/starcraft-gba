# Z3 normal-ROM physical-key replay

The frozen driver at `c1e1a8d` replayed the Z3 controller trace from
`7325b33` through physical emulator button inputs on the unchanged normal Z7
ROM, not a probe or debug-scheduler ROM. The 1,000-update prefix passed four
retained host observations. The fresh full run reached source victory at
host tick 121979, native frame 120346, with 410 retained host observations
matching: Z3 model and campaign fields, unit and building counts, economy,
Chrysalis HP, cursor and selection. The final Chrysalis HP was 250; three
hostile units and three hostile buildings remained, satisfying the source
victory condition. This compares named fields, not entire entity arrays.

Inputs were pinned to ROM SHA-256
`59bd05448a2a579e6addd54cd37809a5678371b38feee0413bf9d52fff052b29`,
ELF `f8fe62b4006c47fe521fddab7fc43c63490e11a9783957de9f5a24c93168a281`,
physical runner `95b3a8ccf0d2a04b7820366ee484012912e2657acb5bddf940ce473c1f06a18b`,
and trace `c588e33cdf744e782edfdbfbbdef73f56884b017f133d02708c813b58b601114`.
The trace, ROM, ELF, runner and driver dependency hashes were unchanged over
the run. The physical backend writes button masks to the emulator, does not
write guest scheduler state, and stops on the shipped game-update counter.
Read layouts and widths came from the retained ELF's DWARF and were checked
against symbol sizes and GBA memory bounds. In this ELF compact Building HP
is four bytes; a unit test also checks a two-byte HP layout with adjacent
nonzero bytes to catch a hardcoded-width read.

The shipped main-loop counters recorded 121979 updates and 111662186005 guest
cycles, a mean of 915421 cycles and 18.33 emulated main-loop Hz. This includes
presentation waits and UI and is not a physical-device frame-rate measurement.
The result and all matched observations are retained locally at
`/tmp/z3-physical-full-frozen-20261031/result.json`; the prefix result is at
`/tmp/z3-physical-prefix-frozen-20261031/result.json`.

After that full run, `04f9a1b` hardened the parser to reject JSON boolean
`true` as schema 1. The actual numeric-schema trace produced exactly the same
full plan, 531 pages and 410 observations under the old and hardened drivers;
the hardened driver separately passed a fresh 1,000-update physical prefix at
`/tmp/z3-physical-prefix-schema-20261031/result.json`. The full run was not
repeated after that parser-only guard. Unit tests cover invalid trace fields,
key transitions, page boundaries, memory bounds, per-field comparisons and HP
width. The historical Z3 scheduler validator remains unchanged and retains its
old exact-final-state contract. This does not qualify other missions, another
ROM, save/resume, full world arrays or hardware performance.
