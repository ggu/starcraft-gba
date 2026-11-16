# Native SCV work after construction: witness plan

The retained native observations do not establish whether a human SCV
automatically resumes mining after completing a building. In an ordinary
11/05 Melee/Challenger probe (Terran human, Zerg CPU), the final retained
screenshot at 12:19:56 shows the completed Supply Depot (supply changes
from 4/10 to 4/18), the selected worker still alongside it, and worker
commands visible. Other SCVs are visibly working the mineral line. The
image SHA-256 is
`17d11196afe94d1a8741940cbfac49e5494a44e2c28f2712b8317311d02f72d4`.
This snapshot does not establish how long the worker remains there or
its next order; aggregate mineral income cannot distinguish this worker
from the other three.

The separate [clean Depot VI run](2026-11-05-n64-depot-vi/README.md)
recorded 1,655 fields after visible completion, with the whole-stream
receipt and hashes retained. The historical raw and lossless XZ paths
under `/tmp/sc64-ares-capture-review/depot-max-clean` are absent in this
workspace. If recovered, stream and hash the XZ without recreating the
large raw file; review port-1 transitions and track the builder visually
before scheduling another native run. VI fields are not simulation ticks.

## Bounded controller-only headless run

1. Start a fresh ordinary Terran versus Zerg Challenger custom game;
   reproduce the existing opening with the same seed input timing,
   and establish a visible, accessible mineral patch and build site.
   Wait for 100 minerals. Select a single SCV observed carrying minerals
   to the Command Center and returning to a known patch. Finish the
   current delivery before interrupting it so an old load is not
   mistaken for new harvesting.
2. Keep that worker selected. Use C-Left, the ordinary Build grid, to
   choose Supply Depot, and confirm a valid site with A. Place the Depot
   far enough from the mineral line to make the return leg distinguishable
   but within the same camera view. Ensure the worker actually constructs
   it. Release the controller and issue no world or selection input until
   after the post-completion window. Other workers need not be stopped
   if the builder can be tracked independently.
3. Capture the last working interval, the first completed interval and
   at least one *measured ordinary gather-and-return interval* afterward.
   Prove a positive result by tracking the same worker back to a mineral
   patch, harvesting a new load and delivering it. Prove an observed idle
   interval by showing the same worker stays idle through that window,
   then explicitly ordering that worker to mine the reachable patch and
   observing a successful harvest and delivery. Aim at a mineral patch
   with no friendly unit covering the cursor hit area; a rejected manual
   order is a controller targeting issue, not evidence about automatic
   resumption. Repeat with an initially idle builder to distinguish
   restoration of the prior mining order from fresh automatic assignment.

The existing isolated actor frontend can send A (mask 1), Start (2),
Down (4), C-Left (8), D-pad Right (16), R (32), C-Right (64), C-Up
(128), C-Down (256), B (512), and signed analog axes. Use neutral
release rows between pulses. The earlier graphical input log used
pause and speed bindings, so it is a guide to the control path rather
than a deterministic headless tape. The new menu, selection and build
placement coordinates still require visual calibration. Do not
interpret the old fixed capture tape or its validation receipt as a
qualification of the new route.

Use the read-only RAM role only after validating a pinned executable
and its provenance. Capture mode 2 in the RAM role records a
cache-aware RDRAM view; the separately pinned visual-only role permits
only 0 or 1. Budget no more than three RAM snapshots and three visual
frames per run, and replay the same prefix with complementary capture
points if the control needs another frame. The custom-game owner
chain at `0x80114660 + 4 * owner`, next link at actor + `0x6c`,
owner at + `0x4c` and raw type at + `0x64` provide a starting
identity check. Require a unique raw SCV 7 record whose coordinates
track the selected builder across snapshots; do not use the
previous selection-correlated global as its stable identity. Treat
uncalibrated order and target fields as hypotheses; establish their
meaning through the explicit gather control. If the chosen role cannot
fit the neutral wait and control within its approved step, wall and
output budgets, shorten the opening or defer; do not quietly extend
the guard. No run was started for this plan.

GBA human construction explicitly sets the completing SCV to
`ORDER_IDLE` with target -1 in `race_construction_worker_tick`. The
documented GBA skirmish CPU behavior comes from that owner's later
planner and does not answer the human native question. A controller
cursor prioritizing a nearby friendly unit over a mineral patch is
also independent of completion behavior. No product change is
supported until the worker-specific native evidence is available.
