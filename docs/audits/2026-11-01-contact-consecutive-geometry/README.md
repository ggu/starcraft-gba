# Consecutive contact geometry candidate

`SC64_CONTACT_CONSECUTIVE_GEOMETRY` is an opt-in prototype. Each uncached
mask query remembers the immediately preceding validated effective radius and
offset on its own stack. After all original filtering and fallback checks, it
skips a repeated nonzero-offset geometry application when speed is nonzero.
The operation only adds bits to the mask, so repeating it cannot add another
bit even if a reference fallback has added bits in between. Speed-zero and
zero-offset behavior, ordering, the global memo and lifecycle hooks remain
unchanged. No persistent state or new spatial index is added.

A separate count-only current Z7 diagnostic found 215,542 consecutive
repeats among 311,357 eligible geometry applications in the physical-button
50001–50900 window, with 135 retained named observations and nine sampled
visibility/frame/palette checks matching its source-matched baseline. Those
counts establish incidence, not a production cycle gain; the diagnostic build
is not this candidate. This opt-in candidate needs focused differential tests,
unchanged normal OFF ROM identity, normal memory guards, and a paired current
physical window before any enablement or performance claim.

The focused fixture uses type -1 as an undescribable reference candidate only
where the unit type field is wide; packed byte-type profiles do not claim that
case. The Scarab-specific fixture branch runs only in configurations that
actually define `SC64_SCARAB_RUNTIME` and must be separately qualified there.

## Focused qualification and bounded result

At `783ee09`, OFF/ON differential fixtures passed for Races, P7, Terran,
Nydus, Zerg, Protoss and the canonical Z7 table vector. Their oracle reports
matched; the canonical Z7 ON fixture checked 637,136 indexed masks and 130,670
inactive masks across 71,716 mutations with zero failures. The wide-storage
fixture checks an undescribable candidate; packed-unit invalid classes and
Scarab-runtime-specific paths were not established by these profile runs.
The normal Z7 and Z8 OFF ROMs matched their current baselines. The gated Z7
link ended at IWRAM `03004fd0` and BSS `0203d718`; Z8 ended at IWRAM
`03004bf0` and BSS `0203dbd8`, within the unchanged guards.

A paired physical-button ordinary-boot experiment compared the normal current
Z7 baseline (`8c61048b47df6d6de9952e61e9bb09871d238877760ca7c3b8d6e5d32b039dba`)
and this candidate (`388103809ee228e99d2edb2a6d768dc4629f18cd0de2665bc3692f6cabb39043`)
over updates 50001–50900. Both passed 135 retained named observations and
matched nine checkpoint comparisons of all described Unit/Building scalar and
fixed-array fields, visibility source counts, minimap and visibility grids,
and active framebuffer and palette. Baseline mean/p95/window maximum were
2,030,413.882 / 4,155,222 / 8,926,169 emulated main-loop cycles; candidate
values were 2,030,729.484 / 4,157,450 / 8,926,510. Mean rose 0.0155%, p95
rose 0.0536%, and window maximum rose 0.0038%. The comparison result SHA-256
is `448e4bc0b590f643e3e16238548694a194b601bf8050b7c4750bf8c0e69ab837`.
These cycles include presentation and UI and are not physical-device FPS.
The candidate showed no measured benefit in this window and remains disabled.
