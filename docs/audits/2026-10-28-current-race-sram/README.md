# Current race builds: ARM continuation and P7 replay

This audit qualifies specific emulator runs after worker-arrival construction
was merged in PR113. It does not qualify the full 30-mission goal, arbitrary
saves, every skirmish configuration, or physical hardware.

The tested source is `852f1ea352df2edd1b0aa6933b21e6c5f4b7b7d5`, whose tree
equals default-branch merge `bdaace017e8fa1765afdcb2713a76538e4116e8a`.
PR114 (`013122760bd91abcbf47fd5e31540ad345780926`) subsequently changes
only two test controllers. The `common/` and `starcraft/` runtime is unchanged.
The diagnostic ROMs replace the normal `main` with a bounded input harness;
they are not byte-identical to the playable products.

## Fresh-process skirmish saves

Each canonical race vector was linked against the existing private prepared
art, briefing and format-2 advisor/audio inputs. The Terran, Z6 and P7 builds
retain their own campaign definitions and save identities. No imported
generation recipes ran, and no private input is distributed here.

Each of the three builds completed this protocol in two owned mGBA processes:

1. Start with an empty, profile-specific save directory. Use normal menu
   inputs to choose Blood Bath, the profile's human race, and three bots.
   Their races are `(human + 1 + slot) % 3`, covering all three bot races.
2. Advance 1,800 neutral play updates. Use START, then B to save through
   the pause menu. Record the saved-world semantic fingerprint.
3. Resume with A and advance 900 play updates to record the uninterrupted
   reference. Terminate the owned emulator and preserve its physical SRAM.
4. Launch the exact same ROM at the exact same path in a fresh process,
   using the preserved save directory. The harness detects a physical SRAM
   header, then presses L from HOME. The game's normal loader independently
   validates the newest record and payload checksum.
5. Compare the restored fingerprint with the saved world, then compare 900
   neutral updates with the uninterrupted reference. Require the continuation
   verdict, rather than accepting another first-phase run.

All three restored worlds and continued worlds match the respective host and
ARM reference records. These are semantic comparisons: frame, RNG, economy,
supply, research, map/race/opponent configuration, geysers, exploration,
owner banks, mineral fields, and live unit/building fields and queues feed
both a 32-bit and a 64-bit fingerprint. Transient UI, audio and render caches
are outside that fingerprint. The hashes are not a byte-for-byte world proof.

| Build | Saved frame | Continued frame | SRAM tag | Payload bytes | Sequence |
| --- | ---: | ---: | --- | ---: | ---: |
| Terran | 1,801 | 2,701 | `53435442` | 15,816 | 1 |
| Z6 | 1,801 | 2,701 | `53435a50` | 16,296 | 1 |
| P7 | 1,801 | 2,701 | `5343505b` | 16,292 | 1 |

Every save is 32,768 bytes, with one valid sequence-1 record and an empty
second slot. Its payload checksum validates independently. The save file
remains byte-identical through the second process, which performs no new save.

| Build | Diagnostic ROM SHA-256 | SRAM SHA-256 |
| --- | --- | --- |
| Terran | `eb3bf0e91034722fee7538744f069a8ef5ac81359b2965d573b313c693c3f620` | `98a8d741a12f626ed7ccf79af45baca605afb377ad8d8ced3190224c2069ddce` |
| Z6 | `afd688668c3663fbb032abaac934e30df5b700c54f0164ac1412b2c59d20797d` | `c20a2c6d1fc2be212e5021b241b6ca091f9681ba8b932f4671427918a2e00f10` |
| P7 | `8172dcf7a0837be95b9646328142cbca88a5f9a3b70a19d300777cc5caad06f1` | `74f9260e8ced69769b87839e5f4182f2ff6550e3adffb17d9603bcf7877df489` |

The normal ROM verifier accepts all three 32-MiB diagnostic images, including
their header checksum, `SCGF` identity and SRAM signature. The unchanged
linker guards require IWRAM end below `0x03005000` and BSS end below
`0x0203e000`; all three links pass. This is a link-bound check, not a measured
stack-watermark result for the skirmish harness.

## Bounded timing

Timer 2/3 cycle counters bracket update, audio and render, including
presentation waits. Update and pre-presentation drawing are measured
separately. Hashing and diagnostic output occur outside the timer, and the
first two advancing calls after a checkpoint are excluded to avoid their
presentation-phase disturbance. The timer is independent of host wall time.

| Build | Initial calls | Mean cycles | FPS equivalent | Resumed calls | Mean cycles | FPS equivalent |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Terran | 2,701 | 579,516 | 28.95 | 899 | 584,796 | 28.69 |
| Z6 | 2,701 | 579,414 | 28.95 | 899 | 595,421 | 28.18 |
| P7 | 2,701 | 581,997 | 28.83 | 899 | 598,253 | 28.04 |

FPS equivalent is `16,777,216 / mean cycles`. The first-phase counters
include the launching call, and the resumed counters include the load call;
those calls become PLAY while doing relatively expensive setup. The
uninterrupted reference includes 900 advancing updates. The measured resumed
window includes the non-advancing load call, excludes the next two calls,
then counts the remaining 898 updates. Hence its metric count is 899.
The raw maxima, buckets and component sums remain in the private result.
These measurements cover an early three-AI scene at the starting camera;
they do not establish crowded-battle, campaign-wide or physical-GBA 30 fps.

An additional no-input run keeps the same starting camera while the three
bots develop and attack. It stops at the normal result screen or 30,000
calls, whichever comes first. All three ARM runs match their host semantic
records through the result. These unattended runs are not player-controller
wins. Each window resets its cycle sums and excludes the next two calls.

| Build | Result frame | Windows | Window FPS-equivalent range | Highest mean draw cycles | Highest mean unit/effect stage cycles |
| --- | ---: | ---: | ---: | ---: | ---: |
| Terran | 10,865 | 4 | 23.57–29.15 | 457,775 | 240,027 |
| Z6 | 14,140 | 5 | 23.39–29.15 | 445,847 | 208,461 |
| P7 | 12,888 | 5 | 21.77–29.02 | 491,796 | 247,888 |

The stage includes living units, death art, projectiles and spell effects.
It does not isolate any one of them. This exposes a rendering bottleneck as
combat reaches the camera, even with relatively modest total actor counts.
The early 28–29-fps measurements therefore cannot be generalized to later
combat. Long-run private evidence is under
`.local/build/main-0131227-skirmish-long/`; its three diagnostic ROM hashes
are `37041b1d0abd66f43cd395cbd48d9afd637cb3805f46f579ee17c3e5ae1bd1be`
(Terran), `d623b5c4ad305e11fedc4d14610b06690ac4037499c0651128aa929d9e908bc8`
(Z6), and `5ba8fed9ed0592d4d9e025170f69e75adfb6ec16a6beebd2ffae762717c5c98c`
(P7).

## Current P7 campaign replay

A separate diagnostic replays the current ordinary-input P7 host route with
1,492 held-input transitions. All 65 state checkpoints match the host oracle;
65 separately checked music records retain a valid stream and no fault.
The strict source victory occurs at call 48,660, frame 36,488, with all three
heroes alive. The terminal record is at call 48,667. The measured main-stack
watermark uses 5,200 bytes and leaves 6,576 bytes above the unchanged reserve.

This diagnostic ROM's SHA-256 is
`b5ebe761491d1f025331a06914a278b9029e5a22cdff023168d6b0107e3b9f95`.
Its valid end-of-mission SRAM record has a 16,292-byte payload and sequence 1.
This run is a complete input-only ARM mission win, not a fresh-process P7
campaign continuation test or a campaign frame-rate measurement. Earlier
P7 continuation results remain scoped to the images named in their audits.

## Reproduction and remaining work

With existing private inputs, use the canonical profile compiler arguments
obtained from a dry run of its Make target. Link a diagnostic `main` in place
of `common/main.c`, retaining the exact profile defines, prepared include
paths, supporting modules, audio bank and linker guards. No real imported
Make recipe or asset generation is required for these checks.

The save harness follows `tests/sc64_skirmish_resume.c`'s menu and world
comparison protocol, but uses physical SRAM and two emulator processes.
Use separate config and save directories per profile, capture mGBA's debug
log on stdout without a debugger, and require both phase verdicts and the
normal loader's success. Do not replace the second phase with host emulated
SRAM, `game_init()` in the same process, or injected state.

Private reproduction inputs, compiler vectors, harness sources, host
goldens, owned-process launch/terminal records, cycle counters and logs are
retained under `.local/build/main-bdaace0-skirmish-fresh/` and
`.local/build/main-bdaace0-p7-arm-qualification/`. Those artifacts contain or
link proprietary inputs and are deliberately excluded from the source repo.

The canonical catalog remains 23/30: Terran 1–10, Zerg 1–6 and Protoss 1–7.
T9's current ordinary-input controller does not yet qualify a win; Z7 has no
current-controls strict win, and later Zerg/Protoss chapters remain work in
progress. The 72-case host skirmish matrix covers both maps, all human races,
one to three mixed bots and focused three-bot compositions. This new ARM
test adds fresh-process proof for three of those configurations, rather than
claiming equivalent ARM coverage for all 72.
