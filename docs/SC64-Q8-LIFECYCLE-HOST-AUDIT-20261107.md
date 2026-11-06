# Q8 actor lifecycle experiment — 2026-11-07

This source-only experiment remains HOST-only and explicitly refuses the active
research journal. It does not enable fractional combat in normal ROMs or
provide persistent HP fractions. Its actor accessors read and commit Q8 HP and
shields; callers still own death callbacks and other world effects.

For eligible Terran buildings, the existing low byte of `shield_remainder` is
the single authoritative HP deficit for both burn and weapon damage; its upper
bits retain damage credit. Other actors retain the experimental HP-debt array.
Non-shield actors report zero shields and a zero shield commit is a no-op; this
must not reinterpret trap cooldown, energy, burn, or other field aliases. The
weapon resolver updates the fraction separately while the existing damage
core applies the whole-HP debit and owns death effects.

Construction growth now adds its existing integer gains to the Q8 HP/shield
values and clears fractional residue at the full cap. The existing morph rule
adds the source/target maximum-HP difference in Q8 and retains its minimum and
maximum clamps. Late Terran repair accepts a unit whose ceiling HP equals its
maximum while the Q8 debt is nonzero; building repair uses the existing burn
debt. The Q8-only T6 and late-Terran paths keep a fractional deficit when a
whole-HP step reaches the ceiling, and clear it when the remaining fractional
repair is completed. The established integer repair pricing and timing are
still an approximation.

Z5 campaign Zergling and Infested Kerrigan regeneration retains the existing
pinned-source rate of 4 Q8 HP per 24 Hz source update, represented as 16/5 Q8
per 30 Hz update with a 0..4 residue in the existing non-shield phase field.
This changes the HOST-only representation; the experiment does not read or
write legacy journals. Exact native regeneration/damage subframe ordering is
not established. The ordinary non-imported worker integer-regeneration path is
outside the tested P10/Z9 imported product vectors.

At runtime/test commit `0f796487b4f09d5da03a2f07ad01ee6b3a3d24a3`, one
serialized, 4 GiB-guarded run passed with ASan/UBSan, 593 verified product
inputs and an independently hashed private Scarab fact header unchanged:

- Existing actual P10 Q8 kernel fixture, including the separate Scarab probe.
- P10, Z9 and Terran product-vector lifecycle fixtures for interleaved weapon
  hits and building burn, source-owner preservation, non-shield alias access,
  damaged Nexus construction saturation, and a damaged Lair morph.
- Z5 Zergling fractional regeneration to full and late-Terran Vulture and
  building fractional-only repair, each on its corresponding product vector.

The local receipt and commands are `.local/q8-lifecycle/result.json`; logs are
`kernel.log`, `p10.log`, `z9.log`, and `terran.log` in that directory. These
are constructed HOST states, not normal-input play, ARM memory/stack evidence,
N64 timing, or a complete actor-lifecycle qualification. P4/P6/Z6 Storm still
requires separate integration and tests. The ordinary shared Egg/Guardian
in-place morph stages and possible fractional-health-dependent selection
remain separate qualification work. No generic Medic healing route was found
in the inspected engine; this observation does not establish parity for an
unimplemented ability.
