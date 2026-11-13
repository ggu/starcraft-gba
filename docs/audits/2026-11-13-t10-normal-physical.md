# T10 from HOME on the normal Terran cartridge

The retained T10 host tape starts after deployment through the private
controller probe entry. Its 278 gameplay transitions replay unchanged
from a fresh HOME boot of the normal KODAK Terran cartridge (`cbba1c5`).
The port-free observer sends physical buttons and reads guest state. It
selects The Hammer Falls in the ordinary campaign menu and reads all 14
briefing pages before deploying on call 63. Four in-game dialogue pages
are acknowledged with press and release while the simulation frame stays
unchanged.

All six host observations match across 18 fields, including launch,
source opening, three progress checkpoints and the result. The cartridge
wins on physical call 3,517 (gameplay call 3,446), frame 3,361. The Ion
Cannon is destroyed, Raynor survives, the objective and mission completion
bits are set, and there are no trigger faults. Mean loop cost is 954,077
guest cycles (17.58 emulated updates per second); the peak is 25,790,057
cycles. This is one qualified route from HOME and does not measure hardware
speed or fresh SRAM continuation. No cartridge was rebuilt.

SHA-256 pins:

| Input or receipt | SHA-256 |
| --- | --- |
| Retained host tape | `a6c04ab0297bf7e0dea2182b895e4d45746d836f97ff8ef9032bcfe87d52a00e` |
| KODAK Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Replay harness | `5b95653d774e61fbc1a75265f266f13a9d12b83f578ad92e5902637c084b5aed` |
| Physical receipt | `ec2528877241c7726a98ffeba1b26f3b6f37c648319f8a3eb564b756fe1d0704` |

A separate T9 (New Gettysburg) check on the same ROM reached tape call
4,000 from fresh HOME. Nine briefing pages and four gameplay pages used
physical acknowledgments. All six checkpoints matched 18 guest fields
through physical call 4,049, game frame 3,946. The controller-local
`failed_action` field is not read from the cartridge. This is an opening
only; the full replay was not started after the disk guard was reached.
Receipt SHA-256: `bc49b057ca97b7902546c732fd290790a0db617190a00a0b86df83c4878e4867`.
