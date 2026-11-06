# Z8 ordinary-input defense observation

This HOST-only investigation uses source `dfee66b75a6f88becddcac05d8d90d9428864d7a` and the Z9 product controller vector. It does not change the controller or game. The enabled vector includes `SC64_SHARED_EGG_SERVICE`; the off vector uses the same Z9 release flags without that selector. Both use the ordinary `game_update()` controller, Clang `-O1`, ASan and UBSan. No resources, units, outcomes or guest state were injected. These are not ARM wins or N64 comparisons.

The two program headers were recovered from the retained descriptors with their original packers; their hashes match the existing Z9 receipt (`ab3fad9e…` for Z8 and `fab0840f…` for Z9). The complete sprite bank was regenerated in an isolated checkout using the documented `docs/DEVELOPMENT.md` recipe and matched the receipt's `fafb2a0ac415ef3f12249539debcc95204560c15a565fbd11d72c2da701c1c9e`. The remaining copied headers and codec2 audio followed the renderer qualification input manifest. Each run took the shared native lock with more than 4 GiB free, and its 272 copied header/blob hashes matched before and after. This is a finite input set for these runs, not a claim that every historical input matched.

| Controller | Egg service | Outcome | Calls | Fault |
| --- | --- | --- | ---: | ---: |
| Baseline | on | loss at frame 11,048 | 11,333 | 0 |
| Baseline | off | loss at frame 21,782 | 22,279 | 0 |
| Combatant count and raid-position group target | on | loss at frame 11,048 | 11,321 | 0 |
| Combatant count and raid-position group target | off | loss at frame 13,412 | 13,766 | 0 |
| Combatant count only | on | loss at frame 11,048 | 11,321 | 0 |
| Raid-position group target only | on | loss at frame 11,048 | 11,309 | 0 |

The count-only and combined enabled traces were byte-identical. These trials reject the combined change as a controller fix; no strategy or engine change from them is retained. The two service trajectories differ. At the first observed per-call difference, call 1,528/frame 1,481, the enabled run contained an extra friendly Egg with both resource banks equal. That expected production representation difference is not by itself a defect.

A sparse observer reproduced the enabled baseline input trace byte for byte. The first raid spawned at frame 1,845 and was dead by frame 2,566. The second appeared at frame 10,555 and was visible by frame 10,852; at frame 11,032 it had 29 of 40 HP, and the mission was lost 16 frames later. The nearby Overlord remained alive. At frame 10,852 the two Hydralisks were about 201–205 pixels from the raid, beyond their 64-pixel runtime range (128 source pixels); six Zerglings had 15-pixel melee range. The existence of eight defenders near the guard therefore did not ensure early fire.

At frame 10,789 the baseline interceptor counted an Overlord and a Zergling, and its selection attempt returned without acting after 62 game frames. At frame 10,852 it counted 11 nearby friendly non-Drones, including two Overlords and a map revealer, and selected the more-than-ten branch that ordered attack-move to the guard. Subsequent command sequences occupied 26–57 frames. The combatant-count trial instead counted one Zergling at frame 10,787; its direct-target sequence occupied 84 frames, followed by further sequences of 81 and 94 frames. The raid still had 40 HP at frame 10,954. Its observer matched that candidate's input trace byte for byte. These observations implicate the controller's command timing and selection, but do not establish every attack acceptance, cooldown or targeting condition.

Private logs, observer source and hashes, the failed patch, exact compile commands and input manifests are retained in `.local/qualification/z8-pair` of the isolated qualification checkout. The baseline enabled input trace SHA-256 is `51251bd1acadfe1e2ded1a2bad6858b9cd3699df9a83fa4ad8718749ad3ab1e6`; the combined enabled and count-only trace is `4891b48e93a2d316782f0e086782e69b25ee0fc4334dabfc6f58da5ac846f52f`. Observer and baseline trace identity checks establish unchanged recorded input/state checkpoints, not exhaustive world identity. A future controller experiment should verify order acceptance and defensive placement with ordinary inputs and qualify a full win, save continuation and ARM replay; a delayed loss does not close the gate.

## Further bounded defense trials

Two additional single-change ordinary-input HOST controllers were tested on the
same `ab9b35c` source checkout and the same enabled Z9 product vector. Each was
compiled with the existing ASan/UBSan flags, ran under the shared native lock,
and preserved all 272 recorded private input hashes. Engine rules, resources,
enemy stats, mission triggers and outcomes were not changed.

| Trial | Only controller change | Outcome | Calls | Fault |
| --- | --- | --- | ---: | ---: |
| Attack post | Pre-raid Hydralisk repost uses attack-move rather than move | loss at frame 11,048 | 11,309 | 0 |
| Earlier interception post | Northeast guard moves from (1950,330) to (1840,430) | loss at frame 11,048 | 11,290 | 0 |

Neither change is retained in the controller. The changed post also altered
earlier gameplay (final Drone count 11 versus 14); it cannot isolate only the
second raid's engagement. The current engine already gives idle non-worker
actors a within-range acquisition path in `SC64_IMPORTED_MISSION`; the older
controller comment saying idle units ignore raids is not a general engine fact.
Thus this evidence does not justify adding idle autoattack or claiming it is
missing. Next observation should capture actual accepted orders, weapon reach,
cooldowns and target/detection eligibility during the second raid before
another strategy or engine change. A full ordinary-input Z8 win remains open.

Exact failed sources, compile logs, full traces, input manifests and machine
readable final states remain local in `.local/qualification/z8-attack-post`,
`z8-intercept-post` and `z8-controller-followup-results.json`. Their trace
SHA-256 values are `99b5afbbb3252b7697966989309b21dec925c362b99a3513788b0357e3a5c18a`
and `301d50bd68dda798153d99f560444bcca020d98d62651ec4d3f358bed94ef01e`.
These are HOST losses, not ARM wins or N64 gameplay observations.

A subsequent read-only observer sampled the original enabled controller every
eight advancing frames during the second raid. Its full trace was byte-identical
to the retained baseline (`51251bd1…`), and all 272 private inputs were unchanged.
It establishes that the two local Hydralisks have runtime range 64 (not 128),
10 base damage and active cooldowns while firing: the raid initially retains
40 HP because it also has 80 shields. Its shields fall to zero before HP falls
to 33. Both Hydralisks hold an accepted direct target of raid slot 25 at frame
10,952, but later controller orders replace that target with attack-move to
the old guard point by frame 11,008. They return to idle at frame 11,016 while
the raider continues toward the defeat rectangle. This supports testing retention
of an accepted direct order, rather than assuming the weapon does not fire.
Local observer sources and logs are retained in `z8-order-observer`; no gameplay
state was written by the observer. The range statement above is corrected to
distinguish imported source pixels from the halved runtime coordinate scale.
