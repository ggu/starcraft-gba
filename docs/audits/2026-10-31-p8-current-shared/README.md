# P8 candidate and shared Protoss service qualification

This checkpoint retains the public P8 history and integrates shared engine
`5760755`. P8 remains an explicit `protoss-p8` candidate. The canonical menu
remains P7 and total canonical coverage remains T10/Z6/P7 (23/30).
This is not completion of all missions, native fidelity or near-30-fps work.

## Actual P8 ROM

The ordinary input tape wins on the actual normal P8 ROM, using its existing
main loop and scheduler. At runtime `bf73484`, all 91 observed host checkpoints
match on screen/frame/mission/won/lost. The final state is won=1, lost=0 at
input tick 90,428, simulation frame 88,769. No actors or world state were
injected; only bounded scheduler inputs were written through the port-free
backend. Dense input pages are refilled after consumed entries rather than
changing the normal ROM's queue size.

A separate normal-ROM pause-menu save at tick 32,445 produced real 32 KiB
SRAM. A fresh process loaded it via ordinary L/release, matched the bounded
entity/economy sample at frame 31,677 (103 units, 122 buildings), kept SRAM
unchanged and continued to the same final victory. A current-writer image
was byte-identical to the loaded input. Intermediate resumed world state was
not compared. The normal replay observes five fields, not whole-world parity.

Normal P8 links at `1853673` and `75e1cd2` are byte-identical to that replayed
ROM, SHA-256 `3d2fb002838d4ee01095c813c5485aab7616aae2bd76d43fd6cf65b199008b81`.
The latter uses a newly generated revision-2 P8 fact header in a new directory;
all original 103 pinned inputs remain unchanged. Its BSS/IWRAM endpoints stay
`0203899c/030049e0`. Thus the replay applies to these exact ROM bytes, without
transferring unrelated products' or hardware/performance claims.

The current P8 host tape matches its prior 91-checkpoint win. Its 234 carried
startup/save cases, shared service checks and skirmish fixture pass. The
normal-ROM tool has focused input/comparator/restore tests, including dense
pages, bad boundaries and mismatched restore summaries. Earlier custom-main
guest proofs remain separate historical evidence.

## Reproduced stale-geometry failure and correction

The shared binding and clock code changes canonical P7 and Showcase behavior;
their runtime is not claimed unchanged. A new actual canonical P7 service run
failed Overlord detection with the frozen historical BC fact header. That
header lacked rectangle rows for raw-42 Overlord and raw-144 Spore Colony.
The table path returned invalid before the reference fallback could run.

The current supported Make generator was run into a separate derived directory,
without overwriting the old header or other fixed inputs. It supplies those
two reference-derived rows. The table's packed V2 layout is unchanged, but
generated rectangle closures now carry geometry revision 2; the consumer
rejects old or unmarked rectangle closures with a regenerate diagnostic.
Fact-only legacy compatibility is preserved. Six asset-free fact tests pass,
including current-header success and old/missing-revision compile failures.

The old actual-engine assertion and its argv are retained. After the contract
change, an actual stale-bank compile fails at the intended diagnostic. With
the current generated header, the same P7 service fixture passes detection,
cloak, Carrier training/flight, Battery service and save/legacy-clock checks.
The actual Showcase vector passes its exposed detection/cloak/Battery checks.
Showcase lacks the full Carrier training/flight capability, which is explicitly
printed as unqualified; no flags were injected to disguise that engine gap.

The original canonical BC header SHA-256 is
`295a2ed07eba3a84e84c788fe30be1a272541bd76b623cf84702e5b5b74947c4`.
The selected newly derived canonical header is
`b13116b8e91b16bfb8740e9a77cd9ccb2d5969d57265fde4a5df55eccbefb154`;
the separately generated P8 header is
`13a6436fa5d4e5ab8e6c36a396a08cd71d8cc0d669d5c2196f70763bf38e17a9`.
All original 104 canonical input files remain unchanged, including the old
unselected header. This is a new derived closure, not a claim that all selected
inputs are identical to the old build. Generated data is not committed.

## Normal product checks at runtime 75e1cd2

- Clean unprepared boundary, doctor, default build and full suite pass: 883
  Python cases with 23 optional skips, ancillary 5/8 groups and native fixtures.
- Four actual normal Make ARM products pass existing memory guards. Independent
  title/code/header checksum/SRAM-marker, ELF payload outside the fixed header,
  and FF-padding checks pass. The Showcase code is `SCSH`; an initial verifier
  used a mistaken expected code and was corrected from the actual Make recipe.
- All 32 threat regressions and all 272 AI progression/recovery/save cases pass
  on the actual four compiler vectors with the corrected derived P7 facts.
- Four actual normal-ROM fresh-process 9,000+900-update SRAM continuations pass
  with three mixed bots. Saved/restored and resumed/reference bounded semantic
  fingerprints include active queues; input SRAM remains unchanged.

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `020370c0` | `03004b40` |
| Zerg | `02037134` | `03004c10` |
| Protoss P7 | `020382f4` | `030049e0` |
| Showcase | `0203afa8` | `03004c78` |

Earlier stale-closure ARM/ELF/map artifacts are preserved with readback hashes.
The separate accidentally started older-checkout test was stopped and excluded;
the source-pinned clean 75e1cd2 run above is the qualifying run. Logs, commands,
original/new input hashes, negative controls and semantic samples stay ignored
under the `p8-final-shared-20261031` qualification folder and the separate
P8-input and clean-default worktrees. All checks ran locally, with no Actions,
SD writes or public proprietary payloads.

P8's remaining service/fidelity gaps and canonical promotion remain separate
work. In particular, source-exact Scarab behavior, complete cross-product
Carrier support and broader mission/performance qualification are not supplied
by a successful mission tape or these bounded save checks.

## Concrete shared performance hold

Do not merge this checkpoint into the shared default yet. A paired-source
audit found a material custom-game cost increase from enabling generic
Protoss services. With identical save and runner hashes, initial fingerprints
and populations, and nine matching sampled state/pixel checkpoints per
900-update window, mean full-loop guest cycles rose from 876,869 to 1,087,220
in canonical P7 and 883,128 to 1,048,185 in Showcase. Orders rose from 422,664
to 520,323 and 410,359 to 502,969 respectively; rendering was essentially
unchanged. The new-source control ROMs are byte-identical to the 75e1cd2
normal artifacts, so these results apply to this checkpoint. They are scoped
emulator measurements and include presentation wait, not hardware timings.

An isolated diagnostic of the Showcase window attributes about 92,071 mean
cycles/update to the shared Arbiter cache processing, close to the measured
92,610 orders increase. Its nine sampled state/pixel checkpoints matched the
uninstrumented control; clock-read overhead and code placement limit that
attribution. The cost must be reduced while retaining cloak, detector, clock,
grace-period, save and slot-reuse behavior. Disabling those shared services in
custom games would not satisfy the intended engine parity. A passing mission
or save matrix does not resolve this performance hold. The separate contact
table experiment improves its own control but does not remove this regression.
