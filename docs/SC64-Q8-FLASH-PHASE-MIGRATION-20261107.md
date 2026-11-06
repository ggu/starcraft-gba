# Legacy shield phase migration into the Flash experiment

Runtime/test checkpoint `9c85814b261fc1cafd2d73b260b0cd3ff27f68a7` extends
HOST-only P10 custom Flash staging. Normal release flags, legacy tags and the
new 0x48 payload schema remain unchanged. The separately published ARM
transaction/ordinary-game witness is on another branch and is not combined
with this migration checkpoint.

## Meaning and ordering

The common detached Battery preflight already converts markerless non-P7
**unit** phase from 1/320 into SH12 1/1280, and checks the marker. Marked
shared-Battery custom units already use 1/1280. Rejecting every nonzero old
unit phase after that preflight discarded canonical recoverable state.
The Flash reader now requires the normalized SH12 context before accepting
such old unit rows; it does not guess a scale from the phase's magnitude.

Legacy **building** regeneration still uses 7/320 per update, even when the
unit SH12 marker is present. For legacy 0x43/0x44/0x45 building rows with a
stock shield capability, phases 0..319 convert to four times that value.
The decoder rejects ambiguous old shielded-building phases >=320 and retains
full-shield zero-phase checks. New 0x48 building rows already use 1/1280 and
are never multiplied. Selection re-reads the chosen raw record before final
preparation, preventing a second conversion of an earlier candidate copy.
Nonshield aliases and eligible Terran burn deficit/credit fields are left
untouched; the burn deficit is not duplicated in the sidecar.

The original detached actor, map, job and wallet preflight remains before
fraction admission. Invalid context or phase returns before world publication
or Flash mutation. In-place normalization only affects candidate scratch;
original legacy bytes remain read-only. No actor/impact capacity or existing
memory guard changes.

## Reproducible local evidence

Run the committed bounded tool from a prepared checkout, supplying the exact
clean historical baseline and private input manifest:

```sh
python3 tools/sc64_q8_flash_phase_check.py \
  --baseline /path/to/exact-8127f7af-checkout \
  --inputs /path/to/private-input-hashes.json \
  --input-root /path/to/prepared-input-checkout \
  --output .local/new-phase-output
```

The tool owns the shared native lock, requires 4 GiB free, applies a 256 MiB
per-file compiler limit, bounds commands, halts on sanitizers and scans logs.
The Make targets now refuse a dirty or non-8127 historical producer; their
default is an explicit private baseline path rather than the current tree.
No old image is retagged.

All **39** tool steps passed on this exact checkpoint with 593 pinned inputs
unchanged. Twelve genuine exact-8127 exports retain their before/after hashes.
Authored scenes cover all 1,280 canonical unit phases and all 320 building
phases, including boundary-to-full behavior. Separate processes load each
legacy image, write a new 0x48 record with the first 32 KiB unchanged, fresh-load
it, and compare 47 advancing regeneration updates against the old source
transition. The scenes also retain Marine nonshield alias 9 and Terran
building burn debt 37 plus its credit owner. A deliberately forged,
checksum-valid old building phase 320 rejects with Flash, actors, wallet and
HP debts unchanged. That negative is not called a genuine producer output.
The existing paid-job/actual-packet Flash suite also passes with the formerly
rejected nonzero old-unit phase now loaded and saved successfully.

Independent source review found no actionable defect in the scoped conversion,
version binding or alias preservation. It did not rerun the fixtures.
The first attempt failed compilation on a nested test-main macro and a missing
already-prepared private include link; no fixture ran, and the failures remain
in `.local/phase/first`. Fixture reuse and local links were corrected before
all recorded passes. A first complete local run at `cc993a4a` also passed;
the public tool repeat above records the final reproducible checkpoint.

Source-boundary, doctor, default build, scope checks and exhaustive namespace
inventory passed. A guarded normal research/Flash-off P10 ARM link produced
exact release ROM SHA256
`317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9`.
The full default suite was not repeated solely for this opt-in migration;
its parent combined baseline result remains separately scoped. No current
optional unified/Terran full-mission suite, all-profile migration, campaign,
Z5 cadence, repair, hardware Flash, ordinary guest migration or native N64
regeneration-order claim follows from these HOST scenes.

Private public-tool receipt SHA256 `25f3e723937e1b569ab1dbb0bbe3756aae89f04f8acfee9995a3f884d2179c69`; control receipt SHA256 `fb25b0dc046284e5e7ccaaf052885ba1494a8bce4850298084ce75d872d30953`. All ROMs, inputs, saves and logs remain ignored.
