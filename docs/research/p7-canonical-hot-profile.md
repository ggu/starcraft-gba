# Canonical P7 hot movement profile

The canonical P7 build has enough IWRAM for the contact steering pass and
inline stock movement step. Its original configuration inherited P6's ROM
placements. Remove those two placement flags only from
`PROTOSS_P7_CANONICAL_FLAGS` and enable the already qualified Arbiter aura
union shortcut. The base Protoss flags, explicit P6 product, other races and
historical P7 probes keep their previous configurations.

The movement changes affect code placement and inlining, not movement rules.
The aura shortcut skips further geometry tests once an allied Arbiter has
already covered a target during that source tick; its packed cache words
match the scalar oracle. It adds no saved state or persistent cache.

## Bounded ARM comparison

The baseline is the canonical P7 profile promoted in PR88, including facing,
weapon/death art, the complete P1–P7 bank and format-2 audio. Both builds replay
the same ordinary-input workload, including update and draw time in mGBA.
All 20 checkpoints and the terminal PLAY record match exactly.

| Window | Updates | Previous cycles | Current cycles | Reduction |
| --- | ---: | ---: | ---: | ---: |
| Opening | 600 | 2,155,200 | 2,113,065 | 1.96% |
| Attack | 1,400 | 2,426,615 | 2,233,602 | 7.95% |

The current means correspond to about 7.94 and 7.51 updates/s, respectively.
They remain far below 30 fps and are neither mission-wide averages nor
physical-GBA measurements. The bounded terminal record is not a mission win.

The steering placement alone measured 2,120,556 / 2,286,171 cycles; moving
both movement paths measured 2,111,661 / 2,276,738. Adding the aura shortcut
trades a small opening increase for the measured attack saving. A candidate
also placing the mapped blitter in IWRAM measured 2,113,065 / 2,234,003 and
spent another 888 IWRAM bytes without a useful gain; it is not selected.
Unlike the earlier rejected internal-probe experiment, this separate canonical
candidate fits the unchanged guard, but fit alone does not justify activation.

## Gameplay and save verification

The ASan/UBSan winning-tape host replay matches all 67 state records and the
strict verdict from PR88: call 39,976 / frame 39,738, all three heroes alive,
all 1,667 input transitions consumed, and no runtime or projectile fault.
The independent aura reference/candidate fixture compares packed cache words
over 80 adversarial/randomized worlds, including owner aliases, boundary bits,
stale spatial-index entries and the scalar fallback.

The exact current profile also passes fresh-process ARM SRAM continuation.
Normal pause-menu input saves at frame 28,000; a fresh process loads from the
title screen and reaches the strict source win with all three heroes and all
1,667 input transitions. All 1,321 prefix and 591 resumed host records match.
The original sequence-1 slot remains byte-identical, and the winning autosave
writes a valid sequence-2 slot. The 16,292-byte payload and generation remain
unchanged. Measured main-stack use is 5,036 / 4,128 bytes, leaving positive
margins of 6,740 / 7,648 bytes above the unchanged reserved boundary. The
validated result SHA-256 is `cb8aeb012671c4cf87d7eeab706ff114b7d8d3b3e3b2cd6bf095450d1ef92a3a`.
This covers one ordinary-input save point, not arbitrary saves or hardware.

## Memory and artifacts

The normal playable product is 32 MiB, with BSS end `0x0203de70` unchanged.
IWRAM ends at `0x03004b38`, leaving 1,224 bytes below the unchanged working
boundary. The SRAM codec and generation remain unchanged. The playable
SHA-256 is
`9d4485b1e848cc9b07cffa2dc67e7c8b395c69a936dbe14aeeede52a35baa8bd`.
The actual public Make-selected product has the same feature defines as the
measured candidate, with the same frozen authored runtime and private inputs.

Private plans, compiler arguments, logs and identities are under
`.local/build/p7-canonical-hot-aura-20261028/` and
`.local/build/p7-canonical-hot-sram-20261028/`; the baseline is
`.local/build/p7-canonical-bounded-20261028/`. No imported data, ROMs or saves
are distributed with this source change.

With prepared private inputs, build through `./dev sc64 build --profile protoss`
and run `make test-sc64-protoss-p7-canonical`. The standalone aura oracle can
run without audio or construction art:

```sh
SC64_AUDIO=0 SC64_CONSTRUCTION_ART=0 make test-sc64-p7-arbiter-aura-once
```

Canonical mission coverage stays 23/30. This change does not qualify arbitrary
save points, hardware behavior, native enemy-AI equivalence or 30-fps play.
