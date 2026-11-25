# Zerg campaign Ventral Sacs permissions

The authenticated USA campaign scenarios identify the human Zerg as source
owner 3. Ventral Sacs is upgrade 24. The base `UPGR` and expansion `PUPx`
records agree, including their player-default selectors. Reading the optional
expansion record after the base record as in the pinned OpenBW policy gives:

| Mission | Maximum / initial | Global default | Overlord / Lair / Hive allowed |
| --- | --- | --- | --- |
| Z1 | 0 / 0 | no | yes / no / no |
| Z2–Z3 | 0 / 0 | no | yes / yes / no |
| Z4 | 1 / 0 | yes | yes / yes / no |
| Z5 | 0 / 0 | no | no / no / no |
| Z6–Z10 | 1 / 0 | yes | yes / yes / yes |

All ten `UPGS` and `UPGx` records select default price and time for this
upgrade: 200 minerals, 200 gas and 2,400 source frames (3,000 engine ticks).
A PUNI restriction controls production eligibility and does not delete
preplaced units. Source facts derive from the authenticated US ROM and pinned
extractor used by `tools/sc64_protoss_upgrade_policy.py`; the Z4–Z10 CHK
hashes are recorded in `tools/sc64_races_pack.py`. This evidence describes the
source data and OpenBW-derived interpretation, not observed N64 hardware.

Z6 keeps its existing source-bound research state and save representation. Z4
and Z7–Z10 use the shared transport research state; their initial level is zero,
and Lair/Hive research must be paid and completed before an Overlord can load
cargo. Z4 restricts Hive production; that restriction does not itself prevent
an otherwise available Hive from researching Ventral Sacs.
