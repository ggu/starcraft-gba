# P7 cached movement, sight and geometry probe

This opt-in profile reduces repeated work in the experimental Homeland
runtime. It combines the contact cache from PR80 with build-time optical
radii and source-pixel actor rectangle extents. Canonical Protoss remains
P1–P6; total canonical progression remains 22/30.

## Reproduce

```sh
# Asset-free feature, fallback, gate and independent geometry checks:
make test-sc64-fact-v2

# Requires the already prepared private full P7 inputs:
make test-sc64-p7-fact-v2-normal-input
make test-sc64-p7-fact-v2-campaign-controller
make protoss-p7-fact-v2-probe
```

The playable output is
`build/StarCraft-Protoss-P7-Fact-V2-Probe-GBA.gba`. It uses the normal
experimental seven-entry menu, projectiles, paid Carrier training and
Interceptor flight. Its header is `SC P7 FACTV2` / `S7V2`. This is an
internal probe, not a promoted canonical campaign or a distributable ROM.

The generator writes a separate
`build/generated/p7-fact-v2/sc64_fact_tables_generated.h`. It compiles with
the target's feature and actor gates, without `SC64_FACT_TABLES`, and derives
the table values from the reference helpers. Legacy V1 actor/collision
headers remain accepted when neither new feature is requested. V2 records
its sight/rectangle feature bits; missing features, unsupported versions and
different actor gates fail compilation. Sight requests outside the roster
retain the original source-provider/fallback policy. Rectangle lookup keeps
the original raw-type, coordinate, pointer and half-open-boundary contract.

The full profile replaces the older `SC64_COLLISION_CLASS_CACHE` and
`SC64_COLLISION_STEER_LIST` flags with `SC64_CONTACT_CACHE`. It does not enable
both implementations together. Neither canonical flags nor the earlier V1
probe change.

## Current-base comparison

The comparison uses main `5b451f687dc47366bf354769de34ee1f568b1ebd` plus the
candidate source, including the format-2 audio bank staged in a separate
private directory. The older shared PCM bank was left unchanged. All three
builds use the full P7 sight-axis/projectile/Carrier/Interceptor profile and
the same ordinary-input tape. The three variants isolate the new table
payloads from the movement-cache change:

| Variant | Opening combined cycles | Attack combined cycles | Opening reduction | Attack reduction |
| --- | ---: | ---: | ---: | ---: |
| V1 actor/collision facts, old movement caches | 2,191,702 | 2,640,498 | baseline | baseline |
| V2 sight/rectangle facts, old movement caches | 2,180,465 | 2,613,607 | 0.51% | 1.02% |
| V2 facts and contact cache, public probe flags | 2,061,085 | 2,325,286 | 5.96% | 11.94% |

These are mGBA measurements over 600 opening and 1,400 attack frames,
including update and draw time. The final row is about 8.14 and 7.22 rendered
updates per second equivalent, respectively. It remains well below 30 fps.
They are not full-mission averages or physical-GBA measurements. The attack
window does not qualify native enemy Carrier AI.

All 20 state checkpoints and the terminal record match exactly across the
three ARM builds and their host goldens. The public Make-selected ARM replay
has a byte-identical unpadded load image to the measured contact-cache build.

| Variant | Unpadded replay bytes | BSS end | IWRAM end |
| --- | ---: | --- | --- |
| V1 facts | 12,859,512 | `0x0203d6dc` | `0x03004918` |
| V2 facts | 12,860,108 | `0x0203d6dc` | `0x03004918` |
| V2 + contact cache | 12,862,196 | `0x0203d568` | `0x03004a70` |

Each replay is padded to 16 MiB. The V2 tables add 596 linked bytes in the
isolated comparison without additional BSS. The contact cache replaces
larger per-slot caches, reducing BSS in the final profile. All original RAM
and ROM guards pass. Measured main-stack high-water use in each bounded ARM
replay is 2,196 bytes, leaving 9,580 bytes above `0x03005000`; this is scoped
to these executions, not arbitrary game states or the separate IRQ stack.

The playable ROM, built with the normal `common/main.c` rather than the
benchmark harness, links to 12,848,648 bytes and pads to 16 MiB. BSS ends at
`0x0203dd68` and IWRAM at `0x03004a70`. Its SHA-256 is
`2f2ea1c065785f89e984d23ae3fe874c3870e4aea4f34cc112e8f414370d7ab9`.

## Full-win regression

A separate current-base ASan/UBSan fixed-input replay applies all 1,667
transitions from the previously qualified winning tape to each variant.
All 67 state records (including the terminal record) and the verdict match the
[frozen full-controller qualification](p7-full-controller-qualification.md).
Each wins at call 39,976 / frame 39,738 with all three heroes alive, the
source victory gate true, source outcome WON, and no runtime or projectile
fault. Both 32-bit and 64-bit state digests match. The fixture changes no
world state and uses the same held/pressed edge rule.

The public V2 campaign controller separately requires a strict win and
passes the offline trace validator. It is a host controller run, distinct
from ARM replay of a fixed input tape. The public bounded replay is a
performance regression fixture, not the full winning tape.

## Full ARM replay of the current cache profile

A subsequent fixed-input ARM replay qualifies the complete winning tape
with the V2/contact-cache profile from PR83, including format-2 audio. The
235 frozen authored `common/` and `starcraft/` files match that source tree;
only the replay harness adds sparse timer reads and diagnostic output. It
does not change the controller tape or game state. Host control and sampled
builds first matched all 67 frozen state records and the strict verdict.

The sampled ARM run then matched the same 67 state records, including both
32-bit and 64-bit digests, and the full terminal verdict. It wins at call
39,976 / frame 39,738, consumes all 1,667 input transitions and retains all
three heroes. The strict source-victory predicate passes with no runtime or
projectile fault. This is ARM replay of recorded ordinary inputs, rather
than execution of the adaptive host controller on ARM.

The diagnostic image links to 12,904,408 bytes and pads to 16 MiB. BSS ends at
`0x0203d620`, IWRAM at `0x03004a70`, and measured main-stack high-water use is
5,036 bytes, leaving 6,740 bytes above the reserved boundary. Its header is
`SC P7 SPARSE` / `S7SP`; it is a replay image, separate from the playable
probe above. The owned emulator process was stopped after its validated win
and stack record.

| Artifact | SHA-256 |
| --- | --- |
| Full ARM diagnostic ROM | `4584a9a0c0883fc5c2a8bcdb8152a696751774c9f28c369b07e6525c04841259` |
| ARM state/profile log | `43716a3c1217fe27fb1f6a845739c2b606b84799922c9dea6093465a1bb1f0b3` |
| Validated result | `aa9be6903ce4dfa8d7c15540732f34ea6973146af37b6923028ed7296dca134f` |
| Compiler argument record | `8717541879e7f2c8fbcd742298269cd65cde7db33bcf12267f38a5fde3cc0aef` |

There are 22 sparse in-play timing samples across the winning tape, each
with update, audio, render and existing subsystem counters. Transition to
RESULT is excluded from those samples. Unit orders account for 61.1–70.3%
of update cycles in these samples. That points to order processing for the
next investigation; it does not attribute that cost between its internal
movement, detection and other work. Sparse samples are not a full-mission
average, a comparison with an uninstrumented control, or a 30-fps result.

These private artifacts, source hashes, host parity checks and the launch
record are under `.local/build/p7-current-full-sparse-20261027/`.

## Current-profile fresh-process SRAM continuation

A separate two-process ARM run now covers one pause-menu save/load path on
the exact FACT V2/contact-cache flags, including the current format-2 audio
bank. It freezes the current 235 authored source files, uses a private copy
of the generated fact header, and replays the same 1,667 input transitions.
Both postlinked 16 MiB images have valid headers and pass the original RAM
guards. Each emulator process has an isolated configuration and uses the
same private cartridge/save path; the second process starts fresh.

The prefix saves at tick 28,166 / frame 28,000 after 1,174 input transitions.
Its 16,292-byte generation-43 payload fits the 16,368-byte limit, validates
its checksum and matches the host payload. The fresh process loads through
the title screen, records the expected selected-building reset from 4 to -1,
and reaches the strict win at call 39,976 / frame 39,738 with all three
heroes. All 493 resumed UI events and 31 each of gameplay, UI and source-output
projection records match the reference. The immediate load source-output
elapsed field resets from 28,000 to 0; later output projections match.

The prefix and resume respectively validate 1,321 and 591 host-corresponding
records, excluding their stack records. The original slot at offset 0,
sequence 1, remains byte-identical; a second checksum-valid payload appears
at offset 16,384, sequence 2. Payload SHA-256 values are the same as the
earlier frozen-profile proof:
`b401174ce8531e6cb48b016afd49f1274a173d1a4d4652db0950e0a9205e5d43`
and `42221acf76b607845857dbc4fcf344c27ff98f696d8209ebca7e87091d3d4114`.

| Run | Unpadded bytes | BSS end | IWRAM end | Main stack used / margin |
| --- | ---: | --- | --- | ---: |
| Prefix | 12,936,728 | `0x0203d618` | `0x03004a70` | 5,036 / 6,740 bytes |
| Resume | 12,896,896 | `0x0203d608` | `0x03004a70` | 4,128 / 7,648 bytes |

Prefix ROM SHA-256 is
`a12ca015375c94c56519683c9bf58e62c3de6e9c73855fbf2c41a1fc20cb966d`;
resume is `c08b10f96c13835bcf655b53b4943110671eb4fc33b5c54a93cb42e7e305c12e`.
The validated result SHA-256 is
`50d3cae9487586790df3e07c4c935006555722adafa3bcf00eca292b7ec0b3b7`.
Private source manifests, host results, actual compiler commands, launch/exit
records, logs and SRAM images are under
`.local/build/p7-fact-v2-sram-qualification-20261027/`.

This qualifies one fixed-tape continuation on the core FACT V2 profile. It
does not establish arbitrary save-state compatibility, the additional flags
of the drawing probe, a performance target or physical-GBA behavior.

Private evidence is under `.local/build/p7-mission-20261027/`: the three
compiler plans, generated tables, host logs, ROM identities, ARM process
identities, state records and timing results. No ROMs, generated banks,
screenshots, saves or proprietary data are included in this change.

## Remaining qualification

This does not implement enemy Interceptor purchases. The existing owner-4
training fixture seeds an order manually; the mission AI currently issues
attack-move orders and does not establish the native fighter-request policy.
That native policy requires separate characterization and implementation.
The new profile still needs full-mission ARM performance work, canonical
P1–P7 integration and earlier-profile save compatibility, broader gameplay
coverage and physical-GBA testing before promotion. The current bounded
SRAM result above closes the exact-profile continuation gap for its tested
cut; it does not automatically qualify other drawing or gameplay flags.
