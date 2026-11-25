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
The optional window scopes grants, exclusivity checks and peak waiting counts;
observations before it establish waiting age. Repeated samples within one guest tick remain separate fields, since updates
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
these figures from 400 captured rows, requiring increasing frames and cycles
and an uninterrupted, one-per-frame countdown with stable worker identity,
owner and target. This is not a direct conversion from
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

## Experimental GBA save continuation

In the shared queue trial, the same three-opponent Blood Bath save for each
cartridge was advanced from frame 10,257 to 10,274. The ordinary pause menu
wrote SRAM while a worker waited behind another. A fresh emulator loaded it
with L on the title screen. The ordered worker slots and the captured world
fingerprint matched immediately and after every one of the next 128 updates
on Terran10, Zerg10, Protoss10 and Showcase. The earlier P10 trial lost its
second worker at site 25 on the initial reload, despite matching the world
fingerprint. That build fails this check. The input SRAM must match the product.

```sh
python3 tools/sc64_gas_service_continuation.py "$runner" "$rom" "$elf" "$busy_sram" "$new_output_dir" --advance 17 --steps 128
```

Tagged journals store queue rank, and preflight checks those ranks before
publishing the candidate world. The Zerg product journal HOST test recomputes
the checksum of a newer bank containing a duplicate rank. The selector accepts
the older valid bank. When both banks are unusable, the load fails and preserves
the checked live state and SRAM. An additional HOST fixture saves immediately
after cancelling a waiter, before the next worker update removes it.

At the qualified trial revision, the race cartridges cleared their configured
512-byte stack margin with 240 spare bytes on Terran, 432 on Zerg and 816 on
Protoss. Showcase uses a different link without this stack check. The trial
still requires campaign and full-match economy and timing comparisons.
