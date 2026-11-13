# Z6 on the exact KODAK Z10 cartridge

The normal Z10 ROM delivered to KODAK, built at runtime
`cbba1c5c72b0f1043eb330cb2cccd520b855c2ba`, ran the
[independent Z6 route-pure HOST tape](2026-11-12-z6-normal-physical.md)
from fresh HOME in an isolated mGBA process. The replay sent ordinary
buttons through the port-free physical backend and read guest state
through the matching ELF. It used no SRAM input, guest writes, debug
commands, or new cartridge build.

All 65 checkpoints and the final mission snapshot matched the tape.
Four checkpoints observed Kerrigan embarked as one cargo unit in a
researched Overlord. She finished alive and unloaded at (951, 1010)
at frame 56,913, with source victory and campaign completion recorded
and zero adapter faults. Force 2 counts in locations 4 and 5 were 0
and 4, respectively. Eight authored text pages added 16 physical
updates without advancing the simulation frame, for 57,578 physical
updates across 57,562 original tape calls.

The retained `physical-z6-current.py` driver was reused with the KODAK
ROM/ELF and observer pins, a 5.25 GiB startup reserve, a 4.15 GiB
playback reserve and a 256 KiB per-file cap. The replay logic and
observations were unchanged. This verifies one ordinary winning
route on the exact cartridge image. SRAM continuation and physical
hardware remain outside this result.

| Artifact | SHA-256 |
| --- | --- |
| HOST tape | `32460d8296bb322d935c3a9f8de935885cdfbe52ff3eca09a901e623d6729769` |
| KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| KODAK Z10 ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| Port-free observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Replay driver | `252a6e6a5694663709c7be19f841df43a4ec5de0ec336191a5e716e989635c89` |
| Replay receipt | `40d2f520d10a742c1340f13f4fa5fe87fa2908dc3968238c3873ed7dc5740d70` |
