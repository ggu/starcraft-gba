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
