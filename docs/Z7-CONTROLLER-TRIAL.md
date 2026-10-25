# Z7 ordinary-input controller trial

This experimental host controller uses ordinary HOME/menu/gameplay inputs
with the existing Egg, source-sight, Scourge and nearest-depot options. Live
state reads guide targeting, economy and retreat decisions. A full run
returns success only for a completed source victory; loss and tick limits
return failure and must not be presented as campaign completion. Neither
policy below has won. No campaign promotion or 30 fps claim is made.

With the private Z7 inputs already prepared:

```sh
make build/test-sc64-z7-controller
mkdir -p .local/build/z7-controller
./build/test-sc64-z7-controller --policy baseline --limit 40000 \
  --trace .local/build/z7-controller/baseline.json
./build/test-sc64-z7-controller --policy early-gas --limit 40000 \
  --trace .local/build/z7-controller/early-gas.json
```

Both known losses return exit status 1. `make test-sc64-z7-controller` runs
the baseline with a 120,000-host-tick limit and writes the trace to
`.local/build/z7-controller/controller-trace.json`. The limit is a host-loop
budget; an in-progress ordinary cursor/menu gesture can consume extra ticks.

`make zerg-z7-controller-probe` builds the separate ARM probe
`build/StarCraft-Zerg-Z7-Controller-Probe-GBA.gba`, title `SC Z7 RUN36`, code
`SCZQ`. The host policy is not compiled into the ROM. Building the probe is
not an ARM gameplay result.

## Integrated observations, 2026-10-25

The baseline is default branch `e4b1178` plus this controller. Its paid
second-Hatchery policy loses at gameplay frame 22,491 / host tick 23,295,
with 55 enemy structures remaining. Ordinary build and production readback
confirm the second Hatchery and paid use of both producers' larva. The
expansion is subsequently destroyed. Re-running after the mineral-slot fix
produces the same 2,619 input/checkpoint/terminal events.

The opt-in `early-gas` policy clears all observed local structures, keeps
issuing assault orders after the wave switch, periodically selects new
reinforcements, seeks an Extractor before the starting gas reaches zero,
and delays expansion until the economy and army meet its safeguards. It
bypasses the baseline's post-switch defensive screen; it is not a proven
replacement for that policy. The local source-region clearance can activate
the source AI switch after a ten-second wait; 9,000 frames is its time-based
fallback, not the sole activation path.

The early-gas trial completes a paid Extractor and assigns two gas workers
through ordinary input at frames 10,626 and 10,762. Gas deposits increase the
bank from 100 to 116. It nevertheless loses at frame 15,198 / host tick
15,587, with 52 enemy structures remaining. Enemy Hydralisks kill most of the
opening ground force while it advances beyond support. This demonstrates gas
recovery, not a successful mission strategy.

That trial exposed an ASan/UBSan read of `m[-1]` in the host controller's
mineral reassignment when fewer than three safe patches exist. Candidate
slots are now checked before lookup. Both complete qualified losses run
without sanitizer errors after the fix. The replay validator accepts an
input transition and a terminal observation at the same host tick because
the observation follows that update; the terminal must still follow the
live start and cannot precede an input. These lost traces remain rejected
as full-win evidence and can only qualify explicit attempt prefixes.

Native AI timing and a mission-wide winning strategy remain unresolved. This
controller does not change AI, objectives, actor capacities or save formats.
Private traces, logs and imported inputs stay outside Git.

## Per-update count-cache comparison

The source-only replay tool compares the reference controller profile with
only `SC64_Z7_PREVALIDATED_TICK` and `SC64_Z7_COUNT_SNAPSHOT` added. It validates
the normal menu route and recorded terminal state, streams the same sparse
held/pressed inputs to two fresh ASan/UBSan processes, checks zero adapter
faults, and compares common-state hashes after every update. Coverage includes
actors, building queues, resources, fog, source runtime/AI, target-list caches,
invincibility and the selected UI fields listed in the C driver. Candidate-only
validation/count context fields are excluded. This is a deterministic state
comparison, not a timing measurement or a proof over every possible input.

After producing the traces above:

```sh
python3 tools/sc64_z7_trace_parity.py \
  .local/build/z7-controller/baseline.json \
  .local/build/z7-controller/early-gas.json
```

Private output streams stay under `.local/build/z7-trace-parity/` by default.
The integrated baseline and early-gas traces match at all 23,295 and 15,587
updates respectively (38,882 total), including their authentic loss states.
Their identical output stream SHA-256 values are
`949a209e55e78cf3f9bcd23a4972880c7b3541878e2467208a009f027007af0e`
and `3d202c237616ea25fa03963f1160ce1506c5ebf4c142f094ed622d95af6888a7`.
Neither a successful mission route nor an ARM replay is implied by this host
comparison. See [Z7-COUNT-SNAPSHOT.md](Z7-COUNT-SNAPSHOT.md) for the earlier
bounded ARM optimization experiment and its limits.
