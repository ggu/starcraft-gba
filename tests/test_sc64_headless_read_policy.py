# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the backend's actual read classifier without linking mGBA."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class PhysicalReadPolicyTest(unittest.TestCase):
    def test_gba_regions_boundaries_and_mmio_rejection(self):
        compiler = shutil.which("clang") or shutil.which("cc")
        if not compiler:
            self.skipTest("C compiler unavailable")
        source = (ROOT / "tools/sc64_headless_replay.c").read_text()
        start = source.index("static int in_readable_memory(")
        end = source.index("\n}", start) + 2
        # Compile the actual classifier, not a second implementation of it.
        classifier = source[start:end]
        cases = [
            (0x02000000, 1, 0, 1), (0x0203ffff, 1, 0, 1),
            (0x0203ffff, 2, 0, 0), (0x02040000, 1, 0, 0),
            (0x03000000, 1, 0, 1), (0x03007fff, 1, 0, 1),
            (0x03007fff, 2, 0, 0), (0x03008000, 1, 0, 0),
            (0x05000000, 1, 0, 1), (0x050003ff, 1, 0, 1),
            (0x050003ff, 2, 0, 0), (0x05000400, 1, 0, 0),
            (0x06000000, 1, 0, 1), (0x06017fff, 1, 0, 1),
            (0x06017fff, 2, 0, 0), (0x06018000, 1, 0, 0),
            (0x06013fe0, 4, 0, 1),  # Modal count/page, not MMIO.
            (0x04000000, 1, 0, 0), (0x04000100, 2, 0, 0),
            (0x040003ff, 1, 0, 0), (0x0e000000, 1, 0, 0),
            (0x06000000, 0, 0, 0), (0xffffffff, 2, 0, 0),
            (0x05000000, 0xffffffff, 0, 0),
            (0x08000000, 1, 0, 0), (0x08000000, 1, 1, 1),
            (0x08000000, 2, 1, 0), (0x09ffffff, 1, 0x2000000, 1),
            (0x09ffffff, 2, 0x2000000, 0), (0x0a000000, 1, 0x2000000, 0),
        ]
        assertions = "\n".join(
            f"assert(!!in_readable_memory({address}u,{length}u,{rom_size}u)=={allowed});"
            for address, length, rom_size, allowed in cases)
        program = "#include <stdint.h>\n#include <stddef.h>\n#include <assert.h>\n" + classifier + "\nint main(void){\n" + assertions + "\nreturn 0;}\n"
        with tempfile.TemporaryDirectory(prefix="sc64-read-policy-") as directory:
            c = pathlib.Path(directory) / "policy.c"
            executable = pathlib.Path(directory) / "policy"
            c.write_text(program)
            subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=address,undefined", str(c), "-o", str(executable)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)
