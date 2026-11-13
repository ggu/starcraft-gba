# Z5 on the exact KODAK Z10 cartridge

This replay uses the normal Z10 ROM built at runtime
`cbba1c5c72b0f1043eb330cb2cccd520b855c2ba` and delivered to KODAK.
It reuses the independent [Z5 route-pure HOST tape](2026-11-12-z5-normal-physical.md)
without changing its original controller transitions. It starts at fresh
HOME in an isolated mGBA process, with no SRAM input and no debug
commands or writes to game state. The physical backend supplies
buttons; guest observations use read-only queries resolved by the
exact ROM's ELF. No cartridge was rebuilt.

The replay checked all 55 tape checkpoints, across 20 named fields
including the 14-field Kerrigan record, then verified the final mission
snapshot. Z5 won at frame 48,347 with Kerrigan alive, Switch 110 set,
campaign completion recorded and no adapter faults. It used 48,803
tape calls and 48,843 physical updates. Twenty authored text pages
each took two extra physical updates with the simulation frame and
source-pause flags held. Separately, the tape's original A press at
call 47,351 acknowledged source `PauseGame` at frame 46,895, changing
`(1,0)` to `(1,1)` without advancing that frame.

The retained driver is the earlier `physical-z5-current.py` with
paths and artifact pins set to the KODAK pair and the port-free
`--physical` observer. The replay logic and observations are unchanged;
this run required 5.25 GiB free at startup and 4.15 GiB during playback,
with a 256 KiB per-file cap. No replay artifacts or game data are
tracked here. This qualifies one ordinary winning route on the exact
cartridge image; it does not qualify save/resume or physical hardware.

| Artifact | SHA-256 |
| --- | --- |
| HOST tape | `3bd6863852c2b273e356a23197687313473ce60025bb8a922f058d475fc54d63` |
| KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| KODAK Z10 ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| Port-free observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Replay driver | `0e0ced93fe159c02b11a3225f5310756f0fa0f1be7bb0ec9935e1c2a4cf3e95a` |
| Replay receipt | `142c147a5e5bdf658c39f24cef1c96caf558f627f27b01a26a83553e090f1525` |
