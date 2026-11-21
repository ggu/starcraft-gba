# Attack-query cost outside active Z5 cloak

The regular Z10 compiler inlined the Z5 detector scan into its common
attack-eligibility routine. The inactive-cloak path still paid a
128-byte stack frame. Keep the detector walk out of line. With the
prepared Z10 inputs, the common routine uses 24 stack bytes and
shrinks from 1,264 code bytes to 380. Calls with active Z5 cloak use
the same detector scan and live eligibility rules.

All four normal ROM and RAM guards pass. The Terran, Protoss and
Showcase artifacts match release `71809328` byte for byte.
The normal Z10 build has EWRAM end `0x02039870` and IWRAM end
`0x03004f60`.

The 3,600-update three-CPU Blood Bath replay matches all 31
world and indexed display cuts, 16 fresh HOME continuation cuts,
all route rows and SRAM. Mean guest cycles fall from 821,028 to
819,722 (0.16%), and p95 falls from 1,110,401 to 1,108,653.
The matched 1,200-update saved Z8 continuation averages 906,851
cycles on each build.

The prepared sanitizer fixture runs the ordinary attack predicate
with active Z5 cloak, an enabled detector in range, destruction
and the disabled detector. An ordinary Z5 HOME load at frame 900
has active cloak. The two ARM builds match through frame 1,020,
including 960 Q8 energy spent. They write the same pause journal
and restore the same active cloak and world from a fresh process.
The 54 normal Custom Game pause and fresh HOME cases pass.

On the rendered Z7 tape, ordinary saves at controller calls 3,038
and 7,486 match byte for byte. Fresh HOME, followed by A to restore
the selected Hatchery on the original neutral update, reaches
gameplay frame 9,438 on both builds. The two cuts match 18 and five
subsequent named observations, respectively.
[Rendered input and tape provenance](2026-11-22-z7-rendered-replay.md)

The retained Z10 final-chapter tape still writes its ordinary
frame-2,526 journal on both builds. Its first divergent historical
observation is call 8,953, and the trace restarts at call 9,053 on
both builds. These are input-tape observations. A fresh winning
trace remains a separate campaign qualification.

| Local evidence | SHA-256 |
| --- | --- |
| Three-CPU Blood Bath comparison | `d90d6e4269ef055c92c348e1cce08a6598d13abbacac6f1c188fe034da2fcc99` |
| Z8 paired continuation | `53cf38895c3830588e9c22837caeb4f0d0249db20f042c1c3ed250d031afc260` |
| Z5 active-cloak control | `823d7d9afeb9bd01e3d829c866b508397eb43829d538abd38162ddffcf65537c` |
| Z5 active-cloak candidate | `3479ebebdcbfd31ecd1eebd63981546ecfd7b8c16a291bd99874b1e3d0c56230` |
| Custom Game fresh HOME matrix | `5bb9402da5e8b13e40b79332a306144386ba413cac5a4db3b1d8a5543c788e1f` |
| Earlier Z7 fresh recovery on the candidate | `80019871dc2c14f16f5d362a0ce92985ba5f40b33f5f7cc2b7f310562fa8b89f` |
| Later Z7 fresh recovery on the candidate | `7fc4d97c5b2360f0d7166f9fee9a62c4f0a150c2feabcfb4afaa7eb5020ae915` |
