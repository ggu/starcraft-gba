# Packed unit sight rows in the shared P10 path

The opt-in `SC64_SHARED_SIGHT_PACKED_ROWS` probe reuses the existing late
Zerg mathematical LUT for the shared source-sight transition. Supported
nonnegative source positions use the packed row decoder. Negative
positions, other radii and legacy sight keep the scalar path. Both
shared paths apply the same removals and additions in the same row
order. The probe changes no persistent data.

The asset-free ASan/UBSan full-grid oracle passes for all existing
variants and the new shared packed variant, checking visibility,
exploration, source counts and minimap occupancy. The candidate also
matches scalar row results for every phase of all eight radii at two
opposite map corners, including empty and clipped rows.

The normal P10 candidate replays the first 43,000 physical calls from
HOME against the normal KODAK cartridge. All 17 host checkpoints, ten
briefing records, seven authored page acknowledgments and 32 ordered
video/state snapshots match. Those snapshots include the shown page,
palette, unit, building and mineral pools, fog, cursor and route state.
The trace stops before victory and does not qualify SRAM continuation
or physical hardware.

| Scope | Control cycles | Packed cycles | Change |
| --- | ---: | ---: | ---: |
| 43,000-call mean | 1,879,249 | 1,845,887 | −1.78% |
| Calls 5,001–6,000 mean | 1,637,408 | 1,626,226 | −0.68% |
| Calls 24,001–25,000 mean | 2,060,827 | 2,021,839 | −1.89% |
| Calls 42,001–43,000 mean | 2,208,690 | 2,141,496 | −3.04% |
| 16 matched ordinary instruction samples, whole loop | 1,759,540 | 1,713,563 | −2.61% |
| Same 16 samples, exploration | 178,302 | 132,476 | −25.70% |

The five late instruction samples improve 4.41% in total, including
26.77% less exploration work. The full prefix reaches 9.09 emulated
updates/s, up from 8.93. The gameplay peak stays near 45.25 million
cycles. This addresses steady sight cost rather than route stalls.

The normal ROM verifier passes. Text increases by 51,920 bytes, the
BSS end stays at `0x0203c0e0`, and the IWRAM end rises 40 bytes to
`0x03004ad8`. The existing guard is intact, but this version does not
meet a no-growth IWRAM requirement. Keep it off pending placement work,
a broader cartridge matrix and full exact replay.

- Candidate ROM SHA-256: `8eb188562cbee61c9df31e84e4ca9106235c23225b521af5bccec47de799d198`.
- Control receipt SHA-256: `d8b2919ddc66671328f2932041f85484268258acba1927784f8ab1407a8b1f47`.
- Candidate receipt SHA-256: `c636bf7142cbc11827c03a20030aa5d42c50005182cac6a817387706455efe79`.
- Matched ordered video/state receipt SHA-256: `aa16bad817241888dc4c7184e9773ecfe2efc28d727bd5d0aed1f928dab1752a`.
