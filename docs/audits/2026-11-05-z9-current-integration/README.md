# Current late-Zerg qualification candidate

Integration `062ddfe5` combines default branch `f9dc1179` (including prepared
save selection, late-objective source art and target-list memory recovery) with
the historical late-Zerg controller branch `3a2765bf`. The Makefile merge keeps
both the current source-art coverage fixture and the late-mission replay rules.
This branch does not change canonical campaign selection or promote Z8–Z10.

The historical Z9 win/save evidence is described in the adjacent
[cargo qualification audit](../2026-11-05-z9-cargo-qualification/README.md).
It does not qualify this integration. New HOST, ARM and genuine save tests
must use the current inputs and runtime before making a new playability claim.
The 1,958 prepared inputs copied from the qualified `a9a0ea76` workspace were
individually checked against its SHA-256 receipt; no importer was rerun.

The merged candidate retains the historical authored town-wave approximation,
including its provisional 5/4 wait conversion and averaged group sizes. Static
script reports are source evidence, not observed N64 attack execution. The
[late-Zerg integration audit](../2026-11-05-late-zerg-integration/README.md)
records those limitations. Ordered source-script execution and Guardian work
remain separate candidates; this integration does not establish combat parity.
The Z10 controller is preserved as an unqualified draft, not a demonstrated win.

Initial gates are source-boundary and script-report tests, Z9 actual game
fixtures, an ordinary-input HOST controller win and a normal Z9 ROM guard.
If they pass, generate a fresh input tape and run full/prefix/fresh-process
continuation on HOST and ARM. Compare complete journals as well as the existing
scalar/digest checkpoints, whose historical digest omits some cargo fields.
Carried-mission validation and unchanged canonical product qualification are
required before any default promotion. Timing remains a separate gate; a
successful mission does not imply the unfinished 30 fps target is met.

All native work uses the shared local flock and bounded storage. ROMs, generated
data, saves and logs remain ignored/private. No Actions or SD writes are part
of this qualification.
