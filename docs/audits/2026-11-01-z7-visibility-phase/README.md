# Exact Z7 source-sight phase rejection

For supported source-sight radii, the engine can skip a nearby visibility
transition when both positions are in the same center tile and their exact
unbounded cell-center footprints are equal. The per-radius phase classes are
constructed using integer circle geometry; equality remains valid after any
map clipping. Negative coordinates, different center tiles, unsupported radii,
and non-source sight retain their existing paths. Z7's canonical flag enables
this for Z7 and inherited Z8; other product flag vectors are unchanged.

A separately gated Z7 build passed the independent direct squared-distance
phase-class oracle and the existing full-grid source-sight oracle, including
its gated variant. An ordinary physical-button Z7 Transport1 prefix exported
its own save, then a fresh process replayed the strict full continuation. All
302 retained continuation observations and the prefix observation matched the
56fc55f7e2c27d5529d8271e6ea4a914b916d8d3a4ab590229c3cbc4f8f2707b
baseline. The ON ROM was
8c61048b47df6d6de9952e61e9bb09871d238877760ca7c3b8d6e5d32b039dba.
Across 179,753 continuation updates mean emulated main-loop cycles including
presentation and UI decreased from 2,186,610.86 to 2,158,730.32 (1.28%);
peak decreased from 53,931,783 to 53,713,860. This is not physical-device FPS.
The corresponding gated link used no additional BSS and retained IWRAM end
03004f80 under the existing 03005000 boundary.

The full Z7 physical proof applies to the normal ROM because its hash matches
the ON hash above. The activation qualification is recorded below.

## Activation qualification

The activation source at `e7961ec` passed source-boundary, doctor, default
build, and the clean unprepared default suite: 932 Python discovery tests
passed with 26 skips. The preceding focused `c7a9005` source differs only in
its subsequently corrected onboarding test expectation; runtime and Makefile
contents are identical. Its normal Z7 ROM exactly matched the tested ON ROM
`8c61048b47df6d6de9952e61e9bb09871d238877760ca7c3b8d6e5d32b039dba`.
The inherited normal Z8 ROM was
`5696e42d69a93ada0be92c5f9f2a4273853719df6947ea1f02870e5d889815c8`;
its source-sight host oracle passed, with BSS end `0203dbd8` and IWRAM end
`03004bf0`, within the unchanged guards. The Z7 full-grid and independent
all-phase oracles also passed on the focused activation source.

The fresh ordinary physical-button prefix matched one named baseline
observation, and the full fresh-save continuation matched all 302 named
baseline observations. The full mean reduction was 1.275057% and peak
reduction 0.404072%. The ON continuation result SHA-256 is
`9d79c123676e092cafd706d82eef8f4ce18355f0bc3f87e6e5f0d84fb75eb7da`.
These observations cover named state and do not establish a physical-device
frame rate.
