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

Release GBA worker timers are independent. An opt-in single-server
implementation needs to preserve arrival order, invalidate waiting or active
workers on retarget, death, transport and refinery loss, and let independent
sites run concurrently. Test fresh save/load during contention and older saves
with multiple partial timers, without losing carried resources. The following sections track the shared trial separately from the release.

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

## Campaign and skirmish measurements on 528d9082

On the three race cartridges, 30 retained victories opened with the expected
completion bits. Each advanced to the next chapter or restarted its finale,
then wrote a pause save. In a fresh mGBA process each save restored the
expected mission, frame and completion mask and re-exported identical SRAM.
This covers chapter transitions. Separately, ordinary inputs from a retained
T9 victory completed T10 at frame 3,361 with Raynor alive and the Ion Cannon
destroyed. All six gameplay checkpoints matched and the victory save survived
a fresh boot.

At frame 35,363 of a retained T9 campaign save, three workers were queued
at the same refinery. A fresh load preserved their order and the selected world
state on all 512 subsequent neutral-input updates. A separate run starting
from the same legacy SRAM compared the finite-gas release with the single-worker
trial. The player's bank started at 1,702 gas on both. During 2,048 neutral
updates only one map site lost gas. It supplied 528 on release and 280 on the
trial. The player finished with 2,222 versus 1,990 gas, and 226 minerals on
both. Amount extracted and bank balance cover different parts of the trip.

To compare pacing, run the native checker above with `--grant-spacing`.
In the saturated N64 sample, 15 consecutive start-to-start gaps at one
refinery spanned 121–151 video fields, median 130 fields (about 2.17 seconds
at a nominal 60 fields/s). Each gap was 37 guest-counter increments.
On trial P10, a repeat of the busy Blood Bath save from frame 10,257
produced five back-to-back service gaps at each of three sites. Count a gap
when an observed empty worker starts at timer 59 and the next service begins
one update after the preceding pickup. All 15 gaps were 60 GBA updates,
3.12–3.18 emulated seconds (median 3.15). These captures have different
armies and loads. A global timer change also affects lighter scenes, where
60 updates at the 30-updates/s target would take two seconds. Measure a light
scene and dense combat before choosing the shipping duration.

The trial P10 cartridge completed P9 from a retained P8 victory. Its original
43 checkpoint records and fresh result SRAM matched. From that P9 result,
the first P10 mismatch was at input 19,591: 128 enemy units versus 127
expected, and 916 gas versus 940 expected. The same adaptive control policy
on HOST reached 220,000 inputs without winning or losing. It had 45 combat
units at the cutoff; the policy normally waits for 60 or a low mineral bank.
The Z10 cartridge admitted the retained Z9 victory; its old finale inputs
first disagreed with the measured release gas bank at input 13,374 (668 gas
on the trial versus 996 on release). A result-transition pass alone does
not establish either finale win from a playable start.

Using the existing `P10_ATTACK_ARMY` control setting, an ordinary first
assault at 40 combat units cleared the Overmind on the FIFO build at frame
46,025 (input 47,257). The normal host recipe reproduced the tape byte
for byte. In mGBA, the cartridge started from the new P9 result above,
matched all 19 recorded checkpoints, kept the heroes alive and restored
the P10 victory from fresh SRAM. The controller's attack threshold changes
the test player's strategy; the mission and AI rules are shared by both
runs. Reproduce the controller with:

```sh
make build/test-sc64-p10-gas-service-home
./build/test-sc64-p10-gas-service-home /tmp/p10-gas-service.jsonl
```

Pass that trace as `--p10-trace` to `tools/sc64_protoss_linked_replay.py`
with the trial ROM, P8 victory and qualified P9 trace. Winning FIFO P10 trace
SHA-256: `708a85076ab45427618db6b234b68cdd13f4d13d62b5e2021dde380c5f7ba246`.

Additional Zerg cartridge cuts used 512 neutral updates from retained play.
In Z1 (49 live units), two complete 59-to-cargo intervals took a median
1.98 emulated seconds. On one-AI Outpost (50–60 live units), 15 such
intervals took 2.60 seconds; on one-AI Blood Bath (31–49 units), nine took
2.48 seconds. For the busy three-AI P10 cut above, 16 complete intervals
took 3.09 seconds. This variability with the same timer supports improving
busy-frame pacing before shortening the service interval globally.

The same ares field capture also distinguishes time spent in service from
time between grants. With `--first-tick 24401 --grant-spacing`, 18 worker
visits have both an observed entry to raw order 83 and an observed exit.
All 18 lasted 37 native guest ticks. Three started with no other observed
waiter at that site (128–140 VI fields, median 140). Fifteen started with
waiters (121–151 VI fields, median 130). Visits already active at the
window start or still active at its end are excluded. This measures one
native battle; an empty queue at entry does not imply a lightly loaded map.
