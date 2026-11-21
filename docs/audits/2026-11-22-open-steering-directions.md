# Shared ground steering cost

The ground mover evaluates its local steering directions after an
actor blocks the intended step. The contact query already identifies
which of the eight offsets cannot move. Visit the remaining directions
in the same order and evaluate distance and terrain only for them.
An earlier terrain check has already prepared the path grid for this
movement call. Equal distances keep the first eligible direction.
The existing contact memo, waypoint state and blocked retry remain in
the same control flow.

The shared path is active in the normal Terran, Zerg, Protoss and
Showcase builds. All four pass the ROM and RAM guards. Static EWRAM
endpoints match the control. IWRAM ends eight bytes earlier in each
race ROM and 24 bytes earlier in Showcase.

| Paired controller replay | Mean guest cycles per call | p95 guest cycles |
| --- | ---: | ---: |
| 9,489-call P10 resume through victory | 1,550,286 → 1,535,936 | 2,134,533 → 2,119,436 |
| 3,600 advancing Z10 Blood Bath updates | 820,235 → 817,911 | 1,109,367 → 1,108,015 |

P10 resumes at frame 36,761 and wins at 46,057. All 68 world,
indexed display and fog cuts and victory SRAM match. Frame and route
counters match on every call. The mean falls 0.93% and p95 falls 0.71%.
The Blood Bath pair uses the combined Z5 detector build as control.
Its mean falls 0.28%. All 31 world and video cuts, 16 fresh HOME cuts,
per-call route counters and SRAM match. These traces measure emulated
guest cycles. Dense campaign pacing is still above the 30 fps budget.

All 54 ordinary Custom Game launches and fresh HOME continuations
pass across the three race ROMs. The updated Z10 ROM also consumes
the first 24,000 calls of the corrected Z7 physical tape and matches
all 61 named observations in that prefix.

| Local receipt | SHA-256 |
| --- | --- |
| Current P10 victory control | `aeb93c63d55a0bac711100683d7da239dfa34b4b2a24bc0f327698575fea461e` |
| P10 steering pair candidate | `c3e926c0d70257c7dc3f539585d6bcab9e0fc7e896fe6b9d94a34e1052f1214f` |
| Combined Blood Bath replay | `7bbe1bda3c207236474a24b09fde9e77974ca3e181bf73350dd53c7fd6889f66` |
| Custom Game fresh HOME matrix | `31fa35ccba191e0b299c2dc4b5e3ae8094e97ecd2367339eda288f87b44e5caa` |
| Combined Z7 named prefix | `d509b47a6381c0f148d79eeba2198b66f82c0d68dbce9478316e12b7c56e79fe` |
