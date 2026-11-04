# P7 order-stage diagnostic (2026-11-04)

A private diagnostic ROM built from runtime source `88c17f15e71a21476c6e93dd6817bbd2626c0983` and the P10 compiler vector replayed the retained P7 ordinary-input controller tape through call 9020. The selected window is calls 8020–9019. Of these, 958 updates were in play and advanced the game frame; the other 42 were excluded from the figures below. Host reference observations at calls 8000 and 9000 report mission 26, frames 7774 and 8732, no outcome, and 157 enemy units. The controller used a host world-state oracle; the replay itself supplies only held inputs. It is a single nonterminal emulator window, not a complete campaign, all-race, physical-hardware or near-30-fps qualification.

The opt-in diagnostic replaces the main loop and measures guest timer cycles. It adds the existing tracer and order counters, plus aggregates for advancing play updates. The first build, with tracer counters only, averaged 2,415,533 full-loop cycles and 1,354,632 order cycles; its largest active full-loop and order measurements were 3,958,370 and 3,007,175 cycles, both at tape call 8347. A second build also timed order subscopes:

| Active-play scope | Mean cycles | Maximum cycles | Maximum call |
| --- | ---: | ---: | ---: |
| Full update, audio and render | 2,462,783 | 4,185,980 | 8347 |
| Orders | 1,400,633 | 3,057,759 | 8347 |
| Order setup | 758,891 | 1,031,264 | 8443 |
| Movement | 409,052 | 2,699,554 | 8347 |
| Nearest-target query | 84,380 | 102,001 | 8672 |
| Map render | 366,859 | 653,151 | 8433 |
| HUD | 124,900 | 387,474 | 8024 |
| Sight | 63,751 | 304,789 | 8537 |

Scopes nest and timer overhead changes execution time. Setup includes bucket setup, owner lists, target indexing and due Arbiter cache work; movement measures calls to `move_toward`. The large movement maximum coincides with the largest orders and full-loop measurements in this window. These data justify splitting setup and movement into finer counters before selecting an optimization; they do not establish which individual setup operation or route rebuild caused the cost. The two diagnostic ROMs matched all 90 sampled actor/fog/display/frame checkpoints, not every update or complete state.

The first and second ROM SHA-256 values are `a17be74029b25351cb9aad4c90a1a145c4ffd8e3c48e2388164d04e4e96d533f` and `141b9f3457253d22cf7fe94eba45433104cd6039eae15359ed770d1e43f35c1b`; the second result is `a5be785743328e4afdbdb1b24f63d5b2b523ac6a740211135d870154596646f9`. The P7 tape source and converted tape SHA-256 values are `cec9bf14ec24dda9921e6239aaa01ccbf04b1900867e721223dfc943ca353d58` and `44e68fec4bd45ef8e8df86144dcfe877dc99592f2adf99107351cfc1bb0ed3a6`. The captured ordered P10 compiler vector hashes to `cfe4696498e7d7b62f8ebc20d6ec7701c6f1538fd12f539c5f4ec999091889e3`, and the 1,195-file copied-input manifest to `b2ff1bc5db9828c5606db75be95caa0e8c00e3a9d02093f1036566b9b4ba0972`. Runtime sources match `91996275`; its Makefile differences concern target routing. The second diagnostic probe hashes to `d9508e96badbbe29df9bcfcf772688fa3451b4997325d5e71fe9698afdfb31ca`, the private builder to `7dde9bbaf67fc393f8e70b46b073b1a0ee4836c6646b4f9830ac0f0769a44da5`, and reader to `ff81b5a1a440c740a5e5b8f1ee9b88d3acc180620404228171fc87cdc1d45fa1`. Their retained local evidence is under `/tmp/sc64-static-key-evidence`; the port-free runner SHA-256 is `506fb4aecaf74b820f77c43da4ac2251868c074b8a3526e98891e21aea2b06ae`. The runner continued through two bounded guest-cycle segments to the completion breakpoint without guest writes. Builds and replays held the shared native flock and passed the 2 GiB free-space guard.

## Setup partition and reproducible probe

A third diagnostic adds `SC64_DIAGNOSTIC_SETUP` clocks inside order setup. It reports these nested subscopes over the same 958 advancing updates:

| Setup subscope | Mean cycles | Maximum cycles | Maximum call |
| --- | ---: | ---: | ---: |
| Unit bucket begin | 28,523 | 28,720 | 8903 |
| Target index end and owner lists | 16,035 | 16,050 | 8151 |
| Target index begin | 78,432 | 88,894 | 8443 |
| Arbiter cache process due | 633,359 | 905,834 | 8761 |

Total setup averaged 759,493 cycles; order work averaged 1,401,307. The full loop averaged 2,464,097 and peaked at 4,184,168 cycles at call 8347, where movement peaked at 2,693,082. This identifies Arbiter processing as the dominant measured setup subscope in this window. It does not isolate the particular cache operation, prove a safe optimization, or attribute all frame spikes to that work. The instrumented ROM's 90 actor/fog/display/frame samples match the earlier order diagnostic; this is sampled equivalence, not full-state equality at every update. Timer calls and changed code placement affect the measurements.

The setup diagnostic can be reproduced with prepared, authorized P10 inputs and the retained generated P7 PX tape. Capture the exact normal P10 compiler command as a JSON argv array, including the generated include paths and blob paths, in a clean checkout. Preserve and hash those inputs before and after. The tools require a fresh output path and do not include private assets:

```sh
mkdir /private/tape
python3 tools/sc64_setup_profile_tape.py /private/sc64_px_tape_generated.h /private/tape/tape.h
python3 tools/sc64_setup_profile_build.py /private/p10-argv.json /private/setup-build /private/tape
PYTHONPATH=tools python3 tools/sc64_setup_profile_read.py \
  --runner /private/runner-read --rom /private/setup-build/profile.gba \
  --elf /private/setup-build/profile.elf --out /private/setup-result.json
```

The probe uses tape calls 0–9019 and aggregates calls 8020–9019; it records all sampled checkpoints and the call of each active-play maximum. It uses a diagnostic-only main loop and the source guards are absent from normal product vectors. The reader continues the same process through bounded runner execution segments and performs no guest-memory writes. Serialize native builds and replay with the shared lock and check available disk space before running.

The partition ROM SHA-256 is `1221708812368b6342807eb5abd46e3ed8040d230f8e7287da2f0b2464b8e4cb`, ELF `e3cc76d0f8196e922c6ca628651e70cce6682eafb6a8161efdb1b63927ebc813`, and result `7b52bb929851db2f0334dbb29a2dfc893c4312c13460a950f12efe0f99e7ae44`. Diagnostic `game.c` hashes to `44e9a1a921f0eba284a60c9c658f2782018e380b494feaee6f839b15c37218bc`. The builder, probe, reader and tape converter are tracked alongside this audit. The copied-input manifest was rechecked after this run with all 1,195 hashes unchanged. No physical-device timing or general performance improvement is claimed.

## Detector-list candidate on current source

On source `6417737c9b354a80d6963c16af69df7a2e1946a2`, further nested clocks put mean detected-mask work at 410,196 of 634,341 Arbiter cycles in the same 958-update window. The candidate builds bounded ordered detector-slot lists only when the first mask refresh is needed in a source tick. Each detector pass retains its original live, hidden, transport, source-owner and geometry checks; all other cache and clock work remains on the existing cadence. There is no new persistent storage. Compile-time assertions bound the byte slot indices.

With the same tape and diagnostic counters, the candidate's mean full loop was 2,381,893 cycles versus 2,464,406 on baseline, and detected-mask work was 299,571 versus 410,196. All 90 actor/fog/display/frame checkpoints matched. Diagnostic timer calls and code placement affect these figures; they do not establish uninstrumented or physical-hardware speedup or a 30 fps result. The candidate and baseline are distinct ARM ROMs built from the same ordered vector and snapshot inputs.

An independent host differential fixture compiled against unmodified source and the candidate produced byte-identical Unit arrays, Building arrays, P7 persistent state and cache initialization/due bits across 1,920 randomized lifecycle steps, SHA-256 `69ab1dd3375de26efacaf55833eb5bb89d00a57562e0975e6ced3e82334aee49` for each 39 MiB output. It exercises index fallback, death, reuse, transported records, malformed cache words, source-owner fallback and due/non-due/reset clocks; it is not exhaustive. The independent original owner-mask oracle passed 2,000 randomized cases under ASan/UBSan, and both randomized aura-union variants passed. The aggregate P7 host suite stopped at `test_battery_candidate_guards` line 103; the unmodified `6417737c` baseline with the same copied inputs fails the same assertion.

The ordinary P10 candidate ROM built and passed its verifier with SHA-256 `7859f5454d81acf0005341597759eaee0ce289f51ee6adeb7288175fcc0bd5f9`, `__bss_end=0x020394f4` and `__iwram_end=0x03004a58`. The instrumented P10 GCC stack report records 1,008 bytes for source tick, 8 for process-due, 88 for update-orders, 224 for game-update and 384 for the diagnostic main; this is not a complete worst-case call/IRQ stack bound. Other production vectors, a full stack/IRQ bound and uninstrumented ARM comparison remain required before release or merge. No hosted Actions were run.
