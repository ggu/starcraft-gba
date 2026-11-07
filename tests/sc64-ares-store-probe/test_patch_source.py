# SPDX-License-Identifier: GPL-3.0-or-later
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[2]
class SourcePatchTest(unittest.TestCase):
 def test_hook_only_after_cpu_write_and_around_decoder(self):
  data=(ROOT/'experiments/ares-store-probe/ares-core-patch.txt').read_text()
  self.assertIn('sc64_store_instruction_begin(ipu.pc, *data, ipu.r[(*data >> 21) & 31].u64)',data)
  self.assertIn('sc64_store_instruction_end()',data)
  self.assertLess(data.index('sc64_store_instruction_begin('),data.index('sc64_store_instruction_end()'))
  self.assertLess(data.index('else busWrite<Size>(paddr, data);'),data.index('sc64_store_committed(access.vaddr,paddr,Size,data)'))
  self.assertNotIn('writeDebug',data)
if __name__=='__main__':unittest.main()
