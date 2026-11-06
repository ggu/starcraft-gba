# Opt-in shared Egg selector ARM probes

Checkpoint `55412d5d0250fae6c9110d77acd0a2193ee8704a` combines the reviewed
HOST-only early-campaign stages with the optional selector from draft #298.
The selector chooses the existing late context for Z9/Z10 and the custom
context for Terran, Z7, P10 and Showcase. No normal product flags or Make
recipes enable the selector. A controller-only Z4 picker fix follows the
actual filtered producer list and retains the current L-cancel-safe controls.
The checked-in probe recipes produce isolated outputs; they do not replace
normal products. The early campaign stages still explicitly require GBA_HOST.

Under the shared native lock, at least 4 GiB plus 256 MiB free and bounded
commands, 19 steps passed: selector off/custom/late contracts; Z7 legacy and
Z6 selector HOST journal checks including actual fresh writer/reader and a
genuine old-tag Z6 save; P10's separately generated Egg-aware fact table;
and six off plus six explicitly enabled ARM links. All ROM/header/SRAM and
linker RAM guards passed. Every log was scanned for sanitizer diagnostics, and
the 594 input hashes and genuine old save were checked throughout. The
private receipt SHA-256 is
`4757a661b45eb07000316c55a3aea7d81d63e80a7d07ec43b1eabc13806a8720`;
the input manifest hash is
`6c467d390599bcf22778860aa2a2efbe2266cc123347c913983af48cafacaa46`.
The P10 Egg-aware fact header hash was
`30b164d622db6fe8f86836a36ae5f3fcf521aa964ef845ba029852a384abfa91`.

| Product | Selector off ROM SHA-256 | Selector on ROM SHA-256 |
| --- | --- | --- |
| Terran | `00b01425480fb1b01cd4720ce36fe9eba274dc34cde2de351e8909ff743d70f8` | `38e1e9018347cd677ba5e0a61de9424a0934843812dec89a0c26f7c842d56ee6` |
| Z7 | `11e2e43369e3fe4307541f6ae1afa92b340f2d3c09f6825120abafc5dd75728f` | `13059f71df0a8a0f1bfcd9e30fe9c6c54ec4a7f506585893e35b17bbaa84f608` |
| Z9 | `aee95a66560b5fc5de7ae7dff8d906cd6035ba62eb50c6051ea6a1301b6d1549` | `5c9f0fd742f417ead6d3f0e681c8dde9fcbae279111956edb48c859cc1883250` |
| Z10 | `9c5304c5797c678ac0a33979b97ad8c7a7415a1be4eadacd5caf92a2a30743eb` | `4ba9ae5ab00de270d0ea34d7004268e5ec2111890de33fb4a2e4369697b88924` |
| P10 | `317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9` | `44ee332df224e6ffd33113f8044052cc138ae52a3825ee7b91d023ee6aa8c6a1` |
| Showcase | `8f5bc974319a40ab6abaaff99222db2b4bb16cade5e1f1eb28321ef66f6f34a5` | `6b6def1a43c3e887c4b8b3c2a1818e7ff162a27cd77f42adb4cb4203f176de92` |

The four normal off ROM hashes match the earlier current-package receipt. The
smallest EWRAM margin is Z9/Z10's 780 bytes to its guard; selector-on Z7 ends
at `0x03004b70` in IWRAM. These are static link bounds, not stack high-water
measurements. No current ARM build enables early-campaign Egg production, no
ordinary early guest Egg/save/birth was tested, and no performance or hardware
claim follows. A guest stage still needs an explicit bounded non-default gate,
ARM storage and stack proof, authenticated source/current-controller and
pending-create checks, physical-button input from a fresh start, actual SRAM
save/reload and natural birth. Z5 remains the separately audited no-new-Egg
source closure. Existing source evidence shows Z6 starts with a source-3
Hatchery and four Drones, making a normal-input Drone Egg a possible next
probe; minerals begin at zero, so it must first be earned in-game. Do not
inject resources, actors, SRAM or retagged saves to create the checkpoint.
