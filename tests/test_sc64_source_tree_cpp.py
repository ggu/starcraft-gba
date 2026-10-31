# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the source boundary against real temporary Git indexes."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

CHECKER = pathlib.Path(__file__).resolve().parents[1] / "tools/check_source_tree.py"


class CppSourceBoundaryTest(unittest.TestCase):
    def check_index(self, name, content):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
            subprocess.run(["git", "-C", str(root), "add", "--", name], check=True)
            return subprocess.run([sys.executable, str(CHECKER)], cwd=root,
                                  capture_output=True, text=True)

    def test_cpp_and_c_source_are_accepted(self):
        for suffix in ("c", "cpp"):
            with self.subTest(suffix=suffix):
                result = self.check_index("tests/probe." + suffix,
                                          b"int main(void) { return 0; }\n")
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_binary_disguised_as_cpp_is_rejected(self):
        result = self.check_index("probe.cpp", b"int main() {}\0payload")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("binary or oversized", result.stderr)

    def test_oversized_cpp_is_rejected(self):
        result = self.check_index("probe.cpp", b"/" * (512 * 1024 + 1))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("binary or oversized", result.stderr)

    def test_cpp_in_generated_directory_is_rejected(self):
        result = self.check_index("generated/probe.cpp", b"int value;\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("local/generated", result.stderr)

    def test_generated_cpp_name_is_rejected(self):
        result = self.check_index("probe_generated.cpp", b"int value;\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("game-derived generated", result.stderr)

    def test_cpp_credential_is_rejected_without_echoing_value(self):
        token = "ghp_" + "A" * 36
        result = self.check_index("probe.cpp", token.encode())
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("possible credential", result.stderr)
        self.assertNotIn(token, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
