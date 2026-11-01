# T8 worker-order controller qualification

On worker STOP/manual-mining runtime 83875b4, the former T8 controller reached its existing 200,000-call limit without a win. It relocates workers and depots but relied on automatic idle mining. The controller now selects empty idle SCVs through normal input and issues contextual A to visible mineral fields near a completed grounded owned Command Center, checking the resulting order and retaining cargo.

The full Terran host controller compiled with prepared product flags and ASan/UBSan won at tick 182,492 / frame 176,566 with zero faults and its existing victory, hero and completion checks satisfied. The limit and objectives were unchanged. Local log and trace: `.local/t1-controller/t8-manual-run.log` and `t8-manual-trace.json`. This is a host-controller result, not ARM replay, save/resume, hardware or frame-rate qualification.
