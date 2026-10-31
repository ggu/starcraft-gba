# T5 worker-order controller qualification

On worker STOP/manual mining source base 83875b4, the T5 host controller uses native selection and orders to keep workers out of escort movements, and issues contextual mineral commands to up to four nearby visible fields. It verifies the selected actor and resulting mining order; empty workers may be reassigned from gas without interrupting carried cargo. The normal campaign objective, hero survival checks and 200,000-call limit are unchanged.

A host ASan/UBSan run with the existing full Terran compile flags won at host tick 124803 / game frame 123481: primary and completed objectives set, Kerrigan and Raynor alive, Command Center alive, zero enemy buildings and zero faults. The log records accepted native mining commands on multiple fields. The prior attempt that selected only the nearest patch reached the limit with nine enemy buildings remaining; the worker-inclusive escort stranded loaded workers on MOVE orders. Logs and input trace are retained locally in `.local/t1-controller/t5-fields-*` and contain no asset data intended for publication.

This is a single host-controller qualification, not ARM replay, save/resume, hardware or frame-rate evidence. Other campaign and skirmish qualifications remain necessary for the worker-order change.
