# P7 physical replay preparation (not yet run)

The pinned host controller trace was produced by a policy using world-state observations; replay supplies its already recorded ordinary held-button transitions only. No guest input/debug memory or SRAM is injected. First run only the 1,000-update prefix; a full run is a separate admission decision.

* Trace: `ca2d6f422064a84340f588e760f17171b37dd1dffcd0632d7939315cfacdde89` (`p7-origin-corrected-trace.jsonl`, tested source `5d4376b`).
* Existing P7 Transport1 ROM: `378ab490ea78915da2d0f6e190f0db5dff003a685e1f39e1c8444c69fb6bd795`; ELF: `17e8398093c91a2ef1a0922e2cdbc0e9391455abdf822ae2d7e5ddcd0e2001fb` (built from `96108b2`).
* Existing physical K/U backend: `166c017b227d3611a751aa0cce7a4179cbe3077b025a9aa108d1dc2ed2e79a20`.
* `0ff3ffb` versus `96108b2` is not a literal empty runtime/Make diff: Z7 flag and Z7/Z8-gated sight changes were merged. Source review finds those gates inactive for P7; binary equivalence to a newly built 0ff P7 ROM has not been measured.

The parser checks the pinned trace hash, increasing zero-based input indices, 10-bit masks, opposing directions, nonredundant transitions and pressed edges, terminal verdict and exact host call bound. Host `input()` increments its counter even when terminal state causes it to skip `game_update`; the final neutral transition at call 49968 and the following call are not sent. Full replay stops after 49,968 actual updates, not 49,970 host calls; the skipped-call mapping requires physical confirmation, not an assumption of success.

The launch observation is host call/update 12 with game frame 1. Host transition at index 999 advances game frame 962→963, so completed update 1000 is a bounded second comparison point. Other host checkpoint records do not contain call indices and are not automatically assigned to nearby transitions. P7 persistent, runtime, model, unit and compact-building fields are decoded using ELF DWARF and read-only symbols; full qualification checks actual result, source-owner Nexus state, three authored hero identities and scheduler outcome/fault/switch/timers. Snapshot fields and checkpoint comparisons are bounded; they are not whole-world equality. The driver and its helper sources and inputs are hashed before/after the run.

No ROM was launched and no prefix or WIN is claimed by this preparation. The driver must receive peer review, lock coordination, free-space and pinned-input preflight before use. A P7 host test win, P8 candidate status, P6 missing-bank gap, all-mission and hardware-performance limitations remain unchanged.
