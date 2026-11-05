# Current-source performance and fresh N64 production observations

## Current P7 performance baseline

On merged `72a71d223506a4e889ceaa1e4a61b33bf6f623de`, a local emulator
replayed the existing ordinary held-key P7 tape, measuring calls 8020–9019.
The selected compile arguments matched the current Make P10 production vector
in order, with the shared Egg experiment off. The observer uses a replacement
main loop. These are guest-cycle measurements, not physical GBA fps or an
unmodified-main timing certificate.

The reference contains 958 advancing frames and three modal acknowledgments.
Its average full loop is **2,200,226 cycles**, with a **3,918,363-cycle** peak
at call 8347. The corresponding instrumented run averages 2,273,295 cycles:
73,069 extra cycles, or 3.32% observer overhead. All 90 sampled named-state
and pixel checkpoints, configuration fields, active counts and modal counts
match between runs. This does not prove equality of every intermediate state.

| Instrumented scope | Mean guest cycles per advancing loop |
| --- | ---: |
| Update | 1,670,171 |
| Orders | 1,184,664 |
| Order setup | 582,673 |
| Arbiter setup, inside order setup | 457,829 |
| Movement | 367,982 |
| Rendering | 602,805 |
| Map, inside rendering | 369,861 |
| HUD, inside rendering | 125,193 |

These scopes are intrusive and nested. Do not add their totals or subtract a
whole scope as a promised saving. Movement peaks at 2,663,616 cycles at the
same call 8347; order setup peaks at 758,870 and its Arbiter subset at 634,189
on call 8443. The roughly 559,000-cycle 30 fps period still requires a major
reduction in this busy window. No optimization or speed gain is claimed here.

The next performance milestone is exact current-source order/detection and
movement attribution, followed by a bounded optimization that changes neither
answers nor timing. Keep the ordinary campaign/custom regression matrix and
stack/RAM gates; do not apply a uniform actor-update skip based on N64 timers.

All 1,958 prepared inputs, tape, compile-argument and text-box hashes were
checked before/after. The shared native-job lock and disk reserve were held;
no guest world data was injected. Private observer harness, build logs,
checkpoint results and receipts are retained locally. The ROM hashes of the
reference and instrumented builds are respectively
`e07c356404586eb6578e755908639624829135daff79ab9e6b0b5a1460c63f94` and
`27f06dc1afa0be17254007eb3acedcc579daf3bb82fcd3353db1c6ee6349170c`.
The private observer is not a new public turnkey build command.

## Fresh N64 Zerg production observation

A fresh private ares session used ordinary controller input to select
Single-Player → Scenario → Setup Custom → Melee → Blood Bath, Human Zerg
against one Computer Zerg, with both remaining slots Closed. Empty private
saves, copied settings and a separate input shim were used. No global keyboard
input, RAM writes, forged state, shared saves or installed-app changes were
used. The process exited at its bounded 175-second limit and released the lock.
No full-RAM snapshot was taken.

The operator selected the Hatchery, selected three larvae, opened Build and
highlighted Drone. The menu explicitly displayed **50 minerals and 1 supply**.
After A, the visible supply changed from **4/9 to 7/9** and a **Cancel** prompt
appeared. The maintainer independently inspected the menu and later capture.
This supports a displayed product cost and reservation during production;
it does not establish an exact three-product debit or prove successful births.

Ongoing mining and transitioning HUD digits prevent a reliable debit from the
captured wallet values. Neither cancellation nor birth was observed before the
cap. Screenshot timestamps and shim input polls are not game frames, VI fields
or a validated source clock. No production-duration, cancellation-refund or
native animation-timing claim follows.

A prior attempt reached custom setup but failed map navigation and produced no
Egg evidence. The corrected run used a source-census map-order lead, then
confirmed Blood Bath on screen; map order was not assumed to prove gameplay.
Private screenshots and input/settings/provenance logs are retained locally,
not committed. Normalized owned ROM SHA-256:
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`;
ares SHA-256:
`4ce08c105b4f930d9a86bf03c8ea51c5867fa3e29c942997a868b2b158a4d51d`;
input shim SHA-256:
`ba9227ef9a14deec020b62affc7d92eb015d51d44d90eb8edacf1cb784f7ff70`.

See the [maintainer priority decision](../2026-11-06-n64-priorities/README.md)
and [living backlog](../../PROJECT-TODO.md). Coverage remains 29/30 and the
full campaign/skirmish/near-30-fps goal is not achieved.
