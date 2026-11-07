# Modal outcome publication

At baseline `47bf5c50`, actual-engine HOST fixtures reproduce both failures: T1
queues its Raynor defeat row and immediately publishes loss/result plus a
journal; P2 queues its Cerebrate victory row and immediately publishes victory,
completion and a journal. The fixtures construct end conditions on
HOST and exercise the real update path; they are not ordinary campaign wins.

Runtime `48b39f40` retains the posting tick but defers public won/lost,
campaign phase/objectives/completion and final result/save until every queued
page is acknowledged. Acknowledgment is not forwarded to gameplay. Direct saves
refuse unsaved transient state; failed loads retain it, while successful loads
and new games clear it. Deferred metadata uses 12 bounded EWRAM bytes; queue VRAM
and serialized layouts remain unchanged. Covered-row reporting distinguishes
deduplication from missing/full queues for a separate presentation follow-up.

Fresh-process ASan/UBSan actual-engine cases pass for T1 loss, P2 victory,
failed/successful load, new game, and the original modal/direct-save fixture.
The final integrated regression additionally requires unchanged phase/objectives
while pending and publication on acknowledgment without advancing simulation.
Independent source review found no blocker for supported text-enabled race
products. Two bounded input-only mGBA UI probes routed normally to T1/global4
and P2/global3, displayed 232-pixel panel edges, froze during acknowledgment and
resumed to frame 200. They are UI evidence, not terminal outcome replays or
physical-hardware qualification.

Six normal ARM ROMs build and pass existing guards at `48b39f40`: T10, Z7, P10,
Showcase, Z9 and Z10. Their final compiler vectors are unchanged after test-only
integration. Z9/Z10 end at EWRAM `0203dce4` and IWRAM `03004bf8`, leaving 796 bytes below
the unchanged EWRAM guard. Available margin decreases from 808 to 796 bytes;
the guard threshold was not weakened. The separate unprepared `664ab07a`
checkout passes source, doctor, build and test: 988 Python tests, 26 skips,
separate 5/8 groups and native fixtures. Its original-art GBA ROM is byte-identical
to the previous qualified default (SHA-256 `9fe916dcb7bd5aeb06476601847e4feebf6df2b2a2499554f8c16248b689a858`).

At `19d56d59`, the integrated modal regressions and canonical Terran/Z7/P10 HOST
suite commands all exit 0. The legacy split-opening fixture initially failed its
old two-building assumption; exact baseline `47bf` reproduces that failure. It now
checks one starting hall, no free Depot/Pylon, four workers, doubled supply 8 and
capacity 20/18, matching the previously qualified opening policy. No runtime
opening rule was changed here.

The profile wrapper initially exits 1 on its final broad hash comparison: its
snapshot includes 38 regenerable match logs/JSON outputs and the canonical Z7
controller trace. Classification retains that failure and every output. All 1,921
other files, including all compiled generated headers/bin data, remain unchanged;
the original 1,958-file reference snapshot is intact. The regenerated Z7 trace is
42e9a86021b3a16f6f67d2e19f3e475c4c98096799ce1abdc652f9117359696a,
the same trace already qualified in the carried ARM audit. No guest SRAM was
constructed or patched. Logs, journals, imported graphics and ROMs remain private.

The existing post-ack presentation holds remain unchanged. Removing those holds
retimes source trigger execution and has a separate adapter/controller/ARM
qualification. The known full Showcase last-stand tick-131-versus-150 assertion
also remains outside this fix. These results do not establish 30 fps, native N64
motion/AI parity, arbitrary historical-save compatibility, or all campaign routes
on physical GBA. Prepared `make unified test-unified` also passes at `19d56d59`, with all 1,921
non-output snapshot files unchanged. No Actions or SD writes are part of this fix.
