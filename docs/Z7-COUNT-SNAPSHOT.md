# Z7 condition-count snapshot experiment

The default-off `SC64_Z7_COUNT_SNAPSHOT` option combines the five canonical
actor-count predicates into one census per source scheduler update. It
requires `SC64_Z7_PREVALIDATED_TICK`. Only the authenticated private callbacks
use these counts; public checked queries still evaluate the world normally.
Counts are discarded after the tick and invalidated on initialization/rebind.
All source conditions are resolved before any source actions execute.

The bounded cache uses 24 bytes of runtime-only context, with no serialized
fields, capacity changes or save-generation change. Standard profiles leave
both experimental gates disabled. With the existing private Z7 inputs:

```sh
make test-sc64-z7-count-snapshot
make zerg-z7-count-snapshot-probe
```

The separate ARM probe is
`build/StarCraft-Zerg-Z7-Count-Snapshot-Probe-GBA.gba`, title `SC Z7 CNT36`,
code `SCZW`. The host target runs paired prevalidation runtime/save fixtures,
the count-enabled runtime/save fixtures, and focused source-query checks.
Coverage includes canonical row identity, checked-API independence, invalid
owners, rebind invalidation, region rectangle boundaries and a surviving
building preventing the source Any Unit defeat predicate.

## Evidence on 2026-10-25

The staged base is `fad7926` plus the prevalidation changes in
`codex/z7-prevalidated-tick`. All 414 asset-free Python tests and host fixtures,
default build/verification, unified build/tests, and the seven focused
ASan/UBSan executions passed. A private 12,000-update ordinary-input host
replay matches full-state hashes at all twelve checkpoints, including combat,
construction, economy and source AI activation. Its output SHA-256 is
`3c00cb16331e010162ac682f71657d20a371bb480a45c2273668d0ab17407ad5`.
The count-disabled prevalidated ROM remains byte-identical to its baseline.

A separate private, self-reporting ARM pair retains the ordinary update,
render and display-deadline loop. It uses the verified normal-menu input
prefix, then neutral input. mGBA writes reports to separate private log files;
it does not connect to another emulator's debugger. Reporting occurs outside
the timed loop. Both builds confirm global mission 18, authenticated Z7
runtime/bindings and zero runtime fault. They have the expected 113 units,
57 buildings, 60 mineral rows, one neutral geyser and five enemy Extractors.
The candidate records 7,440 successful count-cache lookups by update 1,500.
Reported actor, resource, runtime and mutable context hashes match at updates
0, 500, 1,000, 1,500 and 1,512.

Between updates 500 and 1,500, measured update/render cycles fall from
1,123,132,978 to 842,233,196: 25.01% fewer cycles, equivalent to approximately
14.94 to 19.92 effective fps at the GBA clock. This is a bounded instrumented
opening, not standard-ROM, mission-wide or physical GBA performance. An
earlier pair selected mission 17 and was rejected as Z7 evidence.

The production probe adds 1,568 bytes of text and 24 bytes of BSS. IWRAM ends
at `0x03004d38` and BSS at `0x0203dbf0`, within unchanged reserves. No Z7
mission win, fresh-process ARM SRAM continuation or campaign promotion is
claimed. Later parent-branch changes require integrated qualification.
