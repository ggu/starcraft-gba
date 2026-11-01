# Z8 opening selection experiment

The retained boxed-offense (`b4f4383`) and forward-reservation (`a567311`)
trials used the same staged spawn hook and normal-input controller ancestry;
the latter adds a selection fast path and later macro behavior. The Overlord
order occurred at frame 444 and the original Kerrigan's Nydus request/exit at
frames 490/521 in both. The boxed trial issued her attack-move at frame 826
to (900,1544), observed the first Dark Templar at frame 2119 and its clearance
at 2188, with Kerrigan alive at 328 HP. Its later outcome was LOSS at 13412.

The reservation trial issued that attack-move at frame 839 to (900,1542).
It observed the first Dark Templar at frame 2119; at 2160 it retained 30
shields, and by 2280 had left detection. Source trigger 17 produced LOSS at
2600 while the original Kerrigan was alive at 328 HP. It did not reach the
new macro stages. Exact retained log SHA-256 values are respectively
`98bb4d485b2c1c58e05229be83a355fced15d8a5bf050ba9558ec45f576ccfed` and
`2f655eb17aa9297f0dc1784176e2a04ff67069084b95d9e96db6bb3ccb49319d`.

The forward-reservation fast path returns without cycling when the actor is
already selected. Ordinary L-cycle selection also moves the cursor to the
selected actor, so skipping it can change subsequent cursor movement and
rounding. The retained traces do not establish that this change caused the
missed kill: other controller observations and later policy were also added.

This unrun variant gates the shortcut until the existing observed
`FIRST_CLEARED` condition, then retains it for later macro work. It logs the
gate and use, keeps the same source world and ordinary button operations,
original-hero identity checks and 20,000-step bound. The controller reads the
world as an oracle; this is not an unaided human or physical-input trial.
Clearance, survival, macro success and campaign WIN are not yet established.
