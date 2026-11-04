# Source contact oracle

This host-only experiment compares the actual P10-vector GBA contact query with the actual leading-edge movement sweep in OpenBW `4b046d5f65302b10cb0a745f0fecd37ec85b20a8`. It does not change production collision, demonstrate actual N64 behavior, or measure a speedup. OpenBW's PC-derived behavior is a source reference, not proof of N64 runtime parity.

The two synthetic actors use the existing authored source bounds for raw 67 (High Templar, left/up/right/down 12/10/11/13) and raw 19 (Raynor Vulture, 16/16/15/15). The runner verifies these against the public [pinned BWAPI unit table](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UnitType.cpp) with SHA-256 `7f16832dac82b8a739a07616ec0e61bc2c68a34b873f779cca8742de4832c76b`. OpenBW positions and movement are exactly twice GBA coordinates; asymmetric source extents remain in source pixels, avoiding unverified rounding to half-pixels. These are two public reference rows, not an audit of every unit or a check of N64 DAT.

| Synthetic case | OpenBW sweep blocks | Current GBA blocks |
| --- | ---: | ---: |
| East approach | 1 | 0 |
| South approach | 1 | 0 |
| Already overlapping, move closer | 0 | 1 |
| Already overlapping, move away | 0 | 0 |
| Initially separated, move away | 0 | 0 |

The source fixture also asserts X-edge selection before Y in a diagonal step, a synthetic equal-edge insertion-order identity, hidden and flying movers, target pathing and no-collide flags, larva exceptions, harvesting and WaitForGas, owner/alliance-dependent collision, and a burrowed candidate. It verifies that a loaded flag alone is not a low-level sweep filter: finder removal is necessary in that synthetic setup. These are direct low-level source checks; they do not establish complete source actor lifecycle, animation or terrain behavior. GBA comparisons currently cover only the five geometry cases above; GBA lacks equivalents for several source status and finder states. The fixtures are not a crowding benchmark.

Reproduce with an authorized prepared P10 tree and captured normal P10 compiler argv JSON, a clean pinned OpenBW checkout, and the unmodified public BWAPI source file:

```sh
python3 tools/sc64_contact_oracle_run.py --openbw /path/to/openbw --p10-argv /path/to/p10-argv.json --bwapi-unit-type /path/to/UnitType.cpp --out /new/private/output
```

The runner validates the OpenBW commit and BWAPI hash and takes `/tmp/sc64-native-build-20261031.lock` with `flock`, checking at least 2 GiB free. It builds OpenBW and GBA independently, executes both host fixtures, and asserts the five paired results. In the recorded run at source `afdf1497`, both completed successfully under ASan/UBSan for the GBA fixture; all 1,958 prepared inputs matched the existing manifest after the run. Results and compiler log are retained locally in `/tmp/sc64-contact-oracle-final`.

A future runtime proposal must first cover every supported raw type's extents and unknown-type policy, then implement bounded swept edge queries with correct eligibility and finder order, including equal-edge identity and lifecycle updates. Preserve exact 2:1 conversion and diagnose how odd source offsets affect GBA coordinates. Terrain contour checks are a separate source subsystem, and OpenBW collision response/slide and pathing state depend on the chosen actor; swapping only a rectangle predicate into the current eight-direction steering is not source-equivalent. Verify a broad independent source oracle, saved-game/reload behavior, campaign and mixed-AI lifecycle, RAM/IWRAM and actual guest cycles before considering a shared runtime change. No such change is made here.
