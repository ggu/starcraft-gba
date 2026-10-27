#!/usr/bin/env python3
"""Write deterministic synthetic signed 8-bit PCM for the codec cross check.

`python3 tests/audio_codec_vectors.py OUTDIR` writes NAME.pcm (raw samples,
lengths not always a multiple of 8) and NAME.pcm8 (the same, zero-padded to a
multiple of 8 as the encoders pad it). No ROM-derived data is involved.
"""
import sys
from pathlib import Path


def lcg(seed: int):
    state = seed
    while True:
        state = (state * 1103515245 + 12345) & 0xFFFFFFFF
        yield state >> 16


def wrap8(value: int) -> int:
    return ((value + 128) & 255) - 128


def vectors() -> dict[str, list[int]]:
    out: dict[str, list[int]] = {}
    out["silence"] = [0] * 4096
    out["dc"] = [-77] * 1001
    out["saw"] = [(i % 256) - 128 for i in range(30000)]
    out["square"] = [127 if (i // 37) & 1 else -128 for i in range(20003)]
    tri = []
    for i in range(30000):
        t = i % 300
        tri.append(wrap8((t if t < 150 else 300 - t) * 255 // 150 - 128))
    out["triangle"] = tri
    rnd = lcg(1)
    out["noise"] = [wrap8(next(rnd) & 255) for _ in range(50000)]
    rnd = lcg(2)
    walk, samples = 0, []
    for _ in range(60000):
        walk = max(-128, min(127, walk + next(rnd) % 9 - 4))
        samples.append(walk)
    out["walk"] = samples
    # Parabolic arcs with quiet noise, jumps and silent gaps: every width,
    # header code and predictor gets exercised.
    rnd = lcg(3)
    samples = []
    for i in range(80000):
        t = i % 640
        if t >= 560:
            samples.append(0)
            continue
        level = (t - 280) * (t - 280) // 700 - 56
        amp = (i // 2048) % 9
        noise = (next(rnd) % (1 << amp)) - (1 << amp) // 2 if amp else 0
        samples.append(wrap8(level + noise + (100 if i % 4999 == 0 else 0)))
    out["arcs"] = samples
    out["short"] = [5, -3, 7]
    out["eight"] = [-128, 127, 0, 1, -1, 64, -64, 3]
    return out


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    outdir = Path(sys.argv[1])
    outdir.mkdir(parents=True, exist_ok=True)
    total = 0
    for name, samples in vectors().items():
        raw = bytes(s & 255 for s in samples)
        (outdir / f"{name}.pcm").write_bytes(raw)
        (outdir / f"{name}.pcm8").write_bytes(raw + bytes(-len(raw) % 8))
        total += len(raw)
    print(f"wrote {len(vectors())} vectors, {total} samples, to {outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
