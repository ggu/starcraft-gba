# Z7 full-render ARM winning replay

An isolated instrumented ARM build completed the ordinary-input Z7 winning
tape in mGBA. All 365 checkpoints matched the host reference's dual actor and
game-state digests. The final source checks passed at host tick 209,485,
game frame 205,764, with RESULT/won, no runtime fault, a surviving human force,
zero source Force 1 buildings, and all eight action cursors and wait clocks
clear. The replay did not use GDB or writes to world state.

This is a development-profile completion result. It does not promote Z7,
establish native AI equivalence, or meet the 30-fps objective. A separate
frequency-profile ARM save continuation is recorded below. Canonical Zerg
remains Z1–Z6 and qualified staged progress remains 22/30.

## Build and comparison scope

Runtime source was based on `a40bb806a18844ff5ec1b29afd4a742456f6ae57`
(PR #62), retaining the full canonical presentation features, packed Zerg
bank, and the existing optional Z7 rule/performance features. The ARM
preprocessor vector was checked against public `Z7_STEERING_FLAGS` plus
`SC64_REACHABLE_FORMATION` and `SC64_Z7_PRUNE_STARTED`. This build does not
enable `SC64_ROUTE_DECAY_FREQ`.

The public ordinary-input policy is `tests/sc64_z7_campaign_run.c`. Its
20,325 input transitions and 365 state observations form the frozen tape.
An independently compiled indexed host reference and the full-render host
candidate matched all 365 digests under ASan/UBSan before ARM replay. Digest
fields include named actor/gameplay state; padding and derived presentation,
pathfinding, and audio caches are excluded. They are checkpoint digests,
not byte-for-byte snapshots of every emulated memory region.

The private harness supplies only held/pressed buttons through
`game_update`, followed by `audio_update` and `game_render`, including the
normal presentation call. Read-only observations validate the resulting
state. The terminal success predicate directly evaluates the source force
count and pending programs, rather than relying on the menu's displayed win
alone. Generated input headers, ROMs, ELF files, saves and logs stay ignored.

Frozen evidence identities:

| Item | SHA-256 |
| --- | --- |
| Winning input/observation trace | `252891e51ce82c44442acd098f58c63ad7616c79bd895c22c1bebc26e279ec97` |
| Fixed ARM ROM | `3d9982e2a7c64a34cc5dbd18a7ec5154de54c25e516478657291e8663a17ade4` |
| ARM ELF | `82b9e5f39254ab7aa4bb15dfdfa431778beacedd6dd8f599b5e53515ef580e86` |
| ARM harness source | `5cf28fc6401f20e3ddeca3b56c717a4c44634cd7138ae20e70ff3f3fa7a8f6e5` |
| Expected digest header | `e133bbfd9d44b4265db18627ca4d7069160d1f4d58927166e2b729830a1ef844` |
| Input tape header | `a55a2a3c2082ac1084cb5f9e288ae4eb8eb4e769c8f68e7e4ff09b632fc77b5b` |

The fixed ROM is 32 MiB, title `SC Z7 WIN36`, code `SZ7W`. The listed replay
artifacts and tape/digest hashes were checked again after the run completed;
source and generated inputs are pinned by their separate manifests. Its
source state and input tape must not be substituted with a later candidate
when interpreting this result.

## Performance result

The harness measured 205,764 simulation-advancing PLAY updates. Timed update
work excludes the subsequent checkpoint hashing/logging; the draw interval
includes audio, rendering and presentation. GBA timer deltas reported:

| Timed interval | Mean cycles | Maximum cycles |
| --- | ---: | ---: |
| Game update | 5,317,343 | 1,723,919,131 |
| Audio/render/present | 559,682 | 2,071,131 |
| Combined | 5,877,025 | 1,724,760,571 |

At 16,777,216 cycles/second the combined mean corresponds to about 2.85
rendered updates/second. This instrumented emulator result is far below the
30-fps target and is not a physical-GBA benchmark. The extreme update
outliers need investigation; a successful campaign replay is insufficient
performance qualification.

## Frequency-cache follow-up

A second fixed ARM build adds only `SC64_ROUTE_DECAY_FREQ` to the baseline
preprocessor vector. It also completed all 365 checkpoints and the terminal
source win. Its complete checkpoint records are identical to the baseline's.
The tested fixed ROM SHA-256 is
`dfb94f5dcad51f9bdbbb0bb3736d0f9cef80893e0059b431b735395e7c321115`
(title `SC Z7 FREQ36`, code `SZ7F`); ELF SHA-256 is
`b13a12d0c963e4bd6b7f6d24ffb24c16aab710899955df51d3e1ec77c301d0e7`.

| Timed interval | Mean cycles | Maximum cycles |
| --- | ---: | ---: |
| Game update | 5,057,839 | 1,723,228,551 |
| Audio/render/present | 559,624 | 2,070,970 |
| Combined | 5,617,463 | 1,724,139,353 |

The combined mean decreases 4.42%, to about 2.99 rendered updates/second.
This remains far below 30 fps. The larger reduction previously measured in
a bounded late continuation must not be described as the whole-mission gain.
This option remains experimental; no canonical race flag or save format is
changed by this report.

Private host experiments extending the policy's decay period from 128 to
512 or 1024 preserved all 365 digests but increased route builds from 33,357
to 51,645 or 60,283 respectively. Those variants are rejected, rather than
adopted on timing speculation.

## Actual ARM save and fresh-process winning continuation

The full frequency-profile replay also passed a separate two-process SRAM
experiment. The first ARM process replayed ordinary inputs from HOME,
matched 334 checkpoints, and reached host tick 173418/game frame 172247.
Normal START/release/B/release inputs paused and saved without advancing
the game frame. After terminating only that owned emulator process, the
runner validated the newest checksum-valid record in its 32 KiB SRAM file.
The generation-36 payload is 16296 bytes and matches the host UI-save golden
SHA-256 `e08d6175652680cf97653844f913d6c15a68b7f020f78f3c0a400a6fb6de2377`.

A fresh emulator process used the same saved file and normal HOME-L input.
It loaded frame 172247 with 6450 minerals, 4225 gas, the expected mission
state, no runtime fault, no selected building, and the expected initial
persistent-state digest. It then replayed the frozen suffix and matched all
31 remaining checkpoints through the source win at frame 205764. The final
state retained 50 live human actors, zero source Force 1 buildings, no runtime
fault and no pending action cursors or wait clocks.

Persistent digests normalize only the inactive command-menu cursor, which
the loader resets from 20 to 0; every other named field remains included.
This is not a claim of raw-memory or derived-cache equality. No SRAM was
manufactured by the host for this run: it was written by the prefix ARM
process through the normal save UI, checked, then reused unchanged for load.

| Frozen item | SHA-256 |
| --- | --- |
| Fixed prefix ROM | `7baaa6a97cad9904f72c302b2bb5c1c98ebf55362b9f0d1842c340928a5aeaed` |
| Fixed resume ROM | `843fc7d91febb3ca55ebaf9ebbdfa6605d27c87c30257690a63764f9dbe71d59` |
| Prefix-generated SRAM file | `89272e18ad7319b5dcecbe37bdb963f03e557fe581c6e63e1758219accac54ed` |

This proves the bounded emulator save/load continuation for the tested
frequency profile, not flash-cart SRAM or physical GBA operation. Performance
and the full carried-mission qualification still prevent campaign promotion.

## Carried missions and attack-move controller repair

Host regression runs using the full frequency-candidate flags completed
Z1, Z2, Z3 and Z4 through normal menu/gameplay input. The initial Z5 and Z6
controllers did not win. Removing only the frequency-cache flag, or only
reachable formation, produced byte-identical failing traces; neither option
explains those failures. Removing direct attack allowed both controllers to
win, which motivated inspecting their command policies rather than disabling
the game feature.

Z5 intended ground Attack-Move orders but could click a visible hostile and
receive an explicit AttackUnit order. Its controller now chooses a nearby
reachable ground point, verifies the actual resulting orders, and tracks
Kerrigan's exact accepted approach destination for the detector-trap watchdog.
If it cannot find a safe point, it reports failure instead of knowingly
issuing the wrong command. A failed trap retry advances ordinary neutral
input so it cannot bypass the controller's tick limit. Source victory and
defeat checks are unchanged.

With private Z7 inputs prepared, reproduce this carried chapter check with:

```sh
make test-sc64-z7-carried-z5
```

The published compile recipe's preprocessor flags were checked against the
full winning frequency candidate, and its ASan/UBSan controller wins Z5 at
frame 49,786 with no runtime fault and Kerrigan alive. Its input trace SHA-256
is `9fa4dda90910221f86721aca4fbd6f28e087fa6e15c118c7293ba184be53036f`.
The same source also wins with direct attack disabled and with the canonical
six-mission Zerg flags. Both retain the prepatch gate-off trace byte-for-byte:
`4fd6cce64853c8f98e5069d6798c092ff7a303cc5da2fe5fd29c9a0fd736b205`.
Only the test controller changes; production direct-attack rules remain intact.

Z6 now also has a host win under the full development combination. Its
controller preserves intended Attack-Move with ground clicks, checks
reachability from actual selected members rather than a possibly impassable
group centroid, and cancels armed UI state when it cannot find a safe click.
Kerrigan's retreat guard rechecks moving threats while traveling. Storm
support targets currently visible hostiles and uses sequential casts after
the first effect expires, with ordinary energy costs and controller input.
The engine and source victory predicates are unchanged.

With the same prepared private inputs, run:

```sh
make test-sc64-z7-carried-z6
```

This runs the focused controller-policy fixture in a separate process, then
the input-only campaign controller. Synthetic actor edits appear only in the
focused fixture, never in the controller that generates the winning tape.
Under the complete Z7 frequency/formation/packed-cache feature vector the
ASan/UBSan controller wins at frame 134,932 / host tick 136,446, with Kerrigan
alive at the island objective, Ventral Sacs researched, no cargo remaining,
zero Force2 actors in both required locations, and no runtime fault. Its
trace SHA-256 is
`56e038eacaf7d0b6a6dd4d09cc8a6eb7085b24825489b4b03dbc66a7225aae4f`.

The same controller also wins with DirectAttack disabled and with the
canonical six-mission Zerg flags. Those two traces are byte-identical:
frame 115,413 / host tick 116,527, SHA-256
`baa85cc4de68d771b5118d042dac944a52f2d213a776d93a74a8792e290205c5`.
The controller records each input immediately after its own tick, before a
retreat can recursively issue more inputs. The earlier `16ea712…` full trace
and `64a796…` canonical trace captured host wins but contained delayed input
timestamps; they are superseded and must not be used for replay. All 4,725
transitions in the corrected full trace and 3,657 in the canonical trace pass
strict ordering, held-state and pressed-edge validation. The Make target now
runs that validation automatically. An offline check is also available:

```sh
python3 tools/sc64_zp_controller_replay.py --profile zerg-z6 --trace PATH --validate-only
```

Offline validation checks recorded contracts without launching an emulator.
A separate fixed-input host replay with update/audio/render passed all 198
actor/Z6-state checkpoints and the same source victory. The corresponding
ARM replay then matched all 198 checkpoints and won at tick 136,446 / frame
134,932, with no runtime fault and the same objective state. Its ROM SHA-256
is `ad0d35ef2bbabf9e058ae6a5996ea6e6d21355da87719ab3a46ccd9889ab0e0a`.
The linked image used 32,229,996 bytes, ended BSS at `0x0203d7ac` and IWRAM
at `0x03004d10`, and retained a measured 7,560-byte stack margin. These
checks apply to the complete experimental Z7 feature vector, not a newly
promoted canonical build. Fresh-process emulator SRAM continuation for this
corrected tape is verified below. Performance and physical-cartridge behavior
remain unqualified. Canonical progress remains 22/30; performance and remaining
target qualification still prevent promotion.

### Corrected Z6 tape: fresh-process ARM SRAM continuation

A separate ARM prefix replays the corrected tape through tick 130,000 and
matches all 188 original checkpoints up to that cut. At live frame 128,515,
normal START/release/B/release input saves the game. The owned emulator is
then stopped and its actual flushed 32 KiB SRAM is checked: tag `0x53435a54`,
16,296-byte payload, sequence 1 and valid FNV checksum. The payload SHA-256
matches the independent host save:
`d200dcbf65f06bd4adf9731a5503b046217be9ced1aa2d47d9747c2669b6a1b4`.
No host-created SRAM payload is injected into the emulator.

A new emulator process loads that file through ordinary HOME-L input. Its
immediate persistent-state comparison matches, and the unchanged suffix
matches all 10 remaining projected checkpoints and the source victory at
tick 136,446 / frame 134,932. Kerrigan remains at (950,1009), Ventral Sacs is
researched, cargo and both objective-location counts are zero, and no fault
is reported.

Load intentionally resets a closed menu selection, initially rebuilds the
derived Z6 elapsed field, and discards five nonserialized event outputs:
`presentation_opcode`, `last_string_id`, `center_x`, `center_y`, and
`center_requested`. The comparison projects only those listed differences;
tail checkpoints retain actual elapsed time, camera/world/persistent state,
open-menu selection and the unchanged `last_wav_id`. This is not equality of
all transient in-memory bytes.

The runner initially rejected a changed final SRAM file after every load,
checkpoint and victory check had passed. Source review confirmed the normal
victory autosave: the original slot is byte-identical, and the alternate slot
contains a valid sequence-2 record. The post-run review corrects that invalid
file-equality assumption without changing SRAM or rerunning the evidence.

| Frozen ARM artifact | SHA-256 |
| --- | --- |
| Prefix ROM | `8985a8f811d417a122b7e7dc7fa163af1eac50f154e58de1fdc2bdd3a034abe9` |
| Resume ROM | `bc2c5140a9873e313c8179cedfc51e14f9d1e028f4c0fee8df58021d916cd5b3` |

The prefix/resume linked load images are 32,229,956 / 32,241,080 bytes; final
ROMs fit 32 MiB. BSS ends at `0x0203d7ac` / `0x0203d7a4`, IWRAM at
`0x03004d10`, and measured stack margins are 7,088 / 7,144 bytes. Private
logs, raw saves, exact argv and `run/reviewed-result.json` are retained under
`.local/build/z6-sram-continuation-20261027/arm-root-reviewed-v3/`.
This establishes the tested emulator continuation only, not physical GBA
SRAM or near-30-fps performance.

All eighteen rotated skirmish configurations (two maps, three human races,
one to three AI opponents) reached terminal losses in this host matrix. A
separate print-only observer found no Z7 runtime faults and confirmed its
mission runtime was inactive in skirmish. These outcomes establish bounded
completion for those configurations, not AI balance, every possible AI-race
assignment, physical hardware behavior, or 30-fps performance.
