# Z3 from HOME on two normal Z10 cartridges

The Protoss Sighting won from fresh HOME on the exact November 13 Z10 ROM
(`cbba1c5`) and the staged Z10 ROM (`c906058`). The same [route-pure host tape](2026-11-12-normal-campaign-replay-gaps.md#carried-z3-controller-reads-no-longer-alter-the-route-cache)
ran through the normal menu with physical mGBA buttons, no SRAM input and
no guest writes. Three authored victory transmissions took six extra
physical calls, while the simulation frame was frozen.

All 465 named checkpoints matched, and the two checkpoint files were
byte-identical, including the authored outcome and
presence condition. Victory came at frame 136,588 after 139,009 tape
calls and 139,015 physical calls. The Chrysalis retained all 250 HP and
five enemy actors remained. The replay averaged 21.10 emulated main-loop
updates/s. This is one ordinary route, with no physical GBA or SRAM claim.

| Input or receipt | SHA-256 |
| --- | --- |
| Held-key tape | `bba07b5bef6a1c13ee1b39f01213f67d759002a7b4f47fd70f6bc11b1ff0644c` |
| Physical mGBA observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Replay harness | `d6e2373e1d171dbecfc434c4918bf20de6b1a5d79a50e318c54e075f088c4d92` |
| KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| Matching KODAK ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| KODAK receipt | `82bb567cc126a1be53a13478bbd998b36129f867cf5850424ee1ed5bb546f181` |
| Staged Z10 ROM | `ef36f3f0e8f44ea1207ef39c05ac1417c727b498a2596e93b98b765c74b9768e` |
| Matching staged ELF | `2cce15b2607008b1cf293792c63b4253dc0b39debf7ef27f526e37dcc9e23d72` |
| Staged receipt | `a5293d6baaad192b5471cfa6371ba8651faf64cd6aa3b4a3d9688e51a9ba7290` |
