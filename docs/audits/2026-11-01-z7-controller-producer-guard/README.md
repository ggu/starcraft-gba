# Z7 controller producer input guard

The host campaign controller now snapshots the selected producer's observable
identity and eligibility before selection and checks it at every input update
through selection and training. It stops emitting input when the game leaves
PLAY or the producer changes. The scoped guard is restored on all exits. A
same-slot replacement with identical observable identity fields is not
recognizable because no generation token is available. This changes controller
behavior only; it does not change the game engine or prove the reason for any
previous controller loss.

At source `2507fa5885f0e78eb8800d2c70eb2bbf88323d8a`, a canonical Z7 host
ASAN/UBSAN build ran 36 controlled selection-transition cases (six transition
types, injection on updates 1, 3 and 7, with and without an initial selected
building) and six additional training-input suppression cases. The fixture
checks the exact number of updates, suppression of subsequent A/R inputs, and
guard cleanup. A separate canonical host build of the actual controller also
compiled. This controlled fixture is not an ordinary-player or campaign WIN
proof. The locked qualification receipt and logs are at
`/tmp/sc64-z7-controller-guard-fixture-output-20261101/`; the wrapper SHA-256
was `7d251131bc26a120c298abebe100d6e4c7860799ec123a4a80050d4c6293b801`.
Full clean checks and review remain required before merge.
