"""Product pack freshness checks, without private game assets."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SkirmishPackGateTests(unittest.TestCase):
    def test_products_reject_shadowing_stale_pack_and_track_reselection(self):
        with tempfile.TemporaryDirectory() as tmp:
            first, second, stamps = (Path(tmp) / name for name in ('first', 'second', 'stamps'))
            for directory in (first, second, stamps):
                directory.mkdir()
            a, b = (d / 'sc64_skirmish_generated.h' for d in (first, second))
            a.write_text('/* old 32-patch pack */\n')
            b.write_text('#define SC64_SKIRMISH_PACK_REVISION 2\nstatic int data = 56;\n')
            for product in ('terran', 'zerg', 'protoss', 'showcase'):
                stamp = stamps / f'skirmish-pack-{product}.stamp'
                def run(*order):
                    return subprocess.run(['make', '-s', str(stamp), f'SKIRMISH_PACK_STAMP_DIR={stamps}',
                                           'CFLAGS=' + ' '.join(f'-I{p}' for p in order)],
                                          cwd=ROOT, capture_output=True, text=True, check=False)
                with self.subTest(product=product):
                    bad = run(first, second)
                    self.assertNotEqual(bad.returncode, 0)
                    self.assertIn('Stale skirmish pack', bad.stderr)
                    self.assertFalse(stamp.exists())
                    good = run(second, first)
                    self.assertEqual(good.returncode, 0, good.stderr)
                    original, timestamp = stamp.read_text(), stamp.stat().st_mtime_ns
                    self.assertEqual(run(second, first).returncode, 0)
                    self.assertEqual(stamp.stat().st_mtime_ns, timestamp)
                    a.write_text('#define SC64_SKIRMISH_PACK_REVISION 2\nstatic int data = 99;\n')
                    os.utime(a, (1, 1))
                    changed = run(first, second)
                    self.assertEqual(changed.returncode, 0, changed.stderr)
                    self.assertNotEqual(stamp.read_text(), original)
                    a.write_text('/* old 32-patch pack */\n')
                    self.assertNotEqual(run(first, second).returncode, 0)


if __name__ == '__main__':
    unittest.main()
