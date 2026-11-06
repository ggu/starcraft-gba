# Combined research and fractional combat staging — 2026-11-07

This is disabled research work, not a new playable release. Runtime
`8127f7af1abe1ad9b5a911f7ea3554bcc691fba7` descends from draft #336 and
merged main `fb2e414f`. The normal product flag sets are unchanged.

## Separate feature modes

- The HOST-only campaign active-job journal uses namespace `0x47`, with
  detached candidate/controller/source-policy validation. Existing `0x43`,
  research-only `0x44`, and custom-job `0x45` readers retain their own paths.
  Actual P1 Forge and Core jobs and inactive T1/Z1 save paths have constructed
  fresh-process fixtures. Historical producers must be an untouched checkout
  of exact `847dbaaebf6454b0cb45577ce1e7ddd91d81e7d2`; the Make targets
  refuse a substitute or a dirty baseline.
- `SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT` is a separate HOST-only mode. It
  requires research jobs and explicitly refuses the active-job journal mode.
  Saving is fenced off because fractional state has no qualified game codec.
  The passing journal and combat suites are tests of separate configurations
  in the combined source, not a saved fractional-combat game.

## Fractional combat and lifecycle coverage

The pure GPL helper follows pinned OpenBW damage ordering, including the
half-point minimum, damage class/size factors, shield armor and the one-full-
shield-point eligibility threshold. Common live eligibility remains ahead of
it, and the existing core keeps whole-HP debits, deaths and mission callbacks.
Whole HP is a ceiling with a debt byte per actor; eligible Terran buildings
instead keep their existing burn deficit as the single authoritative value.
Shield fractions share the 1/1280 regeneration phase while preserving its
sub-Q8 time carry; non-shield trap/energy/regen aliases are not shields.

Actual-engine HOST fixtures exercise ordinary damage, Plasma Shield armor,
all 1,280 shield phases, half-point HP accumulation, invincibility, building
shields, deaths/slot reuse/new-match clearing, Siege splash, P6 explosive
hits, Archon area events, Interceptor hits and natural dock/relaunch repair,
and an optional Scarab impact. The Scarab target generates separate facts for
its 54-unit roster; it does not overwrite the normal 53-unit generated table
or disable its roster guard.

Lifecycle fixtures cover burn damage/credit, full and fractional-only repairs,
construction clamps, townhall morph max-HP deltas, and Z5 fractional regen.
Dedicated Creep-to-Sunken/Spore fixtures run the actual pre/tick/post service
for 375 updates. Completion clamps authoritative fractional HP before changing
identity: 100.5 HP becomes the existing one-HP minimum, 101.5 becomes 1.5,
and Spore retains the prior fraction. The fixture uses explicit authored source
ownership and is not ordinary-input construction, cancellation or persistence
proof. Fractional damage newly inflicted mid-morph is not a separate fixture.

P4/P6/Z6 Storm scenes cover overlapping pulses, fractional shield spill,
shield amounts below one point, lethal callbacks, P6 trap cooldown aliasing,
and Z6 hallucinations. Hallucination doubles a single Ignore Armor packet
before the shield threshold, rather than applying two separately resolved
hits. Existing nonexperimental behavior is retained. The existing 14-damage
pulse and 20/10-update scheduling are project/pinned-PC assumptions, not an
observed N64 timing certification.

## Evidence and failures retained

The earlier combined `64c38761` default/kernel/journal run passed, as did its
normal research-off P10 link; its headed ROM SHA256 was the existing release
`317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9`.
A subsequent plain `make unified` stopped at missing private interface
preparation metadata. It is not a passing unified test or a reason to fetch
unrelated proprietary inputs.

A lifecycle fixture initially cast a 16-bit building field to `unsigned *`.
UBSan reported a misaligned four-byte store although the process exited zero.
That old result is not clean sanitizer evidence. Commit `8127f7af` replaces it
with correctly sized local storage. The final runner stops on the first
sanitizer finding and also scans logs for recovered diagnostics. Prior
missing-header, roster-mismatch and fixture setup failures remain in local
receipts; no guard was weakened to turn them green.

At exact runtime `8127f7af`, all 17 final commands passed in two serialized
receipts (the first run stopped at the described harness-only conversion error;
the corrected remainder repeated the normal link):

- `scarab-facts`: pass (1.26 seconds).
- `q8-scarab`: pass (18.296 seconds).
- `q8-normal`: pass (19.408 seconds).
- `journal-p10`: pass (57.549 seconds).
- `journal-z9`: pass (32.554 seconds).
- `journal-terran`: pass (32.574 seconds).
- `lifecycle-p10`: pass (18.235 seconds).
- `lifecycle-z9`: pass (16.027 seconds).
- `lifecycle-terran`: pass (15.719 seconds).
- `storm-p10`: pass (17.777 seconds).
- `storm-z9`: pass (15.786 seconds).
- `normal-p10`: pass (21.065 seconds).
- `custom-jobs`: pass (18.307 seconds).
- `clean-boundary`: pass (22.737 seconds).
- `clean-doctor`: pass (0.393 seconds).
- `clean-build`: pass (2.847 seconds).
- `clean-test`: pass (447.81 seconds).

The clean full default suite took 447.810 seconds. Boundary, doctor and build
passed in a separate asset-free checkout at the same head. All 593 prepared
hashes, the isolated Scarab header, all three frozen checkout heads and source
contents remained unchanged. The normal P10 headed ROM again matched exact
release SHA256 `317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9`.
Local receipts are `.local/q8-storm/final-combined/results.json` and
`final-remainder/results.json`; strict sanitizer environment and log scans apply
to both. No full optional unified/Terran test or feature-on ARM claim follows.

## Remaining release gates

There is no qualified fractional-combat save format or normal/ARM activation.
A universal 400-byte tail does not fit all legacy-sized worst-case records;
Terran has only 288 actors and that profile-sized tail fits its measured
296-byte slack by eight bytes, which is capacity arithmetic rather than a
working codec. Five larger actor arrays do not imply the same payload slack.
See the corrected capacity audit and separate wire-design investigation.

Legacy phase conversion, fractional-only repair candidate validation, all
special/healing/morph routes, current-controller/enemy policy, runtime stack/
IRQ bounds, all feature-on production links, ordinary saved gameplay and
physical GBA timing remain required. No actor/impact capacity or memory guard
is reduced. There was no new N64 gameplay observation, hardware test, campaign
win or 30-fps certification in this stage. No proprietary inputs, images, saves
or binaries are published; all checks run locally with no Actions dispatch.
