# Opt-in late-campaign paid Eggs

## Implemented scope

Runtime `f32356c6dcc741b504b7e2f69e90e68161c1c361`, with test-only successor
`7166def09bb2ba80a96a70d48db06d70531c522a`, joins the existing late Egg
journal, producer and deferred-trigger reservation backend under one explicit
`SC64_SHARED_EGG_LATE_SERVICE` flag. Normal product flags stay off. This is an
experimental integration, not completed Guardian production or default rollout.

The Z9/Z10 content vectors already contain Z8 source bindings. The service
adds independent paid Eggs there, candidate-local owner mapping including the
Z10 descriptor, selected outer/inner save context, paid paired-birth commitments
and authenticated deferred CreateUnit cursors. It preserves the original inner
version while validating predecessor Z6/Z7 records. Competing prototype
namespaces remain fenced. No new persistent arrays are added.

The same shared producer is used in custom games under this late vector;
this does not activate mission objectives in custom play. Other products retain
the separately qualified custom-only preview. Common default activation across
the delivered products is still a next gate, rather than a Showcase-only fix.

## Constructed HOST and input evidence

Eight ASAN/UBSAN targets passed at the runtime: Z8, Z9 and Z10 journal/owner
rejection; paid capacity boundaries; Z8/Z9 deferred trigger save/retry; custom
journal rejection; and custom ordinary input select/pay/save/cancel/load/birth.
These are HOST tests, with explicitly constructed states in capacity and
corruption fixtures. A separate ordinary-input controller launched Z8/Z9/Z10
from menus, selected a real Hatchery, paid from source opening resources, saved,
loaded and reached natural birth without resource or unit writes after launch.

The matching-product predecessor target passed at `7166def0` for Z6–Z9:
genuine older-code HOST writer, current normal reader and opt-in reader. The
opt-in also rejects mismatched tag pairs and falls back to the unchanged older
valid slot without changing SRAM. An initial fixture failed because it supplied
a Z7-product record to a Z9-product reader; the normal Z9 reader rejected it too.
The test was corrected to use the same product generation, with no validation
weakening. These HOST journals were never imported into a guest.

## Actual ARM and genuine SRAM

Opt-in Z9 and Z10 ROMs pass the standard header, lower/upper RAM region and
reserve checks. Both have `__bss_end = 0x0203dce4` and
`__iwram_end = 0x03004bf8`: 796 bytes EWRAM linker slack and 1,032 bytes IWRAM
linker slack to the existing guards. Linker slack is not a stack-depth measure.

Four physical-button cases passed with the exact runtime ROMs:

| Product / mission | Actual paid minerals | Genuine 32 KiB SRAM SHA-256 |
| --- | --- | --- |
| Z10 / Z8 | 350 → 300 | `020640f3bcc25e8235d918be81e2ef6bab94b450a71818353bae933342844215` |
| Z10 / Z9 | 750 → 700 | `831393045036a555d03cc3172943c942ba2d8805804364ea9cbad335ffd3f18c` |
| Z10 / Z10 | 1,000 → 950 | `8b5a65b99724df22882a0f5abedf7c407a92a55cea6fee8ab46ed6dd297da9b6` |
| Z9 / Z9 | 750 → 700 | `b5998ed0508b6647e2fcc523d8ffd03bfef30cad856ebbd57926da400271a368` |

Each used normal menus and physical controls, consumed one larva and produced
a raw-36 Drone Egg with source owner 3. Pause B saved; the emulator exported its
actual SRAM. A fresh process booted with exactly those bytes and HOME L loaded
it. Named live units/buildings/queues, resources, supply, selected mission,
active scheduler and AI state matched the uninterrupted run on the next active
frame and after natural birth. The parent independently compared those complete
named-state JSON snapshots and verified the saved and re-exported SRAM bytes.
This is not full-RAM equality, arbitrary-save qualification or a mission win.

Two nonpersistent human-wallet scratch shadows differ immediately after load;
the current runtime overwrites them from real wallets before each active
scheduler tick. Immediate restore comparison excludes only those two shadows
and screen state. The complete named comparison includes the shadows again on
the next frame and after birth, and matches exactly. Real wallets always match.

The first fixed HOST tape failed on ARM because HOST auto-acknowledged mission
text boxes. A second private assertion also assumed an actor enum from a
different roster. Neither failure was fixed by changing gameplay. The passing
controller reads the actual modal count and sends real A press/releases, then
drives cursor selection, production and save/load. Failed attempts are retained.

The validated read-only runner derives from pinned mGBA 0.10.5 and permits
bounded VRAM reads for modal state. Runner SHA-256:
`3c177dd8213c803025fb3fc999bb90f868fc40ecb612cbbdbecdb5f4c9bc79c0`.
No guest-memory writes, debug auto-dismiss, fabricated resources/units/outcome,
HOST SRAM import or network debugger are used. Runtime objects, 1,964 private
input hashes, ROM/ELF and runner identities were checked before/after each case.
Private scripts, traces, failed results and actual saves remain local.

Exact private opt-in runtime ROM hashes before publication recipes:

- Z9: `459a7f254530e6a5ef388f3a092b30fc728b43a7f38aed95d24009cb95a83995`
- Z10: `861b6e50a56d127332092fd11e4c10cf00fe61e07a66bd414ccf6599f2d55c19`

## Remaining gates

All six normal service-off ROM hashes and RAM endpoints match the prior `6ba`
qualification exactly at `7166def0`. This preserves those products; it does not
activate the new service in them. Clean default and public preview recipe
qualification are tracked with the publication result.

Next: a common enabled release capability across applicable products, full
campaign/custom regression and late-source permissions; Greater-Spire Guardian
morph/cancel and Cocoon lifecycle; broader queue/trigger interleavings, stack,
long matches and busy ARM performance. Static Egg art is present, while source
pulse/birth/shadow animation remains unfinished. Z10 ordinary victory is still
unproven and coverage remains 29/30.

See [isolated preview commands](../../RACE-CAMPAIGNS.md#opt-in-late-campaign-egg-preview),
the [performance and N64 observations](README.md), and the
[living backlog](../../PROJECT-TODO.md).
