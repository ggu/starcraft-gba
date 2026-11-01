# Projectile decode destination preflight

The packet validator checks the serialized projectile pool independently of
its destination. Before this change, candidate save preflight could accept a
valid packet even when the eventual arena descriptor was too small or the
packet overlapped the destination. The later decoder would reject it only
after other load state had been installed.

`sc64_codec_validate_target` shares the decoder's existing pre-write arena,
packet-size, address-overflow, overlap and packet checks. The P7 projectile
candidate validator uses it before live-world commit. Packet-only validation
and the decoder's post-write `sc64_pool_validate` return remain unchanged.
The caller must preserve both packet bytes and arena descriptor until decode.
This removes an early target-check gap; it does not establish an infallible
commit, a fully atomic loader, or a new save format.

On source head `7681a2fab1afc6b038d7ad0544104872f6571fdd`, the asset-free
codec and motion fixtures passed under ASan/UBSan. They include valid packet
versus invalid, short, overflowing or overlapping target checks, unchanged
canaries and packet, plus an ordinary decode/re-encode witness. The canonical
P7 and P8 host fixtures passed an ordinary-save candidate check, including
short-arena rejection and preserved world/services. The same P7 fixture,
compiled against base `30858e34246418c6847d8c7e11a779b3ef701b12`, reached
its expected short-arena rejection assertion, demonstrating the old gap.
The fixture's temporary baseline copy changed only its included source path.

An unprepared clean source boundary, doctor, default build and full default
test passed: 935 tests (26 skipped), plus groups of 5 and 8. Canonical P7 and
P8 ARM links, ROM verification and memory guards passed using the verified
109-input receipt, walk header and six explicit sidecars; source and inputs
were unchanged. The two ARM outputs were:

| Profile | ROM SHA-256 | ELF SHA-256 | `__bss_end` | `__iwram_end` |
| --- | --- | --- | --- | --- |
| P7 | `e51adb8cf1d9aacbebff18843863157e73189239c0cde0643e24edc015489cc4` | `0bc9fb4819bf71bed7d4d232c16ad4af3d6c237de45fc8d6a11cba6cc54ce792` | `02038314` | `03004a28` |
| P8 | `b4ca7b9890b1813ebff5fa20b26392fc4a82624315af1dcba153416cac27d908` | `f9bfc99e735a9a08307a3c430ece4890edfda9e69a9bc4baa661678710493391` | `02038cdc` | `03004a28` |

These are local compile and host-fixture results, not a hardware power-loss,
full mission, or game-level atomicity proof. Mission rebinds and other
post-preflight failures remain separate loader work.
