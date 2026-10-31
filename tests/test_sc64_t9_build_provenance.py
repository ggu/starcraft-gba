"""Asset-free checks for the T9 build-provenance capture helpers."""
from __future__ import annotations
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_t9_build_provenance", ROOT / "tools/sc64_t9_build_provenance.py")
assert SPEC and SPEC.loader
prov = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(prov)


class DependencyTests(unittest.TestCase):
    def test_prerequisites_keep_order_and_unescape(self):
        text = ("build/x.elf: starcraft/game.c common/gba.h \\\n"
                " .local/a\\ b/gen.h starcraft/z.h\n")
        self.assertEqual(prov.dep_prerequisites(text),
                         ["starcraft/game.c", "common/gba.h", ".local/a b/gen.h", "starcraft/z.h"])

    def test_ordered_flags_keep_command_order(self):
        args = ["cc1", "-I", "common", "-DB=1", "-Ia", "-D", "A", "-O2", "-include", "x.h", "-UC"]
        self.assertEqual(prov.ordered_flags(args),
                         ["-I common", "-DB=1", "-Ia", "-D A", "-include x.h", "-UC"])


class WrapperTests(unittest.TestCase):
    def test_sub_wrapper_logs_argv_and_renames_dependency_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            sub = tmp / "sub"
            sub.write_text(prov.SUB)
            sub.chmod(0o755)
            tool = tmp / "cc1"
            tool.write_text('#!/bin/sh\nprintf "%s\\n" "$@" > "$OUT"\n')
            tool.chmod(0o755)
            log = tmp / "log"
            (log / "deps").mkdir(parents=True)
            (log / "00-driver.argv").write_bytes(b"gcc\0")
            env = dict(os.environ, T9_PROV_LOG=str(log), OUT=str(tmp / "ran"))
            args = [str(tool), "-quiet", "-MD", "build/x.d", "-dumpbase", "game.c", "-D", "A B"]
            subprocess.run([str(sub), *args], env=env, check=True)
            ran = (tmp / "ran").read_text().splitlines()
            self.assertEqual(ran, ["-quiet", "-MD", f"{log}/deps/game.c.d", "-dumpbase",
                                   "game.c", "-D", "A B"])
            logged = prov.read_argv(log / "01-cc1-game.c.argv")
            self.assertEqual(logged, [str(tool), *ran])
            # An assembler step on a temporary file is named after the last unit.
            asm = tmp / "as"
            asm.write_text(tool.read_text())
            asm.chmod(0o755)
            subprocess.run([str(sub), str(asm), "-o", str(tmp / "x.o"), str(tmp / "ran")],
                           env=env, check=True)
            self.assertTrue((log / "02-as-game.c.argv").is_file())


if __name__ == "__main__":
    unittest.main()
