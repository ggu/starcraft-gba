# Reuse Z7 live actor validation in the trigger census

Eleven gameplay instruction samples of the retained normal Z7 replay
(calls 15,000–15,979, excluding menu call 15,623) put the masked census
at 79,678–82,724 guest cycles, averaging 81,333. Mandatory live actor
validation averages 18,407 cycles. The retained samples include region
geometry calls: the 81,333 cycles are an inclusive scope, not an
estimate of the cycles this probe saves. The samples do not record
the late two-counter mask's frequency.

The default-off `SC64_Z7_VALIDATED_CENSUS` requires the existing
prevalidated, pruned count snapshot. When only the current player's
loss check and hostile force's building check remain, live validation
collects those two counts before the snapshot would otherwise scan
both pools again. No count is published when an actor fails validation.
The snapshot still validates its source rows, source locations and
current-player binding. For other pruned masks, the census retains
the same owner, raw bounds and geometry checks and only reuses the
already-proven raw-to-live type result. The full five-counter census
retains its original path. Trigger evaluation and action order, save
layout and persisted state are unchanged.

The two HOST variants of `test-sc64-z7-validated-census` run with
ASan/UBSan and existing local generated inputs. The fixture checks all
32 masks against independent world counts, source/callback
authentication, mutation and active waits, pause/load and terminal
behavior. The new case compares both late counters to the existing
standalone census across unit and building liveness transitions, and
asserts a malformed late building leaves both output counters untouched
and produces the same actor fault. The ARM comparison below separates
geometry-bearing steady work from the late authenticated two-counter path.

## Paired Z10 cartridge comparison

The staged `c906058` Z10 control and one candidate link use the same
normal vector. The candidate changes only the two census runtime
headers and adds `SC64_Z7_VALIDATED_CENSUS`. Both verify as 32 MiB.
Text increases by 320 bytes; data, BSS end `0x02038b00` and IWRAM
end `0x03004be0` are unchanged.

The ordinary Z7 HOME replay matches 39 HOST checkpoints through
call 15,979. All 12 ordered samples match both indexed pages, palette,
fog, actor/mineral pools, route metadata and trigger state. The
eleven gameplay samples average 1,194,972 → 1,190,944 guest cycles
(−0.34%); the command-menu sample is excluded. These are sampled
updates, not the average of the entire prefix.

Read-only breakpoints at the linked census call and its return
instruction time the masked helper independently. The first three
steady gameplay calls (15,000, 15,089 and 15,178) have
`started=0x3a` and mask `0x1b`. The helper takes 82,739 → 74,455,
82,024 → 73,821, and 88,987 → 73,991 cycles. The clocks can include
IRQ time; the whole updates match those from the independent
12-point replay at the same calls.

A fresh load of the retained normal Z7 journal at frame 185,926
exercises the two-counter path (`started=0xfa`, mask 3). Three
subsequent physical samples at frames 185,936, 185,962 and 185,988
match actor world, trigger state, both indexed pages and palette.
The helper takes 14,487 → 4,161, 14,461 → 4,161, and
14,486 → 4,161 cycles. This excludes the extra counting in live
validation and is not a net savings estimate. Neither qualification
extends to victory; the probe remains off.

- Control ROM: `ef36f3f0e8f44ea1207ef39c05ac1417c727b498a2596e93b98b765c74b9768e`.
- Candidate ROM: `7795e4b27965bfacd8d65bb1df727a1410c7c209991f2ad28126b1f7b971ee68`.
- Control / candidate steady receipts: `d80fcc020d0dd449d96e644ceb761f684010951c78478dc567bb4c79cc3efc01` /
  `ce6ea78d24e2d6cbab99a7c9fdd0a75eb33e944f6faffcdc6245ab1841865bfb`.
- Control / candidate late receipts: `e163d33b5f08a9827aca589066962cb0c5bc2747040cce01f79c393c96ad33de` /
  `cb36d49fcb278a01c553f0dbcacd6d14d63ec9d9534ac58758931eaf4bba10b0`.
