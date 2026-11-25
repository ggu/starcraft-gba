# SPDX-License-Identifier: GPL-3.0-or-later
"""The audio ARM probe driver fails closed on every part of the ROM's result."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_audio_arm_probe as probe

GOOD = dict(status=1, streams=534, linear=534, samples=16111336, sum_low=0xc18b8cd4, sum_high=0xbb236693,
            decoder_bad=0, mixer_bad=0)
HOST = (0xc18b8cd4, 0xbb236693)


class AudioArmProbeTest(unittest.TestCase):
    def test_host_sum(self):
        text = ('audio bank: linear-predictor sample sum c18b8cd4 bb236693\n'
                'audio bank: 521 SFX + 13 music streams, 16111336 samples from 8644816 bytes; pcm_sha256 4d40\n')
        self.assertEqual(probe.host_sum(text), HOST)
        for bad in ('', 'audio bank: linear-predictor sample sum c18b8cd4\n',
                    'audio bank: linear-predictor sample sum C18B8CD4 BB236693\n',
                    'x audio bank: linear-predictor sample sum c18b8cd4 bb236693\n'):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                probe.host_sum(bad)

    def test_verdict(self):
        self.assertEqual(probe.verdict(GOOD, HOST), [])
        for change in (dict(status=0), dict(status=2), dict(streams=0, linear=0), dict(linear=533),
                       dict(decoder_bad=1), dict(mixer_bad=3000), dict(sum_low=0), dict(sum_high=1)):
            with self.subTest(change=change):
                self.assertTrue(probe.verdict({**GOOD, **change}, HOST))
        self.assertTrue(probe.verdict(GOOD, (HOST[1], HOST[0])))


if __name__ == '__main__':
    unittest.main()
