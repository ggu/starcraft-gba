# Z4 from HOME on two normal Z10 cartridges

Agent of the Swarm (Z4) won on the November 13 KODAK Z10 cartridge
(`cbba1c5`) and the staged normal Z10 cartridge (`c906058`). Each run
started at fresh HOME and replayed the same held buttons from the
[route-pure host controller](2026-11-12-z4-normal-physical.md).
The replay acknowledged 14 authored pages with physical presses and
releases while the simulation frame was frozen. It used no SRAM or guest
state injection.

Both runs reached victory after 35,186 tape calls and 35,214 physical
calls at frame 34,984. All 42 checkpoints matched across 38 fields and
the two checkpoint files were byte-identical. Kerrigan survived, the
three hatch effects fired, the hostile Force 3 Command Centers were
destroyed, and there were no trigger faults. This verifies one ordinary
route and does not establish hardware speed or save continuation.

| Input or receipt | SHA-256 |
| --- | --- |
| Route-pure held-key tape | `86a2e728e365361d86e26ef194c41f5d2768647e8d168d78c1911c8f6a563abc` |
| Port-free physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Replay harness | `11c30189dba030faa4baa4434a6b107f18d09464d0c36c8e96e3eaacf653a132` |
| KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| Matching KODAK ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| KODAK result | `92dcbbf6b5738fabbf87abeaf20569721e5baf8a61efa3856eb3704c7cf10dae` |
| Staged Z10 ROM | `ef36f3f0e8f44ea1207ef39c05ac1417c727b498a2596e93b98b765c74b9768e` |
| Matching staged ELF | `2cce15b2607008b1cf293792c63b4253dc0b39debf7ef27f526e37dcc9e23d72` |
| Staged result | `663f766058e0731e51437d4acb1e7cee9e06b82b9bf2a9d948a054f651eda8f7` |
