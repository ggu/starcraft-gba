# Shared custom-game Egg candidate

This note records an **opt-in candidate**, not a release feature or a parity
claim. The runtime and tests were frozen at
`801eb1c21e1e755e8f1dc9f98dfcc6d5a20f3d85`; the comparison base was
`4e949d3f51531d0138455698c2f4f70f6b973d71`. A documentation-only successor
retains that runtime evidence only while the tested source and build inputs
remain unchanged. No default product flag was enabled.

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

## Remaining work

This evidence covers one ARM ROM and one custom-game configuration; it does
not establish all map sizes, three human races, one to three AIs, bot Egg
production, every save migration, physical hardware, or 30 fps. Other
products still need actual guest input and fresh-process SRAM qualification.
Z8–Z10 campaign activation needs its own scheduler and trigger reservations.
The available Egg art proof is static: source Init and birth animations and
shadow remain unfinished. Default-profile and full product-matrix
qualification are separate gates before any promotion.
