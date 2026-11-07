# Combined experimental Flash and phase migration checkpoint

This combines the phase-migration branch (#348), ARM Flash transaction (#349),
current main `df5c8ee8`, and explicit HOST-only custom-roster admission.
Tested source is `8be8245856a1c45dc281ba75dd109c0260dd03f4`.
Normal product vectors do not enable research jobs, Q8 combat, or Flash.

## Checks completed

- The combined runtime at predecessor `7de73fb8` passes the public phase checker:
  39 process steps, all 1,280 unit phases and 320 legacy building phases,
  47 advancing updates, genuine old exports and alias/burn preservation.
  The only later source addition is actual-record malformed-roster tests.
- Authored Terran and Zerg custom writer/fresh-reader checks pass. At `8be82458`,
  checksum-valid mismatched human/bot rosters pass through the actual loader's
  refusal/fallback checks. This extension remains explicitly HOST-only; it
  does not broaden the ARM prototype's ordinary custom admission.
- The transaction and audio-ownership callbacks pass focused ASan/UBSan checks,
  with halt-on-diagnostic and explicit diagnostic scanning.
- All six actual experimental ARM vectors link with their existing reserves:

| Product | EWRAM guard margin | IWRAM guard margin |
| --- | ---: | ---: |
| Protoss10 | 19,324 B | 2,056 B |
| Terran | 28,336 B | 2,856 B |
| Zerg9 | 132 B | 1,936 B |
| Showcase | 28,676 B | 1,464 B |
| Zerg7 | 2,292 B | 1,024 B |
| Zerg10 | 132 B | 1,936 B |

These are static linker margins, not runtime stack/high-water or hardware proof.
All 593 prepared input hashes and all 14 retained ELF/map/ROM hashes were
independently rechecked. The resource receipt's SHA256 is recorded below.

## Ordinary input checkpoint

The exact combined P10 ELF and ROM ran with the audited unforced mGBA physical
backend (`75b8ba5f…`) and a virgin 128 KiB Flash backing through its normal VFile
API. The controller tape uses title/custom selection, natural resource gathering,
paid Forge research, pause/B save and a fresh guest's title/L load. No guest
memory or registers were written to arrange the match.

The paid and saved wallet is 376 minerals/12 gas, job timer 4,999. A fresh load's
first ordinary update has the same wallet and timer 4,998; research subsequently
completes level one. Exported Flash is 128 KiB, leaves the first 32 KiB erased,
and contains the committed 0x48 record. This is named-state evidence for one
Protoss custom game against one AI, not full-world, fractional-damage save,
other-race ARM, campaign, audio-continuity or real-cartridge qualification.

| Artifact | SHA256 |
| --- | --- |
| Experimental P10 ELF | `4246b1f55387d119c532a6dff54ef7fc38702223a8c696d0d1687ae6e79a26a1` |
| Experimental P10 ROM | `f5caf32d107c72ed586853690c80dc6c53600720498e075a811f095e48f348c9` |
| Guest Flash image | `27d75e4fa4bfc1247cd3965129196ba6174ee0737d52f5821af6f35b9b4c7a7d` |
| Normal feature-off P10 ROM | `317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9` |

The feature-off control was independently linked in a separate worktree at the
same source and exactly matches the current playable Mac package. Its run also
passes namespace and experimental-scope guards. It does not overwrite the
retained experimental ELF/map.

## Remaining gates

Normal release activation is still prohibited by the outstanding qualification:
all supported mode/roster codecs and campaign source policy, genuine saved games
including repair/Z5 aliases and fractional damage, transaction failure/recovery,
stack/high-water, audio continuity, hardware Flash behavior, and longer custom
matches against independently selected 1–3 AIs. Z9/Z10's 132-byte static margin
also needs relief before deployment. No new ROM was copied to KODAK.

Raw proprietary data, ROMs, saves and native logs remain private. The public
repository contains authored source, optional checks and this scoped audit.

## Retained private receipt hashes

- Combined focused checks: `5bb5787eac2df356e0e48ccd5f085841a9a05f84ecfa85ea1ea2e6abe06feaff`.
- Six ARM resource checks: `4ba11051d4661f61225096b2130bc2b87a3797f04b344d27261c60e3832d20f0`.
- Ordinary-input guest result: `55f37191165f66f3cdd0243a948902864b419b1d2a2603fc916cb314e7e6ae0a`.
- Normal P10 control: `46b8c45661d8a1cb67b5f0abfc34b7110129b0efabd71c743ad97943e563b3b4`.

## Clean public default checks

At the same `8be82458` source, the fresh asset-free worktree passes source
boundary, `./dev doctor`, `./dev build` and the complete `./dev test` command.
The last includes 1,026 Python tests (26 expected skips) plus compiled and
auxiliary checks; the recorded test retry returns zero in 285.542 seconds with
halt-on-sanitizer and no scanned diagnostics. No private game inputs were linked.

The first supervisory postcondition incorrectly forbade any `build/generated`
output. The public test legitimately creates an authored contact-geometry table.
That failed supervisory attempt is retained; an independent regeneration matched
the table, and the corrected exact allowlist passed the full test rerun. This
was a test-harness correction, with no product-source changes.

Final private receipt SHA256: `c1bc2900b4035329c8960c67743141947e1861a7488a795455ad4a3580380324`.
