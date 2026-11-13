# Normal campaign cartridge qualification matrix

This is an index of complete normal-ROM mGBA replays known on November
13. A win on the exact KODAK ROM is separate from a win on a preceding
normal ROM, a host controller win, or a prefix. No row represents a
physical GBA check or all routes through a mission.

| Mission | Exact November 13 KODAK ROM | Closest other evidence or next check |
| --- | --- | --- |
| T1–T2 | [HOME wins](2026-11-13-t1-t2-normal-physical.md) | Fresh SRAM and hardware still separate. |
| T3–T4 | [HOME wins](2026-11-13-t3-t4-normal-physical.md) | Fresh SRAM and hardware still separate. |
| T5 | [HOME win](2026-11-13-t5-normal-physical.md#exact-november-13-kodak-cartridge) | All 150 observations matched and both heroes survived. |
| T6 | [HOME win](2026-11-13-t6-normal-physical.md) | Fixed the controller's direct menu write and route observer. |
| T7 | [HOME win](2026-11-13-t7-t9-normal-physical.md) | All 15 checkpoints matched. |
| T8 | [HOME win](2026-11-13-t8-normal-physical.md) | All 272 checkpoints matched. |
| T9 | [HOME win](2026-11-13-t7-t9-normal-physical.md#t9-new-gettysburg) | All 51 checkpoints matched. |
| T10 | [HOME win](2026-11-13-t10-normal-physical.md) | Raynor alive; Ion Cannon destroyed. |
| Z1–Z2 | [Z1 HOME win](2026-11-13-z1-normal-home.md), [Z2 HOME win](2026-11-13-z2-normal-home.md) | Briefings and authored pages acknowledged. |
| Z3 | [HOME win](2026-11-13-z3-current-home.md) | All 465 observations also match on staged `c906058`; Chrysalis retains all 250 HP. |
| Z4 | [HOME win](2026-11-13-z4-two-normal-roms.md) | All 42 checkpoints also match on staged `c906058`; Kerrigan survives. |
| Z5 | [HOME win](2026-11-13-z5-kodak-replay.md) | All 55 checkpoints matched, with the source pause and Kerrigan alive. |
| Z6 | No complete exact-ROM receipt here | [HOME win](2026-11-12-z6-normal-physical.md) on a preceding normal Z10 ROM. |
| Z7 | [Fresh SRAM win](2026-11-12-z7-normal-physical.md#fresh-sram-on-the-november-13-z10-kodak-cartridge) | Ordinary HOME reached the late save cut on the exact ROM; reselect the Hatchery after load. Full uninterrupted HOME win on a preceding Z10 ROM. |
| Z8–Z9 | No complete exact-ROM receipt here | [Z8](2026-11-12-z8-normal-physical.md) and [Z9](2026-11-13-z9-normal-physical.md) win on a preceding normal Z10 ROM. |
| Z10 | [Fresh SRAM win](2026-11-13-composed-performance.md) | Both normal ROMs in the comparison won from the same SRAM. [HOME win on the ARM test entry](2026-11-12-z10-arm-qualification.md). |
| P1–P6 | No complete exact-ROM receipt here | [Host and replacement-loop wins, earlier normal save prefixes](2026-11-12-normal-campaign-replay-gaps.md). Rebuild stale P1 input and isolate controller route queries for P3–P6. |
| P7 | [HOME win](2026-11-13-p7-carried-normal-home.md) | All 59 named snapshots also match on staged and nearest-first P10; all three heroes survive. |
| P8 | No complete exact-ROM receipt here | [Preceding normal P8 win and SRAM continuation](2026-10-31-protoss-cannon-lazy/README.md); verify the carried P10 route. |
| P9 | [HOME win](2026-11-13-p9-normal-home.md) | All 11 briefing pages. |
| P10 | [HOME and fresh SRAM wins](2026-11-13-p10-normal-home.md#exact-kodak-cartridge) | Full replay averaged 8.79 emulated updates/s; below the performance target. |

All ten Terran missions now have a checked HOME win on the same exact
cartridge. The exact ROMs in this index are the Terran, Z10 and P10 cartridges
copied to KODAK under runtime `cbba1c5`. Tests on previews or composed
performance builds do not substitute for the normal cartridge row.
