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
