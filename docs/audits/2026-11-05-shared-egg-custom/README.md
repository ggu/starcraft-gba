# Shared custom-game Egg candidate

Updated 2026-11-06. This is an **opt-in custom-game candidate**, not default
activation or a claim of campaign, hardware or performance parity. The current
qualified runtime is `6ba76f8e86b5bcc68a621457e071f333d5f8d007`. Earlier
`801eb1c2` evidence is retained below with its exact scope and subsequent fixes.
Publication adds documentation, owner fixtures and separate preview recipes;
the qualified game and common-runtime Git objects remain unchanged.

## Current qualification

- Six normal product ROMs and unchanged RAM guards passed at the current
  runtime. A clean asset-free build/test passed 999 Python tests, 26 skipped,
  and the compiled suites. The normal rendering path includes the legacy
  Overlord correction; it is not byte-identical to the older release.
- All five opt-in GBA products built and verified: Terran, delivered Z9, P10,
  Showcase and legacy Z7. Terran, P10 and Showcase ROMs match their earlier
  801 versions. Z7 and Z9 use the corrected compatibility runtime. Z9 has
  796 bytes of EWRAM linker slack to its reserve; Z7 has 120 bytes of IWRAM
  linker slack. Those figures are not measured stack depth.
- All five Egg-only HOST controller, journal and rendering bindings passed.
  Additional test-only `04dc9506` fixtures on the same runtime passed gas
  payments/refunds for all four active owners, both P10 Carrier/Egg reservation
  orders and mixed command-row dispatch. Their setup is deliberately constructed.
- Ordinary HOST inputs cover Zerg production/save/cancel/load/birth on Outpost
  and Blood Bath with one to three configured AIs. Terran and Protoss startup
  checks add twelve configurations. These are eighteen starts, not full matches.
- The exact Terran opt-in ROM passed real ARM button input and fresh-process
  SRAM continuation. A separate derived read-only runner also matched all
  88 opaque source pixels of the selected Egg and showed its product/progress
  HUD advancing, without changing guest clock or named state during reads.
- Preview recipes were actually built for all five products and matched the
  qualified ROM hashes. Their clean asset-free suite passed 1,000 Python tests,
  26 skipped, plus compiled suites; the normal default ROM remained unchanged.
  The publication verifier additionally rejects symbols below their RAM regions;
  its boundary tests and verification of all five actual previews pass.

See [preview commands](../../RACE-CAMPAIGNS.md). The Egg service stays off in normal targets.
No ROMs, private data, screenshots or saves are distributed with this source.

## Capability and source review

`SC64_SHARED_EGG_CUSTOM_SERVICE` is a deliberately disabled-by-default
capability. It binds the existing shared paid Egg producer and its staged
wire identity to custom games, independently of the staged Guardian/Cocoon
service. Its compile-time prerequisites include race rules, imported
skirmish, worker-cargo and shared transport, and shared Scourge combat. It
rejects the competing Guardian and Scarab namespaces. Legacy Z7 continues to
use its existing Egg identity and mission validator; the opt-in adds no
Z8–Z10 campaign producer or scheduler reservation.

Custom saves use the existing Egg outer/inner journal tag pair. Prepared-slot
selection checks the outer tag and unchanged inner version together after the
journal checksum; candidate-local context is carried through validation and
unit publication, including compact, derived, and P7 typed codecs. A legacy
custom row cannot gain Egg authority merely from its raw actor value. The
candidate validator also checks owner identity, product and timer, and
available committed unit slots. Failed newer candidates can fall back to an
older valid slot. These are integrity and format checks, not cryptographic
signatures or protection against an adversary who can rewrite a whole save.
An independent read-only source review of the frozen diff found no concrete
correctness, security, or compatibility issue in these paths; it did not run
builds or tests and does not replace the qualification below.

The selected-unit HUD uses the shared Egg actor and product progress. A
legacy Overlord retaining raw 36 resolves its visual identity to Overlord art
without changing its saved identity. That visual correction also changes
source compiled with the service off; the opt-in gate being off is not a
claim that every default binary is byte-identical to the comparison base.

## HOST evidence

At the frozen source, sanitized HOST ordinary-input fixtures passed on the
Terran, Z7, Z9, P10 and Showcase build vectors. Each used the player
controller to configure a custom game, select a Hatchery, pay for a Drone
Egg, save, cancel, reload and wait for birth. The accompanying constructed
HOST rendering checks compared source Egg pixels and selected-unit portrait,
product and progress display. These checks are not guest screenshots.

Egg-only constructed HOST journal fixtures passed on all five vectors,
without Guardian enabled. They covered round trip, outer/inner mismatch,
legacy-tagged custom Egg rejection, and fallback to an older valid slot. A
separate constructed Z7 campaign fixture saved and loaded a paid Egg using
the ordinary transport tag with no Egg feature context. These cases do not
qualify all historical save generations.

With the prepared private profile inputs already available, and the native
lock and disk reserve in place, the HOST commands were the targets in `tests/sc64-shared-egg-custom/Makefile`:

```sh
make -f tests/sc64-shared-egg-custom/Makefile custom-egg-facts
make -f tests/sc64-shared-egg-custom/Makefile \
  custom-egg-terran custom-egg-z7 custom-egg-z9 custom-egg-p10 custom-egg-showcase \
  custom-egg-z7-legacy
make -f tests/sc64-shared-egg-custom/Makefile \
  custom-egg-journal-terran custom-egg-journal-z7 custom-egg-journal-z9 \
  custom-egg-journal-p10 custom-egg-journal-showcase
```

The Egg-only P10 fact table was generated in a separate ignored directory;
its SHA-256 was
`30b164d622db6fe8f86836a36ae5f3fcf521aa964ef845ba029852a384abfa91`.
The final run was serialized under the native-build lock and checked the
frozen clean source and all 1,959 pinned inputs before and after. Private
logs were retained at `/tmp/sc64-egg-final-test.log` and
`/tmp/sc64-egg-final-journal.log`.

## ARM and fresh-process SRAM evidence

The frozen opt-in Terran ARM ROM built and passed ROM-header and RAM-reserve
checks. Its SHA-256 was
`a7975958e1af1ba74c3025183bf019f0f9278442812aa0ceb8c215e44c1db03b`;
`__bss_end` was `0x02036f1c` and `__iwram_end` was `0x030048a8`.
The private build/verification receipt is
`/tmp/sc64-custom-egg-arm-final.log`.

A separate port-free physical-button run used that exact ROM and an
ordinary HOST-recorded input tape; the tape SHA-256 was
`79ab4aece7e9202b9d6a45cf3ad8b9bd8eae46c72b5eb813fe01ad78a4a87da1`.
The scenario used the default custom map, Zerg human and one Terran AI. On
ARM, the player paid 50 minerals, producing an Egg with raw type 36, Drone
target and 373 ticks remaining; pause-menu input saved it at game frame 6.
The first process exported its actual 32 KiB SRAM, SHA-256
`7dd7804073e15e2d376521bb9a1867a12c5b9bb53ba77709c5a5134e1a94a478`.
A fresh process imported that exact file at boot and loaded it using HOME L.
The restored named-world fingerprint and Egg timer matched the saved world.
After 401 game frames, the Egg had naturally become a Drone in both runs,
and their named-world fingerprints matched. SRAM was unchanged by load.

Only physical button input, read-only named state and emulated clocks were
used. No units, resources, guest RAM or fabricated SRAM were injected, and no
port 2345 was used. The fingerprint covers the named fields in
`tools/sc64_skirmish_sram_continuation.py`, not every transient byte. The
proof script and receipt remain private at
`/tmp/sc64-egg-arm-proof-801/proof.py` and
`/tmp/sc64-egg-arm-proof-801/result.json`; the recorded invocation was
`python3 /tmp/sc64-egg-arm-proof-801/proof.py`. That script requires new
output paths for another run and deliberately refuses to overwrite SRAM.
The test held the native lock and checked source, 1,959 input pins, ROM, ELF,
runner, tape and free space before and after.

## Compile-fence successor

Source successor `116655c5b9c42a01294281d63ad161f4442c0700` rejects
`SC64_SHARED_EGG_CUSTOM_SERVICE` when either the older
`SC64_SHARED_EGG_HOST_STAGE` or `SC64_SHARED_EGG_JOURNAL_HOST_STAGE` is also
selected. Those older stages can otherwise enable campaign contexts that the
custom-only writer does not authorize. The named-error preprocessor checks
cover each older flag alone and the pair on the Z9 HOST vector:

```sh
make -f tests/sc64-shared-egg-custom/Makefile custom-egg-conflict-guards
```

At that successor, the valid Terran opt-in ARM ROM was rebuilt with the same
flags and pinned inputs and compared byte-for-byte with the `801eb1c2` ROM:
both have SHA-256
`a7975958e1af1ba74c3025183bf019f0f9278442812aa0ceb8c215e44c1db03b`.
The linked `.text`, `.data` and `.iwram` bytes and addresses, and `.bss`
address and size, also match. The ELF debug information differs after the
source-line change. This supports carrying the preceding ARM runtime evidence
for that exact flag vector; it does not add coverage for other builds. The
three rejection checks, clean source, all 1,959 pinned inputs and unchanged
RAM guards were checked under the native lock. Private receipts are
`/tmp/sc64-custom-egg-fenced-qualification.log` and
`/tmp/sc64-custom-egg-fenced-receipt.json`.

## Pre-cargo campaign compatibility successor

The custom candidate originally retained the selected predecessor's inner
version so that its authentication context remained bound. The Z6 and Z7
campaign validators nevertheless required the current version, so they
rejected a genuine pre-cargo journal that the normal reader accepted. This
was reproduced on `85cb8c32` with a clean `17dc9b3e` normal HOST writer
compiled with the canonical Z7 flags and `-USC64_WORKER_CARGO_KIND`. The Z7
fixture constructed and paid for a Drone Egg; the Z6 fixture saved the
initial scenario. These are HOST-constructed states, not ordinary-input guest
proof. The separate fresh normal reader loaded each file, and the opt-in
reader rejected each. Their private SRAM hashes were respectively
`d60a265c87aa75e3189bbfeecc850b127ae66af9d96f94d5596b13b37792cc74`
and `4ef1264eaddcd38c09f021937be57b3699d25b13a21fae0adbc74ec2d6d3dad5`.

At source successor `6ba76f8e86b5bcc68a621457e071f333d5f8d007`, the Z6 and
Z7 load preflight permits the predecessor version only after successful cargo
preparation and only when the selected context has the exact legacy outer
and inner pair, zero staged features, and the same candidate pointer. It
passes that expected version to the campaign validator; it does not rewrite
the inner version behind the token. Save-time validation still requires the
current version. An independent read-only review found no actionable issue
in this scoped change; it did not execute tests.

Fresh HOST readers at that successor restored both actual predecessor files,
as well as newly generated equivalents from the same old writer. Tests also
rejected wrong candidate pointers, changed token versions, and checksum-valid
outer/inner mismatches. A malformed newer candidate fell back to the older
pre-cargo slot, without changing the HOST SRAM; mismatch checks also compared
the live unit and building arrays. The fixture is deliberately HOST-only:
never import its constructed SRAM into a guest. To reproduce it, provide a
clean matching `17dc9b3e` checkout and a new private output directory:

```sh
make -f tests/sc64-shared-egg-custom/Makefile pre-cargo-campaign-build \
  PRE_CARGO_OLD_ROOT=/path/to/clean/17dc-checkout \
  PRE_CARGO_OUT=/path/to/new/private-output
```

Run `campaign-old-writer`, `campaign-normal-reader`, and
`campaign-optin-reader` from that output directory in separate processes for
mission arguments `6` and `7`, passing each the same new HOST save path. The
complete guarded run and hashes are in the private
`/tmp/sc64-pre-cargo-egg-compat-v2/qualification.json` and `qualify.log`.

At `6ba76f8e`, the five custom controller/render and Egg journal HOST
vectors, Z7 ordinary-envelope campaign fixture, and compile-fence checks also
passed. Opt-in Z7 and Z9 ARM ROM/header/RAM checks passed; their ROM hashes
were `9779755a7c5343a6574682b28ee7e40d5090ded03211770b3560282a26a89860`
and `66ac6e8cf5a66b8703cca31c293cc8e05eada66a7bba31c924e862fe465fca50`.
These changed opt-in ROMs do not inherit the earlier Terran ARM playthrough
as proof of their own gameplay. Service-off Z7 and Z9 ROMs rebuilt from
`801eb1c2` and `6ba76f8e` were byte-identical for their respective profiles.
The private ARM and service-off comparison receipts are
`/tmp/sc64-pre-cargo-egg-compat-v2/arm/receipt.json` and
`/tmp/sc64-pre-cargo-egg-compat-v2/normal-compare-receipt.json`. All runs
were serialized and verified clean source and 1,959 pinned inputs before and
after. Other product and guest qualification remains separate.

## Remaining work

Guest gameplay and fresh-process SRAM evidence still cover one Terran ROM
and one custom configuration; the broader setup checks are HOST evidence.
Other products need their own guest continuation, long-match/AI behavior and
performance checks. Known predecessor Z6/Z7 generations are tested, not every
historical save. Physical hardware and near-30-fps behavior remain unproven.
Z8–Z10 campaign activation needs scheduler and trigger reservations. Egg Init,
birth animation and shadow are not implemented; the selected Hatchery also
does not list independent jobs. These remain work, despite the static-frame,
selected-Egg HUD and normal/preview build checks above.
