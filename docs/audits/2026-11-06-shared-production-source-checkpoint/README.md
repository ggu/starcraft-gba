# Shared production source checkpoint — 2026-11-06

These are source and focused HOST checkpoints on experimental branches, not a
new ROM release or native parity result. Initial source work ran with about
164 MiB free and no compilation. Space subsequently recovered and the focused
checks below ran under the shared native lock and disk guard. No new emulator
capture or art-bank generation ran. Preserve release fences.

## Reaver progression

The explicit `SC64_SCARAB_TECH_PATH_HOST_PROBE` requires the complete HOST flight
stage. Only that probe offers the Support Bay and admits Reaver training through
the existing producer, ownership and prerequisite checks. Default admission
remains unchanged. The new `custom-scarab-tech-path-terran` target constructs
paid structures through request/arrival/build paths, rejects a Reaver purchase
before Support Bay completion without spending, then trains the Reaver body,
buys ammunition and checks its journal continuation.

The fixture at `6d027015` passes with an actual one-AI setup initialized from
the outset, preserving enemy units and unchanged validators. The fixture
controls initial resources and worker positions, and advances component
schedulers. It is not an ordinary-button, full-world, fresh-process SRAM or N64
gameplay proof. The existing
full-service attack test remains separate.

The combined source at `a32d87f0` passes staged P10 typed and Z7 derived HOST
ordinary-button controllers. With explicitly granted starting resources and
initially idle workers, each builds the Pylon, Gateway, Cybernetics Core,
Robotics Facility and Support Bay, trains a Reaver and pays for its Scarab.
After a fresh-process load, it attacks an existing enemy, saves a pending
impact, then loads it in another process and verifies enemy HP loss. The
separate moved-parent test checks launch position, fractional motion and
atomic out-of-map rejection. Logs are
`/tmp/sc64-profile-button-a32-20261106/{p10,z7}.log`. This exercises custom
games under HOST flags, not guest execution or native N64 behavior.

The staged custom AI owner fix selects only active opponent slots for paid
stocking, including the second AI slot; the existing campaign policy is
unchanged. The constructed-wallet fixture checks all prepared maps, one to
three opponents, neutral/human/inactive slots and insufficient funds under
Terran, Z7 and P10 flags. The Terran receipt is
`/tmp/sc64-profile-aiowners-fbb-20261106/terran.log`; Z7 and P10 receipts are
`/tmp/sc64-profile-aiowners-51a-20261106/{z7,p10}.log`. These are direct
source-tick tests, not AI build-order or native behavior evidence. The first
Terran compile used an incompatible profile codec flag and failed its compile
fence; the corrected recipe excludes that flag and preserves the failed log.

## Other isolated source work

- Typed codec checkpoint `727d4705`, expanded fixture `ba1edca8`: feature-15 live Scarabs preserve heading
  and velocity; dead rows preserve signed HP and reject unrepresentable
  cooldowns. The sanitizer fixture passes all 5,120 velocity/HP combinations,
  malformed transactional rejection and legacy raw-36 Overlord byte identity.
  Staged P10/Z7 selected journals and ordinary-button HOST continuations now
  pass separately; ARM and the remaining profile qualification remain open.
- Cocoon checkpoint `8ef2845e`: optional renderer, independent private-bank
  generator and test fixtures. No bank was generated and no game hook or normal
  flags were enabled. Three timeline tests and the sanitizer pixel/layering/
  clipping fixture pass with the explicit scene-clip capability. The
  deterministic wait approximation is not N64 timing.
- Source-town Scarab wallet routing checkpoint `181f23e8` binds Z8 and Z9/Z10
  ammunition purchases to their immutable scenario town slots, rejecting wrong
  source/live owners, stale map bindings and ambiguous late town slots. The
  expanded wallet fixture passes under the Z10 vector, covering Z8 and all
  three Z9/Z10 town balances and nonspending denials. A reassignment
  between two valid source slots sharing one runtime owner cannot be detected
  from existing actor fields alone. Transfer/cancel/refund handling still needs
  an explicit integration test; do not claim capture safety from bank lookup.

## Resume qualification

The existing full-service attack fixture also passes on `6a77f506`: paid
ammunition commands, scheduled launch, pending-impact journal reload and resumed
damage remain intact (`/tmp/sc64-full-service-6a77f506-qualification.log`). This is
separate from the constructed tech-path fixture; it still begins with a spawned
Reaver and does not combine the entire path through ordinary inputs.

Next run the remaining combined journal regressions, then validate the full product
matrix, real tech progression/attacks/SRAM on ARM, source permissions, stack/RAM
and representative frame costs. Collect missing ordinary N64 production and
weapon observations separately. No Actions runner is needed.

The focused wallet recipe is `tests/sc64-scarab-wallet/Makefile`, target
`test-scarab-source-wallet-z10`. The tech-path target is in
`tests/sc64-shared-egg-custom/Makefile`; both use prepared private inputs.

## Failures retained and scope corrections

The initial wallet command tried to regenerate a stale imported prerequisite
and stopped at its missing pinned extractor. The successful focused retry used
existing prepared inputs with `CANONICAL_LOOKUP_INPUTS=`; no new bank was needed.
The first tech fixture inherited an unsupported zero-AI world. The writer
correctly refused it before loading (`mode=1 race=2 bots=0 player=2`). The fix
initializes one AI before creating the world, preserving the 1–3-AI save guard;
it does not relax validation or normalize the candidate before saving.

Private local logs: `/tmp/sc64-wallet-64207a16-prepared-qualification.log`,
`/tmp/sc64-tech-path-6d0.log`, `/tmp/sc64-tech-save-debug-state.log` and
`/tmp/sc64-typed-velocity-expanded.log`. All earlier failed logs remain retained.
