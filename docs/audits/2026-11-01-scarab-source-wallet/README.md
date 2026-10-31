# Scarab source-owner wallet binding

The gated adapter previously charged the generic combat-owner bank. P7 source
players 0 and 4, and P8 source players 0 and 3, each have independent wallets
despite sharing ENEMY. The adapter now resolves a validated parent through its
campaign's source-owner callback before enqueue, cancellation or cancel-all.
Unsupported or contradictory provenance cannot fall back to the enemy bank.
Custom games retain their engine-owner banks.

The transient binding preserves the storage types: campaign `uint32_t` funds
are not accessed through `int *`. P7 force wallets have the existing 10,000
persistent-state bound; P8 and engine wallets have a 65,535 bound. Queue and
refund operations preflight that limit before callbacks or wallet mutation.
No new persistent fields, clock, namespace or Reaver admission are introduced.

At frozen source `da5cfa3cb96c046490c7defc8d860c62349718e6`, local checks passed:

- Source boundary, doctor, default ARM build and full clean unprepared
  `./dev test` (926 Python tests, 26 skips, the separate five- and eight-test
  suites, and remaining host checks). The full test step took 406.45 seconds.
- `make test-sc64-scarab-wallet`: signed/unsigned storage isolation, full
  0..65,535 value range, invalid bindings and source-limit boundaries.
- `make test-scarab-source-wallet`: seeded P7/P8 game fixtures for independent
  wallets sharing an engine owner, human/custom payments, source mismatch,
  insufficient funds, cancellation and active-child refund overflow. Each
  fixture generates its own matching experimental actor-fact table.
- `make test-scarab-unit-adapter`: the existing Terran, Z6, P7 and Showcase
  adapter regressions. Prepared checks used the same 109 pinned inputs and
  walk-word, contact-table and indexed-weapon sidecars, hash-verified before
  and after. Native work was serialized and existing source/pin checks held.

The prepared recipes used `BC_P7_FACT_DIR=build/generated/p7-fact-v2-bc-current-rect2`
and `P7_FACT_V2_DIR=build/generated/current-p7-fact`; their generated Scarab
tables are isolated per run. Private inputs and generated tables are not
committed. Local receipts are `/tmp/sc64-wallet-focused-20261101/result.json`
and `/tmp/sc64-wallet-default-20261101/result.json`.

These are seeded helper checks, not ordinary mission completion, source-art,
production-ROM, hardware-performance or complete Scarab-service qualification.
The transfer contract still requires callers to cancel paid work while the
old source owner owns the parent; a queue without an active child has no
independent payer identity. Full GiveUnit hooks, hidden-child retirement,
source cadence, AI stocking and active-flight save integration remain separate
work. This change does not enable the experimental controls or reinterpret
old saves. See the P8 Scarab readiness audit for the remaining shared service.
