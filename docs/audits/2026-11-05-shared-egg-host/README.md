<!-- Historical first-stage record; later progress follows. -->

# Shared Egg HOST staging

This source-only experiment starts from `3d209a64` and is disabled in normal
builds. `SC64_SHARED_EGG_HOST_STAGE` is accepted only for HOST builds with the
Z9 Egg vector. It reuses the already-existing Z7 Egg identity and fields; it
does not change save tags, actor enums, legacy Z7 policies or guest menus.

Outside Z7, the stage permits existing paid larva products to start independent
Egg jobs in Z8, Z9 or custom games, subject to the existing owner, producer,
prerequisite, catalog, supply, slot and resource checks. An Egg holds its
producer's source owner and retains it through birth, including a paired unit.
Its pending product, timer and supply use the existing Z7 rules. The HOST-only
owner-generic cancel helper refunds the product cost and consumes the Egg;
the existing Z7 cancel menu remains Z7-only. No new per-Egg storage is added.

Until a separate namespace and per-codec proof exist, stage saves refuse any
late/custom live or dead Egg row and the load preflight rejects such candidate
rows; Z7's existing strict save path is unchanged. HOST fixtures may construct
invalid candidates; no forged guest SRAM is used. Tests do not prove Z10 wins,
native Egg animation or timing, full campaign availability/PUNI parity, nor
safe migration of existing in-flight queue jobs. No ROM or save is published.

---

# Shared Egg late-campaign HOST prototype

This is a HOST-only experiment based on canonical `c96c84b7dfa8f562457e73d29969890aa4ba3bd1`. It does not enable guest menus, default ROM features, Guardian/Egg combined saves, or claim a mission win. The production capacity prototype is `3d8ea7a3`; test-only successors through `d5ab6775` add no runtime changes. Existing Z7 retains its original policies and tags.

The staged journal authenticates the selected outer and inner tags before decoding the candidate. Z8/Z9 ownership derives from the saved mission and source descriptor, while custom ownership derives from saved player state. Ordinary old-tag raw-36 actors remain on their legacy path. Paid queues and extra paired Egg bodies reserve free slots. In late campaigns, pending CreateUnit actions in executing scheduler instances reserve their remaining bodies from each instance's cursor; actions before the cursor are already represented in the live pool. Trigger actions activated after paid production may defer at capacity instead of faulting; this is a provisional HOST capacity adaptation, not established native timing behavior. A paired Egg at its last tick waits if its second slot is unavailable.

The pinned 1,921-input receipt used for these runs is `/tmp/sc64-z9-main-3d/.local/qualification/prepared-inputs.json`. Verify each path and SHA-256 against a prepared independent copy before and after running; use `fcntl.flock(LOCK_EX)` on `/tmp/sc64-native-build-20261031.lock` and require over 2 GiB free. Run from the exact clean source head and place outputs in a separate directory. Supply each receipt path to make as `-o PATH`, preserving all generator dependencies and contents. The relevant targets are:

```
make -f tests/sc64-shared-egg/Makefile EGG_TEST_BIN_DIR=OUTPUT \
  test-shared-egg-capacity test-shared-egg-z9-journal \
  test-shared-egg-z8-journal test-shared-egg-custom-journal \
  test-zl-detached-candidate-binding
```

At `3d8ea7a3` those five targets passed with AddressSanitizer and UndefinedBehaviorSanitizer, with matching source and inputs before/after. Test-only successor `0c3621e5` passed a paid Zergling Egg save/load and paired completion; `ced27cb8` passed an actual queue boundary with two pending Z9 creates; `10b6f30b` passed a real Z9 create callback defer/retry; and `d5ab6775` passed a paid pair after save/load waiting at timer 1 under HOST-only artificial pool saturation. Local logs: `/tmp/sc64-egg-3d8.log`, `/tmp/sc64-egg-0c36.log`, `/tmp/sc64-egg-ced2.log`, `/tmp/sc64-egg-10b6.log`, and `/tmp/sc64-egg-d5ab.log`. Earlier `fabfa546` and `fa47cd5b` batches failed, respectively, from a fixture macro redefinition and a stale old-tag expectation; neither is a complete passing batch.

Remaining gates include wider scheduler/production interleavings, Z8 callback capacity tests, four-product authenticated codec/journal integration, Guardian namespace coexistence, StageOFF equality against `c96c84b7`, memory/ROM budgets, and actual guest input and real SRAM continuation. No synthetic guest SRAM was used.

## Authenticated deferred trigger continuation

The earlier `3d8ea7a3` result did not prove Z9's separate action dispatcher or permit saving a deferred create cursor; source review identified both gaps. The final runtime candidate is `56af90a0`. Its staged scheduler pack, detached preflight and final apply accept an unarmed CreateUnit cursor only when the candidate-local Egg authentication is present; legacy wrappers continue to reject that cursor. A legacy-tag rewrite of the Z9 deferred record was rejected without publishing candidate state.

The frozen seven-target batch passed at `56af90a0` with the same input receipt and lock/disk guard:

```
make -f tests/sc64-shared-egg/Makefile EGG_TEST_BIN_DIR=OUTPUT \
  test-shared-egg-z8-deferred-journal test-shared-egg-deferred-journal \
  test-shared-egg-capacity test-shared-egg-z9-journal \
  test-shared-egg-z8-journal test-shared-egg-custom-journal \
  test-zl-detached-candidate-binding
```

The Z9 test activates the actual source trigger while one free slot is committed to a legitimately paid paired Egg, verifies its create defers, saves and reloads the real journal, rejects a legacy-tag version, then kills two fixture-created units and resumes through the crystal create. The Z8 test executes the actual six-unit trigger after its source wait, saves and reloads while blocked, then frees six units and resumes. Those fixture-created units and capacity states exist only in HOST. Local receipt `/tmp/sc64-egg-56af.log` records matching pre/post source and input checks. `b2a9cb3e` failed before this fix because the scheduler rejected a deferred create cursor during save; its log remains `/tmp/sc64-egg-b2a9.log`.

Paid outputs are slot commitments. An activated but unallocated trigger create is retryable: it may coexist with a valid save even when paid bodies plus all pending creates exceed current free slots. This is a staged capacity adaptation; native behavior and timing are not established. Legacy Z7 and the ordinary game profiles remain out of this experiment. The test-only successor `da829467` also passed the Z8 legacy-tag deferred-cursor rejection and retained the real-stage retry (`/tmp/sc64-egg-da82.log`). Broader interleavings and the four-product, Guardian, StageOFF and guest gates above remain open.
