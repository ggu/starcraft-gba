# Experimental source-ordered Z10 attack requests

The existing Z10 candidate dispatches fixed-size attack groups for each town.
The authenticated static AI program report instead contains ordered `attack_add`,
`attack_prepare`, `attack_do`, `attack_clear`, waits and backward loops. For
example, the first Z10a request is one raw-65 Zealot; Z10d first requests two
raw-70 Scouts followed by one raw-65 Zealot. Those are decoded program facts,
not observations of native groups arriving or proof of how `attack_prepare`
waits for production and readiness.

The separately gated `z10-ordered` candidate generates a private bounded table
from the SHA-256-pinned authenticated report, preserving request type order,
counts, waits and the backward loop. The packer rejects ambiguous attack state,
unknown attack commands and unstable loops. Its existing 5/4 source-wait to
engine-update conversion remains a provisional approximation. Production, home
defense and attack-move logic remain authored and are not a native interpreter.
The first four eligible idle combat units per town remain home guards globally,
not four per requested type. Each scheduled attempt orders only available idle
actors of the requested type and source owner; unavailable or unsupported actors
are skipped and the cursor advances without waiting. This is an explicit partial
group policy, not a claim of source readiness semantics.

The candidate uses game code `SCZB` and save generation 50, separate from the
existing Z10 candidate's `SCZA`/49. Its per-town wave byte becomes a bounded
sequence cursor; the existing timer stores the next generated wait. The loader
checks cursor and timer before applying state. Previous generation-49 saves are
not migrated. The stage is not enabled by default and no default Zerg promotion
is proposed.

Prepare the private AI report with `tools/sc64_ai_programs.py --mission Z10`,
then use `make test-sc64-z10-ordered-ai` and `make z10-ordered`. The generated
report and table stay under ignored `.local/` and `build/`; they must not be
committed. The pinned report hash is recorded in the packer. The isolated
candidate uses the same private prepared map and sprite inputs as Z10; missing
late actor artwork is still being addressed separately.

At `f864a188`, five asset-free packer tests passed. An ASan/UBSan actual-engine
fixture checked the first source-requested groups, global guard, partial groups,
loop wrap, cursor/timer rejection, rejection of a synthetic checksum-valid
previous-generation record and a current-generation save/load. Existing
stage-off Z9 and Z10 fixture targets passed. The separate ROM linked and verified
with unchanged guards: EWRAM `0x0203dff8` (eight bytes remaining) and IWRAM
`0x03004bf0`. A single unchanged ordinary-input controller attempt lost at
frame 80,780, with no mission fault; it is not a win or ARM replay. Full native
reference comparison, robust playable route, carried-mission qualification,
current hardware/emulator performance and physical testing remain open.

The supply-recovery controller fix `bd6a3673` addressed an observed selection
failure: at frame 13,631 the controller selected a Drone with zero spare supply,
13 Drones against a goal of 16, and three larva; its precheck rejected the Drone
before it could attempt an Overlord. With Overlord priority fixed, the ordinary
controller started an assault at frame 32,458 with 55 Hydralisks and reached
road leg 6 by frame 38,881, before losing at frame 67,257 / 68,509 calls with
zero faults. A checkpoint at frame 17,736 had 44 Hydralisks and 240 total live
actors (77 player, 163 enemy), so the early 70-unit threshold was pool-limited.

Controller-only commit `97c4a641` tried an earlier assault with at least 40
Hydralisks and at most one free actor slot. Its first assault began at frame
17,513 with 44 Hydralisks and reached leg 6 at frame 25,921; a second began at
frame 38,489 with 55 and also reached leg 6. The attempt lost at frame 67,373 /
68,546 calls, with zero faults and the Temple still standing. Neither attempt
is a qualifying win; no game rules, enemy AI, resources or actor limits were
changed for this strategy experiment.

The Z10 human permission table admits Mutalisk and the Lair/Spire chain. The
ordinary controller experiment `596ccb5e` used building menus and the normal
larva training cycle for a mixed force. It lost at frame 67,954 / 69,175 calls,
with zero faults. An observation-only reproduction confirmed a completed Lair
by frame 15,624, a completed Spire by 23,255 and 20 live Mutalisks by 37,196.
Its assault had already started at frame 16,188, before air production; thus
this does not establish an effective mixed assault. At frame 49,627 the Lair
and Spire were gone while 20 Mutalisks remained. The Guardian actor has no
ordinary player morph service in this profile; no Guardian production claim is
made. The engine, enemy AI and resource and actor limits were unchanged.

Controller `abdb1293` forwarded Mutalisk reinforcements along the same road and
required ten live Mutalisks before assault. It lost at frame 107,479 / 109,235
calls with no assault and zero faults. Observation-only census showed a full
240-actor pool at frame 20,553 when the Spire completed, zero Mutalisks then,
and only four by frame 28,082. The Spire was gone by frame 30,138; ten arrived
at frame 35,455 after rebuilding, when Hydralisk count had fallen below the
assault threshold. Controller `547c7a84` reserved composition space by limiting
pre-air Hydralisks to 30. Its single attempt reached the unchanged 200,000-call
limit at frame 197,630 / 200,027 calls, with no assault, win, loss or fault.
These are failed or incomplete controller experiments, not qualification.
