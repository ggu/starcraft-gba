# Linked Terran T5 victory through T6 and T7

The standard Terran cartridge from the #913 campaign verification was used;
no experimental gas-service flag or scripted world-state injection was used.
Its ROM SHA-256 is
`17f3b4ce1d02e4619cb82828d283bdd523df9e2fe8afe9d12613bfaa41ea86e7`.
The input is the genuine T5 result journal with both surviving heroes,
SHA-256 `1f47c043a05007c926b4d08923998c88516a9289f5517993a2892939910c3202`.
It loads from HOME as a T5 victory with completion mask `0x1f0`.

`tools/sc64_terran_linked_replay.py` loads that journal, uses A on the
result to enter T6, follows the retained ordinary-button controller tape,
checks its observed gameplay fields and dialogue, and freshly reloads the
victory. Its T6 readback becomes the T7 input. Each result reload compares
the persisted world, mission runtime and controls, holds RESULT for eight
neutral updates, and re-exports identical SRAM. The completion mask is read
from the input and its bits are retained, including extra completed chapters.

| Chapter | Controller checkpoint ticks | Dialogue pages | Victory frame | Completion | Victory SRAM SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| T6 | 42 | 13 | 23,536 | `0x3f0` | `328b47d162a292333b091c6ba7c5bb932a0570bd7d18034808e9d372c17b78dc` |
| T7 | 12 | 4 | 3,931 | `0x7f0` | `f8f78c8621ec680cf51dca03809eaae94790546f3bb6e0da81b3a412efd27e6f` |

T6 tape SHA-256 is
`59e1aa7c6524c9cf1b17d31b57858ee0e2d28c8bd0b16ee10dc3233722556bfe`;
T7 tape SHA-256 is
`7971ec31a860a4441c83c85437d45bce46d513b3e909b65dd56be0ccfc3df1d7`.
The linked result and the independent verification agree byte for byte.
The tool reuses the normal replay's Terran state observer and T7 carrier
tracking. Its DWARF layouts are cached only for the pinned ELF during a run.
This proves T5-result-to-T7-result continuation on an emulated cartridge; it
does not by itself establish the entire campaign or physical-hardware timing.
Controller tapes and cartridge assets are local inputs, not included here.
