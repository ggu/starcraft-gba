# Current-release worker cargo integration

Workers retain mineral/gas identity through MOVE, ATTACK, STOP and construction
travel, and can return their old load before starting a new gather order. SCVs
and Probes retain cargo through construction; a Drone is consumed when its
building commits. Repair still uses `carry` for a fractional repair remainder
and discards resource cargo. This is not complete native worker-order parity.

The combined runtime tested here is `1ec3fadc5df079d7f38d0b73b85cf0b08c84b7f2`,
on release `2a8b4ee4`, including the P10 default and Arbiter optimization. The
cargo capability is shared by the current Terran, Zerg, Protoss and Showcase
products and their applicable campaign/skirmish modes. Earlier explicit legacy
profiles remain separate. No Unit or serialized-row growth is introduced; the
P10 fixture payload is 16,332 bytes.

## Save and fixture boundaries

The cargo tag toggles `0x01000000` with XOR because that bit is already set in
historical scenario tags. Transport and Battlecruiser features retain their own
namespaces. Legacy/current selection validates a scratch candidate, and the
selected-journal writer preserves the previous valid record. Migrated cargo
whose kind cannot be inferred is marked unknown rather than paid as a guessed
resource.

Malformed-candidate fixtures explicitly separate independent synthetic HOST
journals after checking rejection and an unchanged world. They first assert
that the production writer refuses the bad committed record. Foreign-record
preservation is checked before teardown; wrong-generation negatives remove an
older valid fallback before testing that separate case. These are fixture
boundaries, not a weakened production overwrite guard.

## Combined local qualification

All checks use the existing pinned extractor
`09b57f787a247107d1de82b8c5eab2734667ea1d` and verified copies of private inputs.
No hosted Actions were dispatched.

- `./dev sc64 test --profile terran`: all ten Terran controllers and profile checks.
- `make -k test-zerg-z7`: all seven carried Zerg missions and profile/save checks.
- `make -k test-sc64-protoss-px`: all ten Protoss host wins and their continuations.
- `make test-sc64-p10-carried-profile`: 234 startup/save configurations.
- `make -k test-sc64-zerg-z8`: separate candidate win at frame 39,617 / call
  40,335, zero mission faults, 24/24 host continuation observations matching.
- Actual T10, Z7, P10 and Showcase renderer vectors pass the unmodified cargo
  renderer fixture under ASan/UBSan, including kind, clipping, fog, nonworkers
  and T6 repair handling.
- A clean unprepared checkout passes source boundary, doctor, default build and
  full tests: 953 Python tests (26 skipped), separate 5- and 8-test suites and
  native fixtures. It remains independent of imported game assets.
- All six normal ARM products (T10, Z7, P10, Showcase, Z9, Z10) link and pass
  their applicable ROM/memory checks. Late Z9/Z10 BSS ends at `0x0203dff8`,
  leaving eight bytes below the unchanged guard; IWRAM ends at `0x03004bf0`.

The P8 changed-route ARM full replay, physical SRAM prefix and fresh-process
resume pass: 95/95, 32/32 and 64/64 host observations match, with cutoff tick
31,374. Those retained logs also pass the stricter comparator from `26c3f88f`.
Its sampled digest does not include worker flags/timers/depot identity, so this
is not an exhaustive cargo-state comparison. P10 and Z7 ARM gates remain open.

P10's dedicated replay initially truncated long log lines before their checksum.
That partial run is not qualified. Instrumentation-only commits `4f7b6df4` and
`26c3f88f` move its digest first, include worker flags/work timer/depot identity,
shorten optional counters and require a complete end sentinel. The comparator
rejects missing/invalid checksums and incomplete P10 observations; 23 asset-free
comparator tests pass. Production game sources and the six normal ROM products
remain identical to `1ec3fadc`; only replay source, its reader and tests changed.
P10 is being re-run with this instrumentation before a merge decision.

## Production SRAM cases

T10, Z7 and Showcase production ROMs pass ordinary-input 60-update play plus
60-update fresh-process continuation cases. The P10 migration case starts with
an unchanged legacy SRAM file produced by ordinary controls on release
`6417737c`: map 1, human Protoss, one Zerg opponent, 300 updates then pause/save.
Legacy SRAM SHA-256:
`76a533126c9da74472fed9f0ba36d2cfbd903acc84696f68e47bee3855543c75`.

The combined cargo P10 ROM
`4c2e61f164e9244ba2957578b960aac5cd735fc4bc7fc7b4aabe9b9c4cbe4a72`
loads that original file through HOME/L. Re-saving publishes slot 1 / sequence 2
while preserving legacy slot 0 / sequence 1 byte-for-byte. A fresh process
matches the captured continuation after 60 candidate updates. This one migration
case does not exercise carried gas/unknown flags, P7-to-P10 campaign migration,
or an exhaustive world comparison. No guest-memory writes or forged SRAM were
used. The recorded input hashes remain unchanged.

## Bounded ARM timing

Same-source cargo OFF/ON comparisons use the existing synthetic crowd probe on
default skirmish map 0. It adds/commands actors at boot and then runs AI. This is
not Blood Bath, a loaded-worker scene or a whole match. Each case has 500 samples;
initial hashes and all exposed non-timing counters/world hashes match. It does
not compare every world field or framebuffer.

| Probe | OFF mean frame cycles | ON mean frame cycles | Change |
| --- | ---: | ---: | ---: |
| Z7, 96 actors, three AIs | 1,169,230.79 | 1,171,483.19 | +0.193% |
| P10, 48 actors, two AIs | 879,380.24 | 881,065.71 | +0.192% |

The receipt SHA-256 is
`f16ed5a7b485598db372dad9d054afb70858b2a00f404f8b186f5c26c6d3ab79`;
2,121 copied input hashes were checked after each operation. The separate
production-guard/SRAM run checked 1,796 inputs unchanged, and the campaign suite
checked its 1,734-file SC64 manifest unchanged. Different manifests cover
different input roots; these counts are not claimed as one identical set.

These measurements do not establish 30 fps, physical GBA behavior, full native
parity or qualification of the unpromoted Z9/Z10 missions. ROMs, generated banks,
captures and saves remain private.
