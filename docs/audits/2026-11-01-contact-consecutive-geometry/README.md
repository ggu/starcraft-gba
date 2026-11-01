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
