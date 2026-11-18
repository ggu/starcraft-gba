#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Measure the pinned Z10 route-grid inner loops in small synthetic GBA ROMs.

Copies only the emitted loop and authored lookup table from each verified
compiler output. It does not boot either game, change guest memory via the
observer, or estimate the complete game loop.
"""

import argparse
from pathlib import Path
import fcntl
import hashlib
import json
import os
import resource
import select
import shutil
import subprocess

from sc64_headless_replay import HeadlessReplay


def require(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def sha(path):
    with path.open("rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def extract(rom, addr, size):
    with rom.open("rb") as f:
        f.seek(addr - 0x8000000)
        b = f.read(size)
    require(len(b) == size, "truncated compiled loop or table")
    return b


def run(args, root):
    return subprocess.run(args, cwd=root, check=True, capture_output=True, timeout=15)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("stock", "candidate", "runner", "out"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    src = Path(__file__).resolve().parents[1]
    stock, opt, runner = (
        args.stock.resolve(),
        args.candidate.resolve(),
        args.runner.resolve(),
    )
    paths = {
        stock: "2b907cdecb5769dde471de04b280ffe7de79a3b43874b2eaabfc2a9b8be43eb8",
        opt: "f0d4ae5395cd4ed9fb1eeb6200891a7973f4e53196103e187b265848fe29450f",
        runner: "4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549",
    }
    if len(paths) != 3 or any(sha(path) != pin for path, pin in paths.items()):
        raise ValueError("grid benchmark requires the documented ROM pair and observer")
    if shutil.disk_usage(args.out.resolve().parent).free < 3.5 * 2**30:
        raise ValueError(
            "grid benchmark requires 3.5 GiB free; no game ROM is launched"
        )
    resource.setrlimit(resource.RLIMIT_FSIZE, (256 * 1024,) * 2)
    loops = [extract(stock, 0x08008A64, 28), extract(opt, 0x08008A36, 16)]
    tables = [extract(stock, 0x093E7DF4, 64), extract(opt, 0x093E7D8C, 2048)]

    def expanded_byte(value):
        return bytes(7 if value & (1 << bit) else 15 for bit in range(8))

    require(
        tables[0] == b"".join(expanded_byte(i)[:4] for i in range(16)),
        "stock lookup bytes differ",
    )
    require(
        tables[1] == b"".join(expanded_byte(i) for i in range(256)),
        "candidate lookup bytes differ",
    )
    out = args.out.resolve()
    out.mkdir()
    for n, x in zip(
        ("stock-loop", "lut8-loop", "stock-table", "lut8-table"), loops + tables
    ):
        (out / n).write_bytes(x)
    results = []
    with Path("/tmp/sc64-native-build-20261031.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        for label, n, reverse in [
            ("wide-open", 2048, False),
            ("wide-mixed", 2048, False),
            ("wide-mixed", 2048, True),
            ("mid-mixed", 1536, False),
        ]:
            shape = "-rev" if reverse else ""
            root = out / (label + shape)
            root.mkdir()
            for f in ("stock-loop", "lut8-loop", "stock-table", "lut8-table"):
                (root / f).symlink_to(out / f)
            values = (
                bytes([255]) * n
                if label.endswith("open")
                else bytes((29 * i + i // 256 * 13) % 256 for i in range(n))
            )
            (root / "pattern").write_bytes(values)
            chunks = {
                "stock": f"""ldr r0, =stock_output
ldr r1, =stock_output+{8 * n}
ldr r4, =stock_table
ldr r7, =cells
.global stock_begin
stock_begin:
.incbin "stock-loop"
.global stock_end
stock_end:
nop
""",
                "lut8": f"""ldr r1, =lut8_output
ldr r3, =cells
ldr r4, =cells+{n}
ldr r5, =lut8_table
.global lut8_begin
lut8_begin:
.incbin "lut8-loop"
.global lut8_end
lut8_end:
nop
""",
            }
            order = ["lut8", "stock"] if reverse else ["stock", "lut8"]
            asm = (
                f""".syntax unified
.cpu arm7tdmi
.thumb
.section .text.bench,"ax",%progbits
.thumb_func
.global main
main:
ldr r0, =0x04000204
ldr r1, =0x4317
strh r1, [r0]
ldr r0, =pattern
ldr r1, =cells
ldr r2, =cells+{n}
1:
ldmia r0!, {{r3}}
stmia r1!, {{r3}}
cmp r1, r2
bne 1b
"""
                + "".join(chunks[k] for k in order)
                + f"""2:
b 2b
.pool
.balign 4
stock_table:
.incbin "stock-table"
.balign 4
lut8_table:
.incbin "lut8-table"
.balign 4
pattern:
.incbin "pattern"
.section .bss.bench,"aw",%nobits
.balign 4
cells: .space {n}
.global stock_output
stock_output: .space {8 * n}
.global lut8_output
lut8_output: .space {8 * n}
"""
            )
            (root / "bench.s").write_text(asm)
            run(
                [
                    "arm-none-eabi-gcc",
                    "-nostdlib",
                    "-mthumb-interwork",
                    "-mcpu=arm7tdmi",
                    f"-T{src}/common/gba.ld",
                    str(src / "common/start.s"),
                    "bench.s",
                    "-o",
                    "bench.elf",
                ],
                root,
            )
            run(
                ["arm-none-eabi-objcopy", "-O", "binary", "bench.elf", "bench.gba"],
                root,
            )
            run(
                [
                    "python3",
                    str(src / "tools/fix_rom.py"),
                    "bench.gba",
                    "GRID BENCH",
                    "SCGB",
                ],
                root,
            )
            rom = root / "bench.gba"
            require(rom.stat().st_size <= 65536, "benchmark ROM exceeds 64 KiB")
            syms = {
                name: int(addr, 16)
                for addr, _, name in (
                    ln.split()
                    for ln in run(["arm-none-eabi-nm", "-n", "bench.elf"], root)
                    .stdout.decode()
                    .splitlines()
                    if len(ln.split()) == 3
                )
            }
            with (root / "observer.log").open("w") as log:
                (root / "home").mkdir()
                env = {
                    **os.environ,
                    "HOME": str(root / "home"),
                    "XDG_CONFIG_HOME": str(root / "home"),
                }
                proc = subprocess.Popen(
                    [str(runner), str(rom), "--physical"],
                    cwd=root,
                    env=env,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=log,
                    text=True,
                )
                try:
                    if (
                        not select.select([proc.stdout], [], [], 5)[0]
                        or proc.stdout.readline().strip() != "READY"
                    ):
                        raise RuntimeError("observer did not start")
                    rsp = HeadlessReplay(proc)
                    clocks = {}
                    for v in order:
                        for marker in ("begin", "end"):
                            address = syms[f"{v}_{marker}"]
                            rsp.breakpoint(address, True)
                            rsp.tick(expected=address)
                            clocks[f"{v}_{marker}"] = rsp.clock()
                            rsp.breakpoint(address, False)
                    expected = b"".join(expanded_byte(i) for i in values)
                    for v in order:
                        require(
                            rsp.read(syms[f"{v}_output"], len(expected)) == expected,
                            f"{v} output differs",
                        )
                    proc.stdin.close()
                    require(proc.wait(timeout=5) == 0, "observer did not exit cleanly")
                finally:
                    if proc.poll() is None:
                        proc.kill()
                        proc.wait(timeout=5)
            result = {v: clocks[f"{v}_end"] - clocks[f"{v}_begin"] for v in order}
            results.append(
                {
                    "pattern": label + shape,
                    "bytes": n,
                    "cycles": result,
                    "data_sha256": hashlib.sha256(values).hexdigest(),
                    "tiny_rom_sha256": sha(rom),
                }
            )
            print(results[-1], flush=True)
    if any(sha(path) != pin for path, pin in paths.items()):
        raise RuntimeError("benchmark inputs changed during the run")
    receipt = {
        "inputs": {str(k): v for k, v in paths.items()},
        "loops_sha256": [hashlib.sha256(v).hexdigest() for v in loops],
        "tables_sha256": [hashlib.sha256(v).hexdigest() for v in tables],
        "results": results,
    }
    (out / "result.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("receipt", sha(out / "result.json"), flush=True)


if __name__ == "__main__":
    main()
