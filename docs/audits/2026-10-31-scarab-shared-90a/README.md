# Scarab namespace on current shared integration

The integration merges shared `90a3a7a83dcee3662d043d2d3c89ba6e975d2a37` into Scarab `bc35bca0db8a4b8e59751db6907e927495e7a208`. The rejected backpressure experiment is not an ancestor. These checks concern the disabled-by-default experimental namespace and do not admit the Reaver feature.

With the full Scarab probe and namespace enabled, the existing `--morph-hp 4` fixture and new `--ai-sunken-save` fixture each passed on both maps under Terran, Zerg, Protoss and Showcase codecs. The human fixture starts the ordinary paid morph. The AI fixture uses synthetic initial structures, then calls the actual defense planner to start a morph, damages the colony and saves during construction. Both reload and compare neutral continuation; the AI fixture additionally checks source type, owner, HP and maximum after reload, then the completed Sunken HP and continuation hash. This is host evidence, not a claim of ordinary unassisted AI gameplay or native hardware equivalence.

`make -j1 test-scarab-deadrow` passed all four profiles and both maps, including post-impact continuation, live-outside and pending-marker save refusal, and checksummed malformed-dead fallback. These checks do not establish whole-world load atomicity or support for saving live outside Scarabs or pending packets.

The opt-in command `make -j1 test-scarab-sunken` builds those probes from prepared local inputs and generates any P7 facts into a new ignored run directory. `make -j1 test-scarab-deadrow` exercises the post-impact cases.

All four normal gate-OFF ARM builds and all four full-probe ON ARM builds passed the existing linker guards. The ON BSS/IWRAM endpoints were Terran `020370d0/03004e68`, Zerg `02037144/03004f68`, Protoss `020382fc/03004c38`, and Showcase `0203df50/03004ff0`; Showcase has only 16 bytes of IWRAM guard margin. These are size checks, not hardware performance evidence.
