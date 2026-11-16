# Native T1 campaign save selection

In the owned USA StarCraft 64 ROM, an ordinary save followed by a fresh-process title load of Terran 1 (Wasteland) did not restore the selected Command Center. The live game kept its selection when the save menu closed. Manually selecting the building again after load restored its ring, name, and commands.

The private ares copy used process-local controller polling and screenshot hotkeys, an isolated settings file and save directory, and an initially empty save directory. Emulator save/load-state hotkeys were disabled; the witness used no emulator state, guest memory edits or cheat. The fresh campaign menu exposed Episode I, so the run used Wasteland. Starting the mission and moving the field camera to the base were ordinary controller actions. After selecting the Command Center, the path was Start → Options → Load / Save → Save This Game → first Empty slot. That process (10147) was terminated after the flash was already on disk. The new process (16691) opened title Load Saved, displayed `Terr01 04:42` and Wasteland, then received one A press to load and a neutral wait before the first stable field screenshot.

The fresh load restored the base camera and the same 40 minerals and 10/18 supply. The Command Center ring, top-left name and building commands were absent until the manual reselect. This is a direct observation of the native T1 title load, not of Z1, Z7, in-mission load, unit groups or every building type. It is consistent with normal GBA Z7 clearing the Hatchery selection on load and needing a fresh A selection before the continuation. The [opt-in GBA save UI experiment](../Z7-SAVE-UI-AUDIT.md) establishes a separate GBA behavior; this witness does not justify enabling that extension as native parity.

| Local artifact | SHA-256 |
| --- | --- |
| Owned USA ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| ares executable | `4ce08c105b4f930d9a86bf03c8ea51c5867fa3e29c942997a868b2b158a4d51d` |
| Process-local input shim | `ba9227ef9a14deec020b62affc7d92eb015d51d44d90eb8edacf1cb784f7ff70` |
| Persisted flash, before and after fresh load | `7b08b269ce9afa7dabaf093a91594b83d3ca97e02f86fca53bd68a9d40874e01` |
| 11:53:40, Command Center selected before save | `cfb117fac3d36908a083a750965f2c6a685a7a389a699b2b7435b4e687643a5b` |
| 11:54:33, selection ring after live save | `627e305416943dcb58c1c9293698d111118e38ed6b52d66d3be7f47cd209b8a8` |
| 11:55:44, title save entry in new process | `ff4e341c172cb89b0ca9097f815ab013173c9de582ae27e44aca72171281c043` |
| 11:55:57, fresh load with no selection | `42253cc6b1cc07c8fa8ec726f88d0fc31249414f0f759f0794c71811a438fbd8` |
| 11:57:09, manual reselect | `a6d1b637fa22f3efc5478b7f8f16f38a167bba6f73d17df6ac372065e05f90a4` |

Only these observations and hashes are tracked. The owned ROM, flash, screenshots and emulator remain local.
