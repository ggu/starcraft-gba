#!/usr/bin/env python3
"""Asset-free tests for tools/sc64_release.py (no ROMs, no make builds)."""

from __future__ import annotations

import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sc64_release as rel  # noqa: E402


def fake_rom(path: Path, code: str, filler: int = 0) -> None:
    data = bytearray(256)
    data[0xA0:0xAC] = b"SC TEST".ljust(12, b"\0")
    data[0xAC:0xB0] = code.encode("ascii")
    data[0xC0:] = bytes([filler]) * (256 - 0xC0)
    path.write_bytes(bytes(data))


class FakeRunner:
    """Records commands; fails any whose text contains a key in ``fail``."""

    def __init__(self, fail: tuple[str, ...] = (), on_run=None) -> None:
        self.fail = fail
        self.calls: list[list[str]] = []
        self.on_run = on_run

    def __call__(self, command, log, cwd) -> int:
        self.calls.append(list(command))
        log.parent.mkdir(parents=True, exist_ok=True)
        text = " ".join(command)
        bad = any(key in text for key in self.fail)
        log.write_text("$ " + text + "\n" + ("src/x.c:1: error: boom\n" if bad else "ok\n"),
                       encoding="utf-8")
        if self.on_run:
            self.on_run(command)
        return 1 if bad else 0


class RegistryTests(unittest.TestCase):
    def test_every_registry_target_exists_in_the_makefile_text(self) -> None:
        text = "\n".join(path.read_text(encoding="utf-8")
                         for path in [ROOT / "Makefile", *ROOT.glob("mk/*/Makefile"),
                                      *ROOT.glob("tests/*/Makefile"), *ROOT.glob("config/*/Makefile")])
        for product in rel.REGISTRY:
            targets = [*product.build_targets, product.verify_target, *product.test_targets]
            for target in targets:
                with self.subTest(product=product.key, target=target):
                    self.assertRegex(text, rf"(?m)^{re.escape(target)}\s*:[^=]",
                                     f"{target} has no rule")

    def test_verify_targets_use_verify_roms_with_a_known_flag(self) -> None:
        verify_text = (ROOT / "tools" / "verify_roms.py").read_text(encoding="utf-8")
        for product in rel.REGISTRY:
            with self.subTest(product=product.key):
                if product.key == "default":
                    continue
                self.assertIn(f'"{product.game_code}"', verify_text)
        self.assertIn('"--showcase"', verify_text)

    def test_product_selection(self) -> None:
        keys = [p.key for p in rel.resolve_products(None)]
        self.assertNotIn("zerg-z10", keys)
        self.assertIn("showcase", keys)
        self.assertIn("zerg-z10", [p.key for p in rel.resolve_products("all")])
        self.assertEqual([p.key for p in rel.resolve_products("zerg,protoss")],
                         ["zerg-z9", "protoss-p10"])
        with self.assertRaises(rel.ReleaseError):
            rel.resolve_products("nonsense")

    def test_make_arg_pairs_accept_a_bare_dash_o(self) -> None:
        args = rel.build_parser().parse_args(
            rel.join_make_args(["check", "--make-arg", "-o", "--make-arg", "x.h"]))
        self.assertEqual(args.make_arg, ["-o", "x.h"])


class ErrorLineTests(unittest.TestCase):
    def test_picks_missing_rule_then_first_failing_test_then_compiler_error(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            log = Path(td) / "l.log"
            log.write_text("usage: x\npython3 -m unittest: error: argument --a\n"
                           "ERROR: test_a (m.C.test_a)\nx.c:1:1: fatal error: 'h.h' file not found\n"
                           "make: *** [t] Error 1\n")
            self.assertEqual(rel.first_error_line(log), "ERROR: test_a (m.C.test_a)")
            log.write_text("cc x.c\nx.c:3:4: error: boom\nmake: *** [t] Error 1\n")
            self.assertEqual(rel.first_error_line(log), "x.c:3:4: error: boom")
            log.write_text("make: *** No rule to make target `a.h', needed by `b'.  Stop.\nERROR: test\n")
            self.assertIn("No rule to make target", rel.first_error_line(log))
            log.write_text("nothing special\nlast words\n")
            self.assertEqual(rel.first_error_line(log), "last words")


class MissingInputTests(unittest.TestCase):
    DATABASE = """\
# Variables
FOO := a:b
# Files
.PHONY: all test
# Not a target:
src.c:
#  Implicit rule search has not been done.

app.elf: src.c .local/in.h | build
\tcc

.local/in.h: .local/source.bin
\tgen

all: app.elf
test: app.elf
# Not a target:
.local/source.bin:
"""

    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.addCleanup(self.tmp.cleanup)
        (self.root / "src.c").write_text("x")
        (self.root / ".local").mkdir()

    def parse(self):
        return rel.parse_make_database(self.DATABASE)

    def test_database_parsing_drops_order_only_and_not_a_target_entries(self) -> None:
        rules, phony = self.parse()
        self.assertEqual(rules["app.elf"], ["src.c", ".local/in.h"])
        self.assertIn("all", phony)
        self.assertNotIn("src.c", rules)

    def test_absent_source_of_a_generated_input_is_reported(self) -> None:
        rules, phony = self.parse()
        missing = rel.missing_from_rules(rules, phony, ["app.elf"], set(), self.root)
        self.assertEqual(missing, [".local/source.bin"])

    def test_old_file_assumption_hides_a_known_missing_input(self) -> None:
        rules, phony = self.parse()
        ignored = rel.parse_ignored(["-o", ".local/in.h", "X=1"])
        self.assertEqual(rel.missing_from_rules(rules, phony, ["app.elf"], ignored, self.root), [])

    def test_present_source_means_nothing_is_missing(self) -> None:
        (self.root / ".local" / "source.bin").write_text("x")
        rules, phony = self.parse()
        self.assertEqual(rel.missing_from_rules(rules, phony, ["test"], set(), self.root), [])


def make_env(root: Path, runner, missing=None, free=10 * rel.GIB, lock=None) -> rel.Env:
    ticks = iter(range(0, 10000, 3))
    return rel.Env(root=root, runner=runner,
                   find_missing=missing or (lambda targets, args: []),
                   free_bytes=lambda: free, clock=lambda: float(next(ticks)),
                   lock_path=lock)


class CheckLogicTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.addCleanup(self.tmp.cleanup)
        (self.root / "build").mkdir()
        self.state = patch_state(self)

    def products(self, *keys):
        return [rel.BY_KEY[k] for k in keys]

    def build_roms(self, *keys):
        for key in keys:
            fake_rom(self.root / rel.BY_KEY[key].rom, rel.BY_KEY[key].game_code)

    def run_check(self, env, products, require_all=False, make_args=()):
        out = self.root / "out"
        return rel.run_check(env, products, list(make_args), out, require_all=require_all)

    def test_all_pass(self) -> None:
        self.build_roms("terran")
        result = self.run_check(make_env(self.root, FakeRunner()), self.products("terran"))
        self.assertEqual(result["overall"], rel.PASS)
        product = result["products"][0]
        self.assertEqual([s["name"] for s in product["steps"]], ["build", "verify", "test"])
        self.assertEqual({s["status"] for s in product["steps"]}, {rel.PASS})
        self.assertEqual(product["rom"]["game_code"], "SCT8")
        self.assertEqual(product["rom"]["sha256"],
                         hashlib.sha256((self.root / rel.BY_KEY["terran"].rom).read_bytes()).hexdigest())
        self.assertTrue((self.root / "out" / "release-check.json").is_file())
        self.assertIn("terran", (self.root / "out" / "release-check.md").read_text())
        self.assertEqual(rel.exit_code(result), 0)

    def test_steps_run_in_order_with_j1_and_extra_arguments(self) -> None:
        self.build_roms("terran")
        runner = FakeRunner()
        self.run_check(make_env(self.root, runner), self.products("terran"),
                       make_args=["-o", "gen.h", "Z8_DESCRIPTOR=d.json"])
        make_calls = [c for c in runner.calls if c[0] == "make"]
        self.assertEqual(len(make_calls), 3)
        for call in make_calls:
            self.assertEqual(call[:2], ["make", "-j1"])
            self.assertEqual(call[2:5], ["-o", "gen.h", "Z8_DESCRIPTOR=d.json"])
        self.assertIn("build/starcraft-terran.elf", make_calls[0])
        self.assertEqual(make_calls[1][-1], "verify-terran")
        self.assertEqual(make_calls[2][-2:], ["test-sc64-terran-full-campaign", "test-terran"])
        self.assertEqual(runner.calls[0][1:], ["tools/check_source_tree.py"])

    def test_missing_inputs_are_skipped_never_passed(self) -> None:
        def missing(targets, args):
            return [".local/a.h", ".local/b.h", ".local/c.h", ".local/d.h"] if "test-zerg" in targets else []
        self.build_roms("zerg-z9")
        result = self.run_check(make_env(self.root, FakeRunner(), missing), self.products("zerg-z9"))
        product = result["products"][0]
        test = product["steps"][2]
        self.assertEqual(test["status"], rel.SKIPPED)
        self.assertIn("missing: .local/a.h, .local/b.h, .local/c.h (+1 more)", test["detail"])
        self.assertEqual(product["status"], rel.SKIPPED)
        self.assertEqual(result["overall"], rel.SKIPPED)
        self.assertEqual(rel.exit_code(result), 0)

    def test_require_all_turns_skipped_into_failure(self) -> None:
        result = self.run_check(make_env(self.root, FakeRunner(), lambda t, a: [".local/x"]),
                                self.products("showcase"), require_all=True)
        self.assertEqual(result["products"][0]["status"], rel.SKIPPED)
        self.assertEqual(result["overall"], rel.FAIL)
        self.assertEqual(rel.exit_code(result), 1)

    def test_build_skip_cascades_to_verify(self) -> None:
        result = self.run_check(make_env(self.root, FakeRunner(), lambda t, a: [".local/x"]),
                                self.products("showcase"))
        steps = result["products"][0]["steps"]
        self.assertEqual([s["status"] for s in steps], [rel.SKIPPED] * 3)
        self.assertIsNone(result["products"][0]["rom"])

    def test_failure_records_first_error_line_and_exit_code(self) -> None:
        self.build_roms("protoss-p10")
        result = self.run_check(make_env(self.root, FakeRunner(fail=("test-protoss",))),
                                self.products("protoss-p10"))
        step = result["products"][0]["steps"][2]
        self.assertEqual(step["status"], rel.FAIL)
        self.assertIn("error: boom", step["detail"])
        self.assertEqual(result["overall"], rel.FAIL)
        self.assertEqual(rel.exit_code(result), 1)

    def test_wrong_game_code_fails_verify(self) -> None:
        fake_rom(self.root / rel.BY_KEY["showcase"].rom, "XXXX")
        result = self.run_check(make_env(self.root, FakeRunner()), self.products("showcase"))
        self.assertEqual(result["products"][0]["steps"][1]["status"], rel.FAIL)

    def test_low_disk_refuses_to_build(self) -> None:
        runner = FakeRunner()
        env = make_env(self.root, runner, free=1 * rel.GIB)
        result = self.run_check(env, self.products("default"))
        self.assertEqual(result["products"][0]["steps"][0]["status"], rel.FAIL)
        self.assertIn("GiB free", result["products"][0]["steps"][0]["detail"])
        self.assertFalse([c for c in runner.calls if c[0] == "make"])

    def test_build_lock_is_held_around_make_only(self) -> None:
        import fcntl
        lock = self.root / "lock"
        seen = []

        def probe(command):
            if command[0] == "make":
                with open(lock, "a") as handle:
                    try:
                        fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
                        seen.append("free")
                    except OSError:
                        seen.append("held")
        self.build_roms("default")
        self.run_check(make_env(self.root, FakeRunner(on_run=probe), lock=str(lock)),
                       self.products("default"))
        self.assertEqual(seen, ["held"] * 3)
        with open(lock, "a") as handle:
            fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)   # released afterwards

    def test_capability_skip_is_reported_as_skipped(self) -> None:
        log_dir = self.root / "logs"

        def runner(command, log, cwd):
            log.parent.mkdir(parents=True, exist_ok=True)
            log.write_text("test_x (m.C.test_x) ... skipped 'no clang'\nOK (skipped=1)\n")
            return 0
        step = rel.run_global_step(make_env(self.root, runner), "engine-capabilities", ["x"], log_dir)
        self.assertEqual(step["status"], rel.SKIPPED)
        self.assertIn("no clang", step["detail"])


def patch_state(case: unittest.TestCase):
    original = rel.source_state
    rel.source_state = lambda root=rel.ROOT: ("a" * 40, False)
    case.addCleanup(setattr, rel, "source_state", original)
    original_versions = rel.tool_versions
    rel.tool_versions = lambda: {"python": "test"}
    case.addCleanup(setattr, rel, "tool_versions", original_versions)
    original_ram = rel.ram_margins
    rel.ram_margins = lambda elf, root=rel.ROOT: {
        "__iwram_end": {"end": 1, "limit": 3, "margin_bytes": 2},
        "__bss_end": {"end": 1, "limit": 5, "margin_bytes": 4}}
    case.addCleanup(setattr, rel, "ram_margins", original_ram)
    return None


class PackageTests(unittest.TestCase):
    REV = "0123456789abcdef0123456789abcdef01234567"

    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.addCleanup(self.tmp.cleanup)
        (self.root / "build").mkdir()
        patch_state(self)
        for key in ("terran", "protoss-p10"):
            fake_rom(self.root / rel.BY_KEY[key].rom, rel.BY_KEY[key].game_code, 1 if key == "terran" else 2)
        self.checklist = self.root / "check.md"
        self.checklist.write_text(
            "# T\n\nintro\n\n## Common\n\n- [ ] boots\n\n## Terran\n<!-- products: terran -->\n\n"
            "- [ ] terran thing\n\n## Protoss\n<!-- products: protoss-p10 -->\n\n- [ ] protoss thing\n",
            encoding="utf-8")
        self.check_dir = self.root / "chk"

        def missing(targets, args):
            return [".local/z"] if "test-zerg" in targets else []
        env = make_env(self.root, FakeRunner(), missing)
        keys = ("terran", "protoss-p10", "zerg-z9")
        for key in ("zerg-z9",):
            fake_rom(self.root / rel.BY_KEY[key].rom, rel.BY_KEY[key].game_code, 3)
        self.result = rel.run_check(env, [rel.BY_KEY[k] for k in keys], [], self.check_dir)
        outside = tempfile.TemporaryDirectory()      # outside the (fake) repository
        self.addCleanup(outside.cleanup)
        self.out = Path(outside.name) / "pkgs"

    def package(self, **kw):
        kw.setdefault("revision", (self.REV, False))
        kw.setdefault("checklist_path", self.checklist)
        # the check was written at the patched revision; align it with HEAD
        data = json.loads((self.check_dir / "release-check.json").read_text())
        data["revision"] = self.REV
        data["dirty"] = kw.pop("check_dirty", False)
        (self.check_dir / "release-check.json").write_text(json.dumps(data))
        return rel.build_package(self.check_dir, self.out, root=self.root, **kw)

    def test_package_contents_manifest_sums_and_readme(self) -> None:
        pkg = self.package()
        self.assertEqual(pkg.name, f"StarCraft-GBA-{self.result['date']}-01234567")
        names = sorted(p.name for p in pkg.iterdir())
        roms = [n for n in names if n.endswith(".gba")]
        self.assertEqual(len(roms), 2)                       # zerg skipped its test step
        self.assertTrue(all("-01234567.gba" in n for n in roms))
        manifest = json.loads((pkg / "MANIFEST.json").read_text())
        self.assertEqual(manifest["revision"], self.REV)
        self.assertFalse(manifest["dirty"])
        self.assertFalse(manifest["hardware_tested"])
        for entry in manifest["roms"]:
            self.assertEqual(hashlib.sha256((pkg / entry["file"]).read_bytes()).hexdigest(),
                             entry["sha256"])
            self.assertEqual(entry["ram"]["__iwram_end"]["margin_bytes"], 2)
        sums = (pkg / "SHA256SUMS").read_text().splitlines()
        self.assertTrue(all(re.fullmatch(r"[0-9a-f]{64}  \S+", line) for line in sums))
        self.assertNotIn("SHA256SUMS", " ".join(sums))
        self.assertEqual(len(sums), 4)                       # 2 ROMs + MANIFEST + README
        readme = (pkg / "README.txt").read_text().splitlines()
        self.assertIn(self.REV, readme[1])
        self.assertIn("SOURCE REVISION", readme[1])
        text = "\n".join(readme)
        self.assertIn("zerg-z9", text)
        self.assertIn("NOT packaged", text)
        self.assertIn("no test on a real Game Boy Advance", text)
        self.assertIn("[ ] boots", text)
        self.assertIn("[ ] terran thing", text)
        self.assertIn("[ ] protoss thing", text)
        shasum = shutil.which("shasum")
        if shasum:
            done = subprocess.run([shasum, "-a", "256", "-c", "SHA256SUMS"], cwd=pkg,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            self.assertEqual(done.returncode, 0, done.stdout)

    def test_product_sections_follow_the_selected_products(self) -> None:
        pkg = self.package(products=[rel.BY_KEY["terran"]])
        text = (pkg / "README.txt").read_text()
        self.assertIn("[ ] terran thing", text)
        self.assertNotIn("protoss thing", text)

    def test_revision_mismatch_is_refused(self) -> None:
        with self.assertRaisesRegex(rel.ReleaseError, "HEAD is"):
            self.package(revision=("f" * 40, False))

    def test_dirty_tree_is_refused_unless_allowed_and_then_labelled(self) -> None:
        with self.assertRaisesRegex(rel.ReleaseError, "modified tracked files"):
            self.package(revision=(self.REV, True))
        with self.assertRaises(rel.ReleaseError):
            self.package(check_dirty=True)
        pkg = self.package(revision=(self.REV, True), allow_dirty=True)
        self.assertTrue(pkg.name.endswith("-dirty"))
        self.assertTrue(all(p.name.endswith("-dirty.gba") for p in pkg.glob("*.gba")))
        self.assertTrue(json.loads((pkg / "MANIFEST.json").read_text())["dirty"])

    def test_unpassed_requested_product_is_refused(self) -> None:
        with self.assertRaisesRegex(rel.ReleaseError, "did not PASS"):
            self.package(products=[rel.BY_KEY["zerg-z9"]])
        pkg = self.package(products=[rel.BY_KEY["zerg-z9"]], accept_skipped=True)
        self.assertIn("accepted explicitly", (pkg / "README.txt").read_text())

    def test_changed_rom_since_check_is_refused(self) -> None:
        (self.root / rel.BY_KEY["terran"].rom).write_bytes(b"changed")
        with self.assertRaisesRegex(rel.ReleaseError, "changed since the check"):
            self.package(products=[rel.BY_KEY["terran"]])

    def test_tracked_or_unignored_output_is_refused(self) -> None:
        for sub in ("docs/pkg", "tools/pkg", "pkg-not-ignored"):
            with self.subTest(out=sub):
                with self.assertRaisesRegex(rel.ReleaseError, "tracked or non-ignored"):
                    rel.path_is_untracked_ignored(ROOT / sub)
        rel.path_is_untracked_ignored(ROOT / "build" / "release" / "x")   # ignored: allowed
        rel.path_is_untracked_ignored(Path(self.tmp.name) / "elsewhere")   # outside: allowed

    def test_existing_package_folder_is_not_replaced(self) -> None:
        self.package()
        with self.assertRaisesRegex(rel.ReleaseError, "already exists"):
            self.package()


class CopyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        base = Path(self.tmp.name)
        self.pkg = base / "StarCraft-GBA-2026-11-10-01234567"
        self.pkg.mkdir()
        (self.pkg / "a.gba").write_bytes(b"A" * 5000)
        (self.pkg / "b.gba").write_bytes(b"B" * 7000)
        (self.pkg / "README.txt").write_text("readme\n")
        (self.pkg / "MANIFEST.json").write_text(json.dumps({"revision": "r" * 40, "dirty": False}))
        sums = "".join(f"{rel.sha256_file(self.pkg / n)}  {n}\n"
                       for n in ("MANIFEST.json", "README.txt", "a.gba", "b.gba"))
        (self.pkg / "SHA256SUMS").write_text(sums)
        self.dest = base / "card"
        self.dest.mkdir()
        self.log = lambda message: None

    def test_copy_and_read_back(self) -> None:
        (self.dest / "OLD.SAV").write_bytes(b"save")
        verified, results = rel.copy_package(self.pkg, self.dest, log=self.log)
        self.assertTrue(verified)
        self.assertEqual({r["result"] for r in results}, {"OK"})
        folder = self.dest / self.pkg.name
        self.assertEqual((folder / "a.gba").read_bytes(), b"A" * 5000)
        report = (folder / "COPY-VERIFIED.txt").read_text()
        self.assertIn("COPY VERIFIED", report)
        self.assertIn("r" * 40, report)
        self.assertEqual((self.dest / "OLD.SAV").read_bytes(), b"save")   # untouched
        ok, text = rel.verify_destination(self.dest, self.pkg)
        self.assertTrue(ok, text)
        self.assertEqual(rel.verify_destination(self.dest)[0], True)

    def test_second_copy_is_idempotent(self) -> None:
        rel.copy_package(self.pkg, self.dest, log=self.log)
        self.assertTrue(rel.copy_package(self.pkg, self.dest, log=self.log)[0])

    def test_corrupted_destination_is_detected(self) -> None:
        def corrupting(src: Path, dst: Path):
            data = Path(src).read_bytes()
            if Path(src).name == "b.gba":
                data = data[:100] + b"\xff" + data[101:]
            Path(dst).write_bytes(data)
        verified, results = rel.copy_package(self.pkg, self.dest, copyfile=corrupting, log=self.log)
        self.assertFalse(verified)
        self.assertEqual([r["file"] for r in results if r["result"] == "MISMATCH"], ["b.gba"])
        folder = self.dest / self.pkg.name
        self.assertTrue((folder / "COPY-FAILED.txt").is_file())
        self.assertFalse((folder / "COPY-VERIFIED.txt").exists())
        ok, text = rel.verify_destination(self.dest, self.pkg)
        self.assertFalse(ok)
        self.assertIn("MISMATCH", text)

    def test_later_corruption_is_found_by_verify(self) -> None:
        rel.copy_package(self.pkg, self.dest, log=self.log)
        victim = self.dest / self.pkg.name / "a.gba"
        victim.write_bytes(b"short")
        self.assertFalse(rel.verify_destination(self.dest, self.pkg)[0])
        self.assertFalse(rel.verify_destination(self.dest)[0])
        victim.unlink()
        self.assertIn("MISSING", rel.verify_destination(self.dest, self.pkg)[1])

    def test_existing_different_file_is_never_overwritten(self) -> None:
        folder = self.dest / self.pkg.name
        folder.mkdir()
        (folder / "a.gba").write_bytes(b"someone else's ROM")
        (folder / "extra.sav").write_bytes(b"save")
        with self.assertRaisesRegex(rel.ReleaseError, "nothing was overwritten"):
            rel.copy_package(self.pkg, self.dest, log=self.log)
        self.assertEqual((folder / "a.gba").read_bytes(), b"someone else's ROM")
        self.assertFalse((folder / "b.gba").exists())       # nothing partially copied
        self.assertEqual((folder / "extra.sav").read_bytes(), b"save")

    def test_damaged_source_package_is_refused(self) -> None:
        (self.pkg / "a.gba").write_bytes(b"damaged")
        with self.assertRaisesRegex(rel.ReleaseError, "damaged"):
            rel.copy_package(self.pkg, self.dest, log=self.log)
        self.assertEqual(list(self.dest.iterdir()), [])

    def test_unsafe_names_in_sums_are_rejected(self) -> None:
        with self.assertRaises(rel.ReleaseError):
            rel.parse_sums("0" * 64 + "  ../escape\n")

    def test_missing_destination_is_refused(self) -> None:
        with self.assertRaisesRegex(rel.ReleaseError, "not a directory"):
            rel.copy_package(self.pkg, self.dest / "nope", log=self.log)


if __name__ == "__main__":
    unittest.main()
