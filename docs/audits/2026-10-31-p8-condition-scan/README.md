# P8 trigger-condition live-slot probe

The P8 candidate enables `SC64_P8_LIVE_CONDITION_PROBE` in its campaign build; other products retain their existing flags. For each read-only Command or Bring condition, it constructs an ordered list of currently live unit and building slots, calls the original actor provider for every listed slot, and restores the original provider immediately afterward. The list is rebuilt for each condition, so an earlier trigger action that creates or removes an actor is visible to the next condition. The existing source pass cadence, trigger ordering, action callbacks and fault behavior are retained. The list uses 800 bytes of EWRAM at the current 240-unit/160-building limits; it is not saved.

The actual-P8 actor-pool differential fixture compares all nine authenticated P8 conditions for eight owners across the initial world, a dead actor, invalid source owner, invalid raw identity, invalid coordinates, a newly created actor and its removal. The seven cases yield 504 comparisons; all passed under ASan/UBSan. The fixture seeds source owner/raw identity for its synthetic new Marine. Commands and logs are in `.local/p8-condition-differential-v2`; an earlier compile rejected an old fact header lacking geometry revision 2 and is retained separately. An optional `test-sc64-p8-live-condition` Make target requires prepared local P8 inputs and a revision-2 fact header.

Both literal normal `-g` P8 ROM links passed existing guards. Probe-off EWRAM/IWRAM ended at `0x0203899c`/`0x030049e0`; probe-on ended at `0x02038cbc`/`0x030049e0`. The probe-off ROM SHA-256 `1b2700822d8c362c57ac03e228303a58320dce41e98371fbc5cd06584cec2c30` matches the retained public-P8 control; the probe-on ROM SHA-256 is `54cfbe2da8169767424cc71172b1b6319d7be508fb2c58dec62225f7f734cf68`. Arguments, maps, ROM/ELF hashes and verified same-byte compression receipts are in `.local/p8-condition-arm-v2`.

A port-free paired run resumed the same retained natural P8 SRAM state, without injected actors or world changes. The 901 sampled frames matched for the captured actor/building/mineral, visibility, resources, mission and service-clock state, and nine rendered pixel/palette checkpoints matched. Across the paired 900 guest-cycle samples, update mean fell from 1,210,878 to 1,158,897; p95 from 2,988,542 to 2,584,409; p99 from 3,589,422 to 3,165,149. Maximum update cycles changed from 5,027,355 to 5,028,109. The samples are one saved-mission window, not a complete replay, native N64 measurement, or near-30-FPS claim. Full samples, hashes, runner/save provenance and captures are retained in `.local/p8-condition-paired-v1/result.json`.

### Action visibility and error restoration

The actual-P8 host fixture dispatched a source CreateUnit action between
condition evaluations, then invoked a test-only removal callback through the
engine damage path. Both changes were observed by the original and indexed
conditions, and the provider and count were restored after an error. The
ASan/UBSan build and run passed at `fa2e295`; argv and logs are retained in
`.local/p8-action-qualification`. P8's source action bridge does not
implement a kill/remove trigger opcode, so the removal is synthetic and is
not evidence of an authored P8 removal action.

The enabled candidate `71e7714` passed the normal P8 ARM memory guard using
the normal `-g` flags. After the standard ROM header fix, its SHA-256
`54cfbe2da8169767424cc71172b1b6319d7be508fb2c58dec62225f7f734cf68`
matches the earlier enabled paired-run ROM byte for byte. A fresh normal-ROM process loaded the retained SRAM through ordinary inputs,
matched the bounded entity/economy fingerprint at frame 31,677, kept SRAM
unchanged, and continued to victory after 57,985 resumed inputs. This checks
the restored sample and final five fields, not intermediate whole-world parity.
The full normal-ROM input replay won at tick 90,428 / frame 88,769,
matching all 91 five-field checkpoints. Neither replay injects actors or world
state. Exact-source `71e7714` clean unprepared `./dev build` and `./dev test`
passed, including 886 Python tests (23 skipped), native checks, and 5+8
auxiliary tests. Replay argv/results are retained in `.local/p8-enabled-final`;
the clean logs are in the detached `p8-live-condition-clean-20261031`
worktree. These results do not establish whole-mission 30 FPS or native N64
fidelity.
