# Z7 scenario prevalidation experiment

The default-off `SC64_Z7_PREVALIDATED_TICK` path avoids rescanning immutable
scenario metadata on every source-condition evaluation. Authenticated map
initialization and save rebind still validate the complete tables. Only then
does the private bound world receive a runtime capability to use the faster
callbacks. Failed initialization or rebind clears that capability and its
callbacks.

The ordinary checked APIs retain metadata validation. The faster path retains
the live actor/owner/type census and persistent-state, cursor, timer and action
checks. The capability is not saved; actor capacities, save generation 36 and
RAM reserve guards are unchanged. Standard builds leave the feature disabled.

With the complete Z6 inputs and Z7 scenario prepared as described in
[Development](DEVELOPMENT.md), build and test the isolated candidate with:

```sh
make -j2 test-sc64-z7-prevalidated
make zerg-z7-prevalidated-probe
```

The four host checks exercise reference/candidate runtime and save/load paths
with ASan/UBSan. The candidate ROM is
`build/StarCraft-Zerg-Z7-Prevalidated-Probe-GBA.gba`, title `SC Z7 FAST36`, code
`SCZV`. It also enables the existing Egg, source-sight, Scourge, construction
HP, direct-attack and nearest-depot options. It remains a development probe.
No ROM, scenario table, game art, save or capture is included in Git.

## Evidence on 2026-10-25

Paired 1,500-update ARM neutral openings averaged 13.71 fps with the gate off
and 14.93 fps with it on. This is about 8.9% higher effective fps and 8.2%
fewer emulated hardware cycles for that interval. Normalized entity and
accounting snapshots matched at updates 0, 1,000 and 1,500. This bounded
opening is below the 30-fps goal; it is not a mission-wide or physical GBA
performance result.

A separate private host replay supplies only the held/pressed input stream
from a normal-menu controller attempt for 12,000 updates. Reference and
candidate full-state hashes match at all twelve 1,000-update checkpoints.
Coverage includes live and dead actor/building rows, minerals, geysers, fog,
campaign/runtime state, resource and supply accounting, RNG and mutable world
context. Only performance caches/counters and the capability byte are excluded.
The replay includes combat, construction, economy and source AI activation;
it ends during play and does not establish a win.

Focused tests cover public checked-API rejection of forged rows, invalid binds
and rebinds, live actor ownership faults, startup, save/load and rejected saves.
Reference and candidate save tests produce identical output. The gate-off ROM
also matches the pristine baseline byte for byte. An independent source review
found no actionable defect in the changed paths.

The paired target passed with the full candidate profile, including paid Egg
production and larva consumption. The original non-Egg runtime target passed
separately. Default build/verification, all 414 asset-free Python checks and
their host fixtures, and the unified build/tests also passed.

The measured candidate ends IWRAM at `0x03004a40` and BSS at `0x0203dbd8`,
within the unchanged reserves. No completed Z7 playthrough, fresh-process ARM
SRAM continuation or full-campaign qualification is claimed by these checks.
