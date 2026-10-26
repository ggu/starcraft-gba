# Z7 steering raw-density preflight

The Z7 steering-list builder now counts raw unit-bucket nodes in its bounded
union-cell window before looking up mover collision facts or writing candidate
entries. When the count exceeds the existing 48-entry capacity, it returns
`0`; `sc64_steer_best_direction()` then uses the existing `unit_step_clear()`
path for each candidate offset. This raw count may include the mover, dead,
loaded, flying, or otherwise filtered actors. Those only cause conservative
fallback to the original query. P7 and other non-Z7 builds retain their prior
builder path.

The threshold is unchanged: 48 raw nodes continue through normal candidate
filtering; the 49th selects fallback. No storage, geometry, direction order,
collision predicate, or save data changed.

## Evidence

The ASan/UBSan Z7 steering fixture passed with the late-public-profile compiler
flags. It checked the existing live Z7 and dense mixed-unit step oracles, exact
48-node acceptance, a filtered 49-node Overlord crowd, dense crowds at all four
map edges, gathering-worker fallback, capacity fallback, collision-class slot
reuse, and cache rebuild after SRAM load. Fallback cases use the original
`unit_step_clear()` path. The fixture reported 392 queries, 5 blocked answers,
49 successful builds, and 7 fallbacks.

A private ARM replay loaded the genuine Z7 continuation save at frame 89,340
(`started=250`), ran 2,000 idle ticks through normal HOME/L load, and matched
all 20 explicit digests through frame 91,340. Neither run faulted, won, nor
lost. Mean cycle counts were:

| Replay | Update | Draw | Full |
| --- | ---: | ---: | ---: |
| Caller-owned steering-list baseline | 7,188,545 | 435,796 | 7,624,342 |
| Raw-density preflight | 7,014,808 | 439,873 | 7,454,681 |
| Change | -2.42% | +0.94% | -2.23% |

These figures describe one 2,000-tick idle continuation only. They do not
establish mission-completion performance, frame rate, or hardware behavior.

The corresponding private standalone link passed the existing linker guards:
BSS ended at `0x0203dff0` (16 bytes below the limit), IWRAM at `0x03004c60`
(928 bytes below the limit), and ROM load data at `0x09ead690`. The final public
product target passed the same bounds and the ROM header/SRAM verifier; its
logs and hashes are retained under
`.local/build/z7-live-20261027/preflight-public-final/`.

## Rejected candidate

A separate private eight-bit blocked-offset mask experiment passed its
step-answer and late-digest checks but regressed the measured continuation:
mean full cost rose from 7,624,342 to 10,289,205 cycles (+34.95%). It is not
part of this change.

## Reproduction artifacts and limits

The private prototype used the same Z7-only branch body behind a temporary
compile define. The published fixture command and its output are retained under
`.local/build/z7-live-20261027/steering-preflight-public/host/`. The private
prototype source snapshot, replay ROM/ELF, seed hash, checkpoint log, standalone
link map and metrics are retained under
`.local/build/z7-live-20261027/late-blocked-preflight/`; the report is
`programmatic-preflight-report.json`. The ARM evidence is an emulator replay
of a saved continuation, not a complete campaign or a physical GBA test.
