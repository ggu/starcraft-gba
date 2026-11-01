# Z8 paid production controller observation

The retained `893879b` host run logged one Hydralisk production request:
the Hatchery queue changed from empty to one at frame 9915 as the wallet
changed from 79/300 to 4/275; it returned to empty at 10439. The coarser
attempt log's 379-to-4 mineral delta spans a separate 300-mineral Hatchery
foundation payment at frame 9667. The old fixed-slot observer did not census
all births, so the queue's later empty state alone does not prove which actor
emerged. Only original northern Hydralisks 11 and 13 were sampled. The Den
died at frame 10405 and original Hatchery at 11240; no later wallet change was
logged after frame 10439. One request cannot explain a five-attempt cap being
reached in this run.

The Z8 path does not use the Z7 Egg job: its predicate requires an active Z7
mission. Its ordinary paid Hatchery queue is the observed production path.
The earlier helper increments macro counters on an attempt even if it did not
admit a product. It also presses A to select the Hatchery even when it is
already selected; on that building A queues the current displayed product.
Two initial Drone calls yielded queue counts 1 and 3, consistent with an
extra queue request. This is a controller accounting/input issue, not evidence
of a game-side production defect.

This source-only variant retains the first-raid selection gate. Its ordinary
input helper avoids reselecting an already selected Hatchery, checks that one
new requested queue entry appeared on the A-press update, and increments the
later Drone/Hydralisk counters only on that observation. Rejected or ambiguous
attempts use a bounded retry delay. The first two setup requests use the same
checked helper. A read-only census records all human-source raw-38 actor
appearances, disappearances and periodic position/health/order samples along
with producer and wallet state. Same-slot, same-identity reuse between samples
cannot be distinguished. No defense, source state, resources, unit stats or
win conditions are changed. This variant has not been compiled or run.

In the retained run, original Overlord 48 was moved to (1328,1848), took
repeated damage from frame 12606 and died at 12802. Nearby Scouts and Dragoons
were recorded; the log does not identify a unique attacker. A later hostile
Dark Templar reused slot 48 and reached T17's source location 18, whose GAME
rectangle is x1184–1440 and y1952–2048, leading to defeat. A different detector
post or escort is a separate, unverified policy problem; no position here is
claimed safe from moving enemies. The host controller uses world observations
and does not qualify an unaided human or physical replay.

## One bounded host trial

The single `06161dfa2a1b0df6182813dc407dde7f3b0e6315` trial reached strict
LOSS at frame 13450 (native 10760), with T17 location-18 count 1, fault zero,
and the original Kerrigan alive at 328 HP. Its first Dark Templar was visible
at frame 2119 and cleared at 2188. This is not a campaign WIN.

The two initial Drone requests were each observed as one admitted queue entry
(q0→1 at 4590, q1→2 at 4593), without a third accidental entry. Two later Drone
requests were confirmed. Three Hydralisk requests were observed as admitted at
7108, 8278 and 9191; source-owned raw-38 appearances followed at 7632 in
slot 5, 8802 in slot 103, and 9715 in slot 55. Slot 103 accepted a Nydus order
at 8880 and was gone at 9942; slots 5 and 55 were gone at 10769 and 10909.
The original north Hydralisks 11 and 13 remained in the logged staging box,
but no six-unit readiness/push event occurred. The log records one new
reinforcement transit, not the hypothesized three. The Den and original
Hatchery were destroyed at frames 10325 and 10968; the last wallet observed
was 44 minerals and 225 gas. No forward-worker trip or foundation was logged.
The trip has several simultaneous prerequisites, and this log does not
continuously record all of them, so its absence is not assigned one cause.

Original detector 48 was removed at frame 12952; a later T17 defeat followed.
No detector or defensive policy was changed by this source variant, and no
later-route fix is claimed. Source identity, queue entries and world census
are read-only host observations; same-slot, same-identity reuse remains a
limitation. The 109 pins and five sidecars were verified unchanged. Retained
log SHA-256:
`2ffed4f3d12ee77870d75cce85f0320d4f19ddd2851f7abab53f35c038cd8ba8`.
