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
full-service attack test remains separate; the eventual ordinary-input gate
must combine progression, ammunition, attack and genuine fresh-process saves.

## Other isolated source work

### Button-driven tech progression and genuine HOST save continuation

Controller `8e178e16` passes a stronger progression gate: one-AI custom setup
through menus, button-built Pylon/Gateway/Core/Robotics/Support Bay, paid Reaver
training, exact 15-mineral Scarab debit and completion through `game_update`.
It pause-saves at tick 7,101, exports the genuine journal and starts a separate
process that pause-loads and continues for 60 frames. Initial 5,000 mineral/gas
funds and idle workers are declared fixture setup; no prerequisite structure,
Reaver or target is spawned by the controller. This is HOST input qualification,
not an unassisted economy opening or ARM proof. The SRAM hash is
`9432ba321157357943d0c4ff1dcbd18682ae88a19e60663a79253142ce2eeacb`.

The real-opponent attack extension at `3808c41a` **does not pass**. Its tech
write/read pass, but no Scarab impact appears before match loss at game frame
12,448. The Reaver remains alive with an attack-move order. Retain this as a
playability gate and investigate routing, target acquisition and launch state;
a constructed nearby-target test cannot substitute for fixing or qualifying
the real custom-game path. Logs remain private at
`/tmp/sc64-scarab-controller-8e1.log` and `/tmp/sc64-scarab-controller-380.log`.

### Codec, renderer and source-wallet checkpoints

- Typed codec checkpoint `727d4705`, expanded fixture `ba1edca8`: feature-15 live Scarabs preserve heading
  and velocity; dead rows preserve signed HP and reject unrepresentable
  cooldowns. The sanitizer fixture passes all 5,120 velocity/HP combinations,
  malformed transactional rejection and legacy raw-36 Overlord byte identity.
  Selected journal integration and ARM qualification remain open.
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

Next integrate the codecs and run the remaining combined journal regressions,
then validate the full product
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

A bounded fresh N64 Pylon-reference attempt reached only the Episode I menu.
Its controller shim did not produce the intended sequence; the owned process
was terminated and its hashes/logs retained under the ignored private
`pylon-completion-2zwm0rfp` session. No construction, cost, animation, timing or
parity finding follows. No guest-memory writes or RAM snapshots were used.
