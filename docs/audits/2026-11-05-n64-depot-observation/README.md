# N64 Supply Depot observation

A clean, private ares session on the owned USA ROM used ordinary controller
input in Single-Player, Scenario, Setup Custom, Melee, Challenger (96×128),
Human Terran versus one Zerg computer. The game-speed slider was explicitly
set to its MIN endpoint. No save-state loading, guest memory writes or unlock modifications were used. Inputs and screenshots remain private
under `.local/n64-depot-probe` in the isolated investigation worktree.

The Z+R Master Build Box selected Build Basic → Supply Depot; an SCV was
selected by the game, the valid green placement was accepted, and 100 minerals
were spent. Later screenshots show the SCV working at a visible foundation
while supply remained 4/10, and then the completed Depot with supply 4/18.
This confirms one ordinary custom-game construction and the observed supply
increase in this configuration. The GBA Depot rule also charges 100 minerals
and grants 16 internal doubled-supply units, equivalent to the observed eight.
The control sequence follows the [StarCraft 64 manual](https://nintendo64.pl/wp-content/uploads/StarCraft-64-USA.pdf).

The total construction duration was **not measured**. Construction began between
sparse captures, and emulator frame-advance pulses were not independently
calibrated as one displayed video frame apiece. The last in-progress capture was
followed by a paused interval and then a ten-second unpaused interval before
the completed capture; this only bounds the remaining interval and cannot
establish the total time. Wall-clock time and the input-poll count are not
substitutes for game frames. No native/GBA numerical timing parity is inferred.

For comparison only, the current race rules specify raw Depot build_frames
600, converted by `building_build_time()` to 750 GBA update ticks using
`(frames*5+3)/4`. The original runtime rate and the effect of the native game
speed setting remain to be measured. ROM SHA-256 was
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`, the
previously verified isolated ares binary SHA-256 was
`4ce08c105b4f930d9a86bf03c8ea51c5867fa3e29c942997a868b2b158a4d51d`, and
input shim SHA-256 was
`ba9227ef9a14deec020b62affc7d92eb015d51d44d90eb8edacf1cb784f7ff70`.
The owned emulator process was terminated and the shared native lock released.
