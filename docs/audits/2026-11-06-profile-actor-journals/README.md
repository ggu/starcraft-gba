# HOST-only Z7 and P10 shared actor journal probes

This source checkpoint qualifies a narrow, explicitly staged HOST candidate. It
does not enable the service in guest or normal product flags. Runtime source
`8acb2344a6252b31c54cbec73ca411559344a759` merges the stable shared-actor
checkpoint `5ea13b73`, the typed/derived codec work, and only the Scarab launch
adapter fragment from `833297f7`. The fragment's forward launch offset is a
PC-reference approximation; it is not an observed N64 launch measurement.

`SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE` requires HOST, the complete shared
flight candidate, Z/P full storage and Battlecruiser/Yamato support. Static
profile checks limit this experiment to Z7 derived generation 38 or P10 typed
generation 47; in particular it does not enable Z9 flight without BC support.
The existing authenticated feature-15 outer/inner tag pair and the transport
bit are retained. The reader binds the unchanged inner version to the selected
candidate; writer and reader apply Guardian, waypoint, flight and real profile
codec conversions in opposite order. Existing previous-tag selectors and the
normal/default product flags are unchanged.

## Selected-journal evidence

At `8acb2344`, both test targets below passed under ASan/UBSan with the shared
native lock and more than 3 GiB free. They use separate, genuinely older game
source to write each profile's own feature-1 Egg predecessor; the predecessor
is not a guest save. The HOST fixture constructs its world, including resources
and an unobstructed launch site, and is not an ordinary-input playthrough.

```sh
make -f Makefile -f tests/sc64-profile-actor-journal/Makefile \
  PROFILE_OUT=/tmp/sc64-profile-8acb2344 profile-actor-z7
make -f Makefile -f tests/sc64-profile-actor-journal/Makefile \
  PROFILE_OUT=/tmp/sc64-profile-8acb2344 profile-actor-p10
```

Both selected the new feature-15 journal, loaded paid Egg, Cocoon and Scarab
state together, checked a real autonomous Scarab route across save/load, and
used a retagged newer slot to verify fallback to the actual old predecessor.
The fixture applied 750 direct Guardian morph component ticks and checked the
born Guardian after another save/load. It does not prove full-world morph timing
or a mission win. The P10 test also executed four real `game_update(0,0)` calls
with the Arbiter cache and Scarab motion both valid. Exact log:
`/tmp/sc64-profile-8acb-journals.log`. All 1,959 paths in
`/tmp/sc64-current-c96-evidence/inputs.json` matched before and after the two
profile batches. P10 used the pre-existing private feature-15 P10 fact table;
no pinned generated header was regenerated.

The initial inherited Terran fixture did not compile as Z7 because that
profile lacks its distinct Guardian and Battlecruiser enum names. A later P10
fixture at a fixed map coordinate did not acquire a route. The corrected P10
fixture initially bypassed launch initialization, then reached a real route
but its save was rejected. These failures and diagnostics are retained in
`/tmp/sc64-profile-actor-z7-d5a0.log`,
`/tmp/sc64-profile-actor-p10-a573.log`,
`/tmp/sc64-profile-actor-p10-aed9.log`,
`/tmp/sc64-profile-actor-p10-f9f0.log`,
`/tmp/sc64-profile-actor-p10-3e28.log`,
`/tmp/sc64-profile-actor-p10-9036.log`, and
`/tmp/sc64-profile-actor-p10-3cfd.log`.

## P10 field collision and bounded representation

The P10 Arbiter detection cache uses the entire `Unit.weapon` field and its
world validator requires the cache marker. The staged Scarab motion code had
also used that field for turning and previous-waypoint-known bits. A direct
validation exemption would not suffice: ordinary Arbiter cache updates also
write the field and could overwrite Scarab motion.

For this exact HOST P10 profile only, `weapon` remains the Arbiter cache. The
two Scarab motion bits use bits 14–15 of `path_timer`; its low 12 bits retain
the prior source waypoint. The typed feature-15 waypoint bridge serializes
those motion bits, both independent current and previous waypoint coordinates,
and the real 0–10 shield value. It checks map bounds, identity, logical flags,
and unused wire digits. The typed row has 14 flag bits; the candidate uses a
radix of 8,192, leaving a capacity of
`2048² × 25 × 13 × 512 × 8192 = 5,717,460,464,435,200` values for at most
`2^46 × 11 × 4 = 3,096,224,743,817,216` combinations. A static assertion
checks that bound. Z7 and other codecs retain their prior flag radix and field
interpretation. P10's typed actor codec separately retains Scarab heading and
velocity using the staged live raw-85 HP representation; dead rows keep signed
HP and reject unrepresentable cooldowns.

The component fixture checked all four motion-bit values, coordinate extrema,
shield 0–10, invalid digits and feature mismatch. The typed codec fixture
checked all 256 velocity bytes across HP 1–20 and old raw-36 Overlord byte
identity. These are component checks, not proof that every real-world state is
reachable or that every profile's cache/actor interaction is complete.

At `8acb2344`, source boundary, the conservative 20,224-value historical tag
registry check, the 100,000-case derived codec test and the 12,000-pair typed
codec test also passed; log `/tmp/sc64-profile-8acb-regressions.log`.

## Remaining gates

Full-button Z7 and P10 gameplay and longer save/resume are not established by
these fixtures. Qualification still needs actual new-state/full-world cadence,
old profile-specific 0/5/7 predecessor and malformed-newer fallback coverage
where those formats were actually supported,
legacy raw-36 Overlords in selected journals, source-created actor and flag
preflight, ownership/capture/target/death/reuse and pending-impact/transport
interactions. ARM builds, timing, RAM and stack, genuine fresh-process guest
SRAM and applicable campaign/custom regression remain open. The normal and
guest enablement fences must stay in place until the combined service passes
those gates; Z8 ordinary-win and other campaign completion gates are unchanged.
