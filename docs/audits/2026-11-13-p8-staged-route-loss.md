# P8 retained route on staged normal P10

The retained carried-P8 tape launched The Battle of the Conclave from
fresh HOME on the staged typed-reader P10 ROM. A bounded test matched
both checkpoints through tape call 2,199. The full attempt matched five
checkpoints through tape call 9,459 (frame 9,183) across eight named
fields. This did not compare the whole actor state.

The first frame mismatch was at zero-based tape index 11,223: the tape
pressed B and stayed at frame 10,892; on the cartridge, the field menu
stayed closed and the game advanced. The bounded forensic replay then
observed RESULT at tape index 11,235, frame 10,905, with a loss, human
runner outcome 2, defeat trigger bit 2 set, and no runtime faults.
The authored trigger is Fenix's absence after creation. The Stasis Cell
still had 2,000 HP. The tape's next A at index 11,259 restarted the
mission at frame zero; the later frame 321 checkpoint was in the new
attempt. No pause-menu load took place. This is an observed gameplay
loss on one old tape, not missing mission functionality.

A new tape should use the current P10 flag vector and log Fenix's
source slot, health, position and order before the opening raids,
alongside Hyperion and the selected worker. Establish actor parity
before treating the menu mismatch as an input translation problem.
Preserve ordinary inputs and stop replay at the first unexpected
RESULT.

| Input or receipt | SHA-256 |
| --- | --- |
| Staged P10 ROM | `0505b233994dca90cda47d92f1a153a14f01e781d980b3498390d3b4005e7c41` |
| Matching ELF | `6319d1647b73ffa7ac9c0f165c1db1a8dbdf3a5792dc066c97024203f6672d44` |
| Carried P8 tape | `63a5085ac4b96598f2e24aa4f77bfab9b55475080acf03587e628a40501bd788` |
| Port-free physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Full-attempt divergence receipt | `b0cda627ef446256bcec0d28fca14b7a6ee3d662bc7a41f2208647f5da4a6a60` |
| Bounded forensic driver | `4421ff7b8bbce2b67f3779179437b51c52e778d29abf4a984759e5995ca64c0c` |
| Bounded forensic transition log | `1000e6f1e59d225d3b9c7c5c6f75bf05535479738ade32d188b415c2c686a20c` |
