# Common Egg release qualification — 2026-11-06

This is the **working candidate for draft #298**, based on merged #297. It is
not a claim that normal activation has shipped, S1 is complete or all campaigns
win. The common-selector runtime is `6357206f`; product/controller alias and
fixture successor `bafc1f22` supplies the release vectors. Combined publication
`95b30e10` also includes the physical P10 probes and qualified Z4 controller fix.

## Capability and binary matrix

One `SC64_SHARED_EGG_SERVICE` selector requests the existing authenticated late
context for Z9/Z10 and the custom context for Terran, Z7, P10 and Showcase.
Normal ROM and carried campaign controller recipes use product aliases with
that selector. Fact generators stay separate; P10 builds Egg-aware facts in a
new directory without overwriting its legacy pinned input. Explicit old preview
commands retain their isolated contexts.

All six normal product ROMs built at `bafc1f22`, passed standard headers and both
lower/upper RAM reserve guards, and match the corresponding earlier qualified
preview ROM byte for byte:

| Product | SHA-256 | EWRAM end / IWRAM end |
| --- | --- | --- |
| Terran | `a7975958e1af1ba74c3025183bf019f0f9278442812aa0ceb8c215e44c1db03b` | `02036f1c / 030048a8` |
| Z7 qualification | `9779755a7c5343a6574682b28ee7e40d5090ded03211770b3560282a26a89860` | `0203d474 / 03004f88` |
| Z9 delivered vector | `459a7f254530e6a5ef388f3a092b30fc728b43a7f38aed95d24009cb95a83995` | `0203dce4 / 03004bf8` |
| Z10 candidate | `861b6e50a56d127332092fd11e4c10cf00fe61e07a66bd414ccf6599f2d55c19` | `0203dce4 / 03004bf8` |
| P10 | `562b6477e98457ad8333f8fd5b7d4b2ac6ebc99205137a53832139c8c19f9428` | `020391e4 / 03004b98` |
| Showcase | `4b5cc84a552bcaf481a408e85c55c2445243fdbbe3f558d81826582ffa720075` | `02036d64 / 03004cb0` |

These are unchanged context binaries, not new speed results. Z9 has only 796
bytes EWRAM linker slack; Z7 has 120 bytes IWRAM linker slack. Neither is a
measured stack depth. The [earlier actual ARM Egg/SRAM proofs](../2026-11-06-current-native-qualification/late-eggs.md)
retain their exact product/mission scope through the matching ROM bytes.

## HOST gameplay and compatibility

Sanitized ordinary custom-game input fixtures pass across all six product
vectors: select, pay, save, cancel, load and birth. Common-selector journal,
legacy raw-36 Z7, P10 custom, paid capacity and deferred CreateUnit continuation
checks pass. Same-Z9-product older-code HOST records for Z6–Z9 restore through
normal/common readers; malformed pairs and newer-invalid/older-valid fallback
are checked. Constructed HOST journals were never imported into a guest.

All T1–T10 and P1–P10 ordinary-input campaign controllers pass with the actual
product selector. Zerg carried Z1, Z2, Z3, Z5, Z6, Z7 and Z9 also pass. These are
HOST controllers, not physical GBA wins. Z4 was blocked by obsolete fixed
five-product selector arithmetic. The controller-only fix follows the actual
availability-filtered picker after selecting its producer, with a bounded
visited-type guard; it wins on Z9 at frame 35,537 and Z6 at frame 48,941.
No resources, actors, objectives, strategy or run limit were changed for that fix.

**Z8 remains the release gate.** The common and explicit late-preview controller
lose at frame 11,048; service-off also loses, at 21,782 with a different trajectory.
Do not call those trajectories equivalent. Targeted read-only raid/defender
telemetry and ordinary-command trials are investigating detection continuity,
production responsiveness and defensive firepower. Unsuccessful experiments
remain private and unqualified; no enemy or mission-condition changes are used
to force a win. Z10 still has no ordinary win and remains the unproven 30th
mission, rather than a condition invented for the delivered Z9 content.

Thus content coverage stays 29/30, while this candidate's current ordinary
controller evidence covers **28 wins** after the Z4 fix. Neither figure proves
complete N64 mechanics, all saves, all guest campaigns or hardware performance.

## Actual early P10 saves and migration

On the existing normal P10 ROM and the new selector ROM, all P1–P7 save points
at frame 31 pass ordinary physical-button save, actual SRAM export, fresh HOME-L
load and neutral continuation. The corresponding old actual SRAM also loads in
the new product. The extended matrix reaches frame 1,031 for old, new and
migration in each mission: 21 passing cases, each matching its uninterrupted
reference and preserving the 32 KiB SRAM bytes. Shorter frame-151 checks pass too.

Compared named fields include frame/mission, live units/buildings/queues,
resource banks/fields, supply, `race_campaign`, P3–P7 persistent structs and P7
projectile storage. The parent reviewed the portable probes under
[tests/sc64-p10-early-save](../../../tests/sc64-p10-early-save/README.md).
Input artifact hashes are checked before/after; the normal controller transport
and read-only VRAM modal observations do not write guest world state.

A preliminary probe compared transient world contexts as if they were saves.
The documented differences are not silently discarded: P5's unused objective
refresh request byte is distinct from its persisted objective stage; P6/P7
elapsed caches are overwritten each active tick; P7 count snapshots are rebuilt
and deactivated each tick. The corrected probe compares persisted mission
state and subsequent named state. These are finite early neutral prefixes,
not arbitrary saves, complete codec coverage, active battle effects or wins.

## Default checks and retained failures

At combined head `95b30e10`, source boundary (1,732 indexed text files), doctor,
`./dev build` and the complete `./dev test` pass. The main Python suite has
1,000 tests with 26 skipped; compiled/auxiliary checks also pass. The final test
command completed in 424 seconds. Earlier 180-second timeout and stopped retry
logs are retained and are not counted as complete suite passes.

All 1,959 pinned game-input hashes remain unchanged. Native work is serialized
under the existing lock and output/disk reserves. Only closed owned HOST
executables were losslessly archived with SHA-256 roundtrip/open-file checks;
ROMs, saves, inputs, logs and traces were preserved. No Actions run was used.

## Rejected performance experiment

The separate opt-in detector-first prototype `a5226e8d` is **not included**.
Its independent owner-mask and lifecycle oracles pass, but the exact normal P7
pair regresses from 2,200,225.76 to 2,202,589.45 cycles per advancing full loop
(+0.1074%). Peak changes from 3,918,363 to 3,915,972 cycles at call 8347. All 90
sampled state/pixel checks, 958 active updates and three modal acknowledgments
match; pinned inputs match before/after. A historical optical-loop upper bound
was not a saving. Do not promote this candidate or label it a speed improvement.

Next performance work distinguishes the route-flood/blocked-gradient work at
the movement spike and reduces measured collision/steering cost with exact
oracles. The bounded movement diagnostic is prepared but has not run; its plan
is not new runtime evidence. No uniform actor-update skip is justified.

## Remaining work

Resolve Z8 through ordinary controls before common rollout, then qualify the
combined final source and delivered package. Z1–Z6 independent Egg producer,
owner/save contexts, Greater-Spire Guardian/Cocoon, animation pulse/birth/shadow,
stack, full custom matches, broader campaign saves and near-30-fps behavior
remain unfinished. The [living backlog](../../PROJECT-TODO.md) retains the full
goal; this candidate does not redefine completion around the checks above.
