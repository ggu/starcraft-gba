# Recurring order work after the wide-route release

This audit uses the canonical release `99e4c26d`. The ordinary P7
controller replay launches through HOME, executes 9,020 tape calls,
and measures the final 1,000 calls. Of those, 958 advance gameplay.
Mean orders cost is 831,114 cycles. The mean full update is 1,749,591
cycles against the 561,792-cycle budget for 30 updates per second.

## Optical visibility trial

A tile cache reused optical detector results for the same cell and
source-owner mask within one source tick. Both builds matched all
1,000 recorded timeline rows and 11 sampled world and framebuffer pairs.
Mean full-update cost rose from 1,749,591 to 1,753,692, and p95 from
2,047,006 to 2,052,822. Keep the tile cache outside the release.

## Primary collision checks and fallback steering

A read-only trace from the exact shipping P10 ARM image observes the
primary `unit_step_clear` and fallback `sc64_contact_steering_query` calls
from `move_toward`. It covers tape calls 8,400–8,463, including 56
advancing updates. It records the intended endpoint and actual stock
speed. Of 2,665 aligned full steps, 2,091 fail their primary check. Those
checks cost 2,673,904 cycles, or 47,748 cycles per advancing update.
Every eligible blocked primary bit agrees with the fallback mask.

The clear primary calls average 843 cycles. Replacing primary and
fallback with one mask breaks even if a clear mask averages less than
5,502 cycles before additional dispatch cost. The observed masks after
blocked moves average 2,569 cycles. The tests below measure the actual
whole-update effect of evaluating clear moves through the mask.

The opt-in `SC64_CONTACT_FUSED_PRIMARY` uses the mask for an aligned
full ground step where the existing layer-reach proof is available.
Partial steps and passes with cached steering answers use the prior
path, preserving memo ordering and admission. The existing terrain
checks and fallback direction ordering stay in place.

| Timing | Release | Full-step trial | Same-bucket gate |
| --- | ---: | ---: | ---: |
| P7 mean loop | 1,749,591 | 1,730,083 | 1,733,216 |
| P7 p95 loop | 2,047,006 | 2,024,222 | 2,026,565 |
| P7 peak loop | 2,438,394 | 2,408,574 | 2,415,098 |
| Blood Bath opening mean | 561,488 | 561,488 | 561,488 |
| Blood Bath late mean | 999,819 | 1,021,908 | 1,017,064 |

Both trials match the 11 sampled P7 worlds, palette and framebuffer
pairs, and all timeline rows. The same normal three-AI Blood Bath setup
matches 32 world, palette and raw framebuffer pairs. The same-bucket
trial shows a different page selector at frame 16,601 with the same raw
page contents. Late 1,000-update cost rises 2.21% with the unrestricted
trial and 1.73% with the bucket gate. The latter passes 141,817 aligned
and partial primary-step comparisons under the existing contact oracle.
Keep both trials outside the release.

In the exact P7 trace, 1,471 of 1,626 aligned calls that follow a blocked
primary for the same actor slot and raw type are also blocked. This is a
90.5% observed precision for an offline predictor. It leaves 620 other
aligned blocked calls. Measure that prediction on other workloads before
adding a predictor to the engine.

## Earlier rejection inside the primary scan

`SC64_CONTACT_EARLY_DIRECT` reuses the contact layer reach proof in the
original endpoint scan. With a proven radius bound, a candidate outside
either endpoint axis can be discarded before reading its actor facts.
The same two offsets are reused by the original distance predicate.
Unproven layers use the original acceptance rules. This change keeps
the primary and fallback scan counts and ordering.

| Timing | Release | Earlier primary rejection |
| --- | ---: | ---: |
| P7 mean loop | 1,749,591 | 1,728,678 |
| P7 mean orders | 831,114 | 810,395 |
| P7 p95 loop | 2,047,006 | 2,023,642 |
| P7 peak loop | 2,438,394 | 2,418,393 |
| Blood Bath opening mean | 561,488 | 561,488 |
| Blood Bath late mean | 999,819 | 998,765 |

The P7 mean falls 1.20%, with 1,000 matching timeline rows and 11
matching sampled world and framebuffer pairs. Blood Bath matches all
32 world and framebuffer samples and the per-update route counters.
The prepared ASan/UBSan oracle passes 4,138,459 step checks and 637,410
active-mask checks. The trial P10 link uses 168 more IWRAM bytes, ending
at `0x03004e20`, with unchanged EWRAM. All 40 private inputs match.
Keep the change opt-in until the other canonical links and representative
ordinary controller replays have been checked. These are emulated cycles.
In a prepared checkout, use
`make CC='arm-none-eabi-gcc -DSC64_CONTACT_EARLY_DIRECT' verify-protoss-p10`
for the trial. The isolated source reproduces its verified ROM byte for byte.

| Local receipt | SHA-256 |
| --- | --- |
| P7 baseline and optical tile comparison | `567a72974cce8be2f48616a5688651147dc685ec0c5ec3bd7cc9526a5ca17bd4` |
| Exact primary and fallback trace | `f8152da350c18791980891ded636158abb8dc80a27d4e51498b2ad626f383265` |
| P7 baseline and full-step comparison | `7f7666b604602356658900ac2617f327673eb93df820d32e2157553294e0f9b6` |
| Blood Bath release | `6a25f6e7863f04643bf1aa6553a45eb8ef9f0038a539add3b31b126869251617` |
| Blood Bath full-step trial | `8333cf3b14d36fda924cf999344b84f67ba856857727456a4234468f035e84a1` |
| P7 baseline and same-bucket comparison | `00cf5c28e2981585a2c5ca1a91f888d994b3ada87e1e00ab151d0fec04d05f3e` |
| Blood Bath same-bucket trial | `9516d00268459921f03188ba0146e27fd8f3fa51fed64175e51dc365f5ab55fa` |
| P7 baseline and earlier primary rejection | `edcf47a76a4a806b4b1def8958700b43a7e9f179c69e75af1b911daa41a3ee08` |
| Blood Bath earlier primary rejection | `435d3afe76c5328025ebaa955bae02ff0000bd0f77a363f1b0a71368372947d2` |
| Earlier primary rejection ARM ROM | `7e03188adcc9f1b7c1759a684e2c708d2b98d650109d003f9b7ee9d4e4829ff2` |
