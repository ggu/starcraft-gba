# P10 early collision rejection

The current release already rejects coincident positions using a packed
coordinate read. The P10 IWRAM kernel now also rejects an actor whose
offset lies outside a proven collision reach. Retain the original
checks whenever that reach cannot be proven. Terran and Z10 use the ROM
kernel and retain their existing behavior.

## Current release comparison

The baseline uses release `f924b9df`. The canonical P10 candidate uses
`a21ce275` with the same 24 pinned prepared files. The candidate leaves
648 bytes below the IWRAM link guard and 8,484 bytes below the EWRAM
guard. The P7 campaign replay drives 9,020 ordinary input steps. It
compares the 958 advancing updates in the final 1,000 steps. All 11
world, fog, raw actor and displayed pixel cuts match. Route counters
and camera coordinates match throughout the measured interval.

The three-AI Blood Bath replay steps from the ordinary Custom Game
menu through update 17,301. It uses a Zerg player and Zerg, Terran
and Protoss CPUs. Both runs match the 20 sampled world, actor, resource
and fog states in the final 1,000 updates.

| ARM cycles | Release | Steering | Change |
| --- | ---: | ---: | ---: |
| P7 mean | 1,884,767 | 1,851,948 | −1.74% |
| P7 p95 | 2,185,045 | 2,156,612 | −1.30% |
| P7 peak | 2,553,373 | 2,522,633 | −1.20% |
| Blood Bath mean | 1,001,597 | 999,806 | −0.18% |
| Blood Bath p95 | 1,407,402 | 1,403,992 | −0.24% |
| Blood Bath peak | 2,193,638 | 2,195,675 | +0.09% |

P7 orders average 842,353 cycles, down 3.75%. It still exceeds the
561,792-cycle budget for 30 updates per second. The next performance
priority remains the number and cost of wide point-route fills and
uncached steering queries.

The sanitizer contact oracle passed 637,410 live mask and 4,138,459
step checks on the previous release. The previous release replay also
saved P5 midflight, confirmed a byte-identical 32 KiB SRAM image, loaded
it in a fresh process and won. Those checks are scoped to the prior
release lane.

## Receipt pins

| Replay | SHA-256 |
| --- | --- |
| Current P7 release | `ea454a6e6c4b7a93076673b4b5682ffcd9db351c734b1740cd68d68d3f4267f8` |
| Current P7 steering | `0605aec5db200a4370648c6bf18b7cf00a7bd6ff3cb9a07f6623006bbb24522c` |
| Current Blood Bath release | `7e18c69916088d3a9c07fe3abfef68606d9e95323a6090fe55105fc2c1431385` |
| Current Blood Bath steering | `6f10863a5de52979d93ad7f599aecbd84b2c33a14f73198eadb90fffa5f2692f` |
