# Saturated gas-site service observation

A retained ordinary USA N64 Blood Bath match was continued with neutral input.
The retained state was at guest tick 19,444; the sampled continuation began at
24,400 and covered 3,001 VI fields. At one saturated Refinery, 16 consecutive
service starts from tick 24,665 through 25,220 were 37 guest-counter increments
apart. Each grant went to a worker in the oldest observed waiting group. At
most one sampled worker at that site had raw order 83, while up to 12 had raw
order 82. Other stocked captures showed service overlapping across distinct
sites. These are sampled observations: workers whose first waiting sample is
simultaneous are tied, and no rule for breaking that tie is established.

`tools/sc64_gas_fifo_observe.py` checks per-site sampled exclusivity and that
an observed grant does not bypass an earlier observed waiter. It accepts private
JSONL observations as `[field, guest_tick, [[worker_id, raw_order, site_id], …]]`;
82 is waiting and 83 is service. IDs can be opaque. It rejects a grant whose
wait was not sampled; use the optional tick window to exclude preexisting jobs.
Repeated samples within one guest tick remain separate fields, since updates
and VI boundaries need not coincide. Its tests exercise FIFO, cancellation by
absence, site independence, occupancy and unresolved arrival ties. This is an
observation checker and executable specification, not a test of current gameplay.

```sh
python3 -m unittest tests.test_sc64_gas_fifo_observe
python3 tools/sc64_gas_fifo_observe.py "$private_fields" \
  --first-tick 24665 --last-tick 25220
```

The observed grant ticks were 24665, 24702, 24739, 24776, 24813, 24850,
24887, 24924, 24961, 24998, 25035, 25072, 25109, 25146, 25183 and 25220.
The input captures and game assets are not included. A full observation adapter
for future GBA implementations should record every relevant worker at each
sample; omitted workers cannot establish exclusive occupancy or fairness.

## Comparison with the packaged GBA build

In the separate packaged P10 three-computer Blood Bath capture described in
[the gas audit](2026-11-26-n64-gas-pickup.md), 24 complete observed gas
intervals ran from timer 59 to cargo in 59 updates. Median elapsed emulated
GBA time was 3.068 seconds, calculated from CPU cycles at 16,777,216 Hz.
There were simultaneously up to 2, 3 and 3 empty workers counting down at the
three sampled sites. `tools/sc64_gas_gba_clock.py "$private_clock"` computes
these figures from 400 captured rows. This is not a direct conversion from
N64 guest-counter increments to wall time, and neither measurement alone fixes
the duration for an implementation.

## Implementation boundaries

Current GBA worker timers are independent. A future opt-in single-server
implementation needs to preserve arrival order, invalidate waiting or active
workers on retarget, death, transport and refinery loss, and let independent
sites run concurrently. Test fresh save/load during contention and older saves
with multiple partial timers, without losing carried resources. Timer-field
encoding and save compatibility have not been established. No behavior or
shipping build setting is changed by this observation checker.
