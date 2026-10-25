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
not an ARM gameplay result. The experimental host/probe flags now opt in to
prevalidation and the condition-count snapshot; the reference remains
available to the parity tool. Standard race ROM flags stay unchanged.

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
actors, building queues, resources, fog grids and incremental visibility/
minimap snapshots, source runtime/AI, target-list caches, invincibility and the selected UI fields listed in the C driver. Candidate-only
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
`32ee290ba591e9e54183dadaac1d50df356fa8e8defb3b3ed1f498588b41bb16`
and `a635fcd145833601be5e7d7953ce30aec2071610d586074bc8506339ea603ab9`.
Neither a successful mission route nor an ARM replay is implied by this host
comparison. See [Z7-COUNT-SNAPSHOT.md](Z7-COUNT-SNAPSHOT.md) for the earlier
bounded ARM optimization experiment and its limits.

## Integrated ARM timing window

A separate private pair built from the current source replays the first
12,000 ordinary-input baseline host ticks in fresh mGBA processes. The
observer retains `game_update`, `audio_update`, `game_render` and its display
deadline. Reports and checkpoint hashing occur outside the timed interval;
the debugger scheduler is replaced with a read-only input tape. This is an
instrumented ARM experiment, not a standard-ROM or physical GBA measurement.
Only prevalidation and the count snapshot differ between the pair.

All 25 frame/common-state checkpoints match, including mission 18, bound
callbacks/map, source AI activation and zero runtime faults. The 500-update
window means are:

| Measurement | Reference | Count snapshot | Reduction |
| --- | ---: | ---: | ---: |
| Update, audio and render cycles per update | 1,808,087 | 1,481,554 | 18.06% |
| `game_update` cycles per update | 1,294,846 | 958,699 | 25.96% |

Both builds exceed the 561,792-cycle two-VBlank 30 fps budget on 11,814 of
12,000 updates. The maximum measured full updates are 17,134,620 and
16,616,771 cycles. The experiment does not establish the cause of those
outliers. These results justify the faster experimental probe, not a claim
of 30 fps or a campaign promotion. A winning normal-input route, full ARM
replay, fresh-process SRAM continuation and further performance work remain.

The resulting non-observer controller probe is header/size/SRAM-signature
verified at 32 MiB. IWRAM ends at `0x03004d38` and BSS at `0x0203de00`, below
the unchanged `0x03005000` / `0x0203e000` reserves. The focused host save check
reports a 16,296-byte payload below the 16,368-byte record limit. All seven
count/prevalidation/runtime/save checks pass. Both optimized controller
policies reproduce their original complete input/checkpoint/loss traces.
The default original-art ROM remains byte-identical; all 447 asset-free
Python checks and the default host fixtures pass.

A separate [source-radius sight-span probe](Z7-SIGHT-SPANS.md) reduces visibility
update work while retaining the count-only probe for comparison. Its tested
state equivalence and bounded ARM timing are documented separately.
