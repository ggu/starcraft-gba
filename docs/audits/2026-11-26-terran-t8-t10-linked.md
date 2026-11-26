# Linked Terran T8 through T10 with finite gas

The standard Terran ROM (`17f3b4ce1d02e4619cb82828d283bdd523df9e2fe8afe9d12613bfaa41ea86e7`)
loads the T7 victory produced by the [linked T6–T7 run](2026-11-26-terran-t6-t7-linked.md).
Each result advances with ordinary A input and supplies its freshly reloaded
victory SRAM to the next mission. The replay uses physical button input and
never writes game state. It compares every retained gameplay checkpoint,
including resource and objective values; no wallet or objective differences
are waived.

| Mission | Checked ticks | Victory frame | Completion | Freshly reloaded victory SRAM SHA-256 |
| --- | ---: | ---: | --- | --- |
| T8 | 467 | 285,946 | `0xff0` | `2233638de1b8ed50541c169162e3ca20672c076912b65e16dd34e958496dbf97` |
| T9 | 54 | 49,486 | `0x1ff0` | `0562b6ca094bf595752e4486eeb55c0238b1b6a820697f7e90ddb0aee6b2b64b` |
| T10 | 6 | 3,361 | `0x3ff0` | `147b1f62ca27455d99094ae8f16f7d55df14094f7b4ddbbe49a4c88ca4063eb4` |

T8 uses the longer finite-gas controller tape, SHA-256
`96e9ef4157fd53aadaaa32716d96bba328d6a3cc2555b1eb802ab37c72dd5cd4`;
T9 uses its finite-gas tape,
`503846e32dba7abf6eab01ac29c4e3984bb611c8ef83d5e153adb1e792574b91`.
The older T8/T9 tapes are not substituted: their wallet values diverge on
this finite-gas cartridge. The final journal includes all ten Terran bits.
A fresh HOME load restores each RESULT and re-exports byte-identical SRAM.
The emulated linked run starts at the carried T5 victory and is not a
physical-hardware timing result. Cartridge assets and private controller
tapes remain local inputs.
