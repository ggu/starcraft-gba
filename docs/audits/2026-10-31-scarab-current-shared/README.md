# Scarab foundation port to the current shared engine

`fd76f97` merges the reviewed hidden-child stage `ffb5545` with shared
integration `53f1668`. The single textual conflict was at the end of the
Makefile; both the indexed-weapon fixture dependencies and the gated Scarab
fixture definitions are retained. Shared worker orders, weapon caching and
indexing, route flags and current save behavior are carried forward.

The port exposed two fixture integration defects. The legacy-codec test
captured source through an adjacent preprocessor branch, which changed when
the namespace stage was inserted. It now ends at the serialization loop's
own closing line and still compiles the actual production casts in four
enum profiles and both legacy and compact storage. The three explicit
Scarab fixture recipes also used the current indexed weapon flag without
declaring its selected header dependency; they now declare the common
canonical lookup inputs. The focused codec and dependency tests pass.

At source `9c34ef9`, a checkout without private game inputs passed the
source boundary check, default build and complete `./dev test`: 806 Python
tests with 23 optional skips, the separate Python suites and the native C
checks. The earlier failed run is retained locally; its three failures were
the missing fixture dependencies just described. It is not counted as a
passing qualification. The original-art ROM from the final build has
SHA-256 `73c0deeeb36cca4cb8614248a7e60cadac11a0ec542f2ece31c37b171e2c56b8`.

Four normal ARM products built at runtime source `480b4b5`, before the
fixture-only prerequisite correction in `9c34ef9`, with the existing
104 prepared inputs. Their input hashes matched before and after. All
unchanged linker reserve guards passed:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `0x020370b8` | `0x03004b78` |
| Zerg Z6 | `0x02037124` | `0x03004c48` |
| Protoss P7 | `0x020382e4` | `0x03004a18` |
| Showcase | `0x0203df38` | `0x03004e08` |

Terran and Zerg use eight more bytes of IWRAM than the `53f1668` normal
links; the BSS endpoints are equal. These are normal-product links with
both Scarab gates **off**, not namespace-stage-on ARM, gameplay, stack or
frame-rate qualification. The previously reviewed hidden-child host and
journal evidence belongs to its recorded revisions; it is not relabeled
as a complete current-base gameplay pass.

The separate experimental flight-control and clear-motion probes are not
included in this port. Reaver admission, native movement/pathing, arrival
and bullet scheduling, other codecs, complete load atomicity and production
performance remain open. No generated data, ROMs or captures are committed,
and no Actions or SD writes were performed.
