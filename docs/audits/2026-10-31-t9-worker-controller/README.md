# T9 worker-order controller qualification

On worker STOP/manual-mining runtime 83875b4, the former T9 controller sent its starting SCVs by MOVE and relied on automatic idle mining. It reached the existing 500,000-call limit with zero minerals, no Wraiths and two SCVs. The controller now issues native contextual A orders to visible home fields for empty starting and idle SCVs, verifies each selected worker and resulting mineral order, and preserves carried cargo.

The full Terran host controller compiled with the normal prepared flags and ASan/UBSan won at host tick 48,169 / frame 47,416, with the Protoss objective buildings cleared, Kerrigan alive and zero faults. The normal goal and call limit were unchanged. The local log and trace are `.local/t1-controller/t9-manual-run.log` and `t9-manual-trace.json`. This is host-controller evidence, not ARM replay, save/resume, hardware or performance qualification.
