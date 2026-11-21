# Custom Egg and Carrier journal composition

At source `f9a54b4571db42eadc2391e6903c61df684fa84f`, the shared Custom Game Egg service remains opt-in. The earlier Egg preview and its ordinary-input evidence do not establish composition with the later normal Carrier journals or the P10 research journal. Release-enablement draft #298 selects the late-campaign Egg context for Z9/Z10 and the custom context for Terran, Z7, P10 and Showcase; its recorded six-product results predate this Carrier composition. The draft remains open, with its Z8 ordinary controller and early Z1–Z6 integration among the stated gaps.

## Current payload bounds

These bounds include the components named in each existing envelope, not an Egg/Carrier composition test. The fixed SRAM payload limit is 16,368 bytes.

| Envelope | Existing maximum | Remaining bytes | Scope |
| --- | ---: | ---: | --- |
| Terran Carrier custom | 15,929 | 439 | 100-slot compact projectile packet, 32 impacts, two Yamatos and transport trailer. |
| Z10 Carrier custom | 16,285 | 83 | 100-slot compact projectile packet, 32 impacts and transport trailer. |
| Showcase Carrier preview | 16,368 | 0 | 95 projectile slots, 32 impacts, two Yamatos and transport trailer. |

The maxima are established by the assertions in `sc64_terran_carrier_envelope.h` and `sc64_zerg_carrier_envelope.h`, and the Showcase maximum and tests documented in `CARRIER-PROJECTILES.md`. A new trailer cannot fit the Showcase maximum without changing that envelope or a stated capacity. This does not imply an Egg needs extra bytes: its existing candidate uses actor rows and authenticated tags.

## Composition blockers

The Carrier gates in `game.c` test `SC64_SHARED_EGG_CUSTOM_CANDIDATE` before the later `SC64_SHARED_EGG_CUSTOM_SERVICE` block defines it. Therefore that test alone does not fence the real combined configuration. On the pinned final ARM recipes, preprocessing f9 source with the Egg service flag succeeded for Terran and Z10, but a subsequent `-fsyntax-only` failed for both: the projectile candidate callback lacks the new authentication argument and another caller references `guard_auth` without a declaration. P10 preprocessing rejected its research/Egg combination; the separately staged P10 compound research/Egg probe is not the normal product.

There is an independent save-format blocker. The Egg custom writer uses `SC64_SHARED_EGG_SAVE_VERSION` as the inner version, while the current Terran and Z10 Carrier packers require `SAVE_VERSION`. Their custom branches run before the ordinary Egg writer. Even after the compile errors are fixed, the packers reject that candidate unless a deliberately authenticated combined format is designed. The Carrier outer tag, Egg candidate-local authority, predecessor selection, and fallback must agree before any candidate is installed. The existing test-only P10 research/Egg context is a reference, not proof for Carrier composition.

The next bounded change should establish a separate combined candidate authentication and thread it through projectile validation, pack/unpack and selector preflight, retaining old-tag fallback and rejecting mismatched inner/outer tags. Test a live Egg alongside a nonempty Carrier packet, full impacts and transport state, including corrupt-newer fallback and old journal reads. Run per-product full-capacity checks, then normal-control fresh HOME continuations and ARM memory/timing checks before activation. Keep the showcase bound explicit.

## Probe receipts

The isolated probe used the three existing `arm-final` recipe arrays with `-DSC64_SHARED_EGG_CUSTOM_SERVICE`, `-E` or `-fsyntax-only`, and f9 source. It generated no ROM or game assets. Terran and Z10 preprocessing returned 0; P10 preprocessing returned 1. Terran and Z10 syntax checks returned 1. SHA-256 of the diagnostic logs: Terran syntax `bff4aa0fb97f7c44cc2f29f214c846e393bea5ddbd02880422d70dd7bad4d5f6`, Z10 syntax `4ff8013d3bf1de98a4db2a6ef0facb58c1e49d43b1f7a680fb062c27024e5806`, and P10 preprocessing `5cf7705cbe2a3af7cbf9db89de3e8a0f293417513bdfab22820e0696fa02f1e3`.
