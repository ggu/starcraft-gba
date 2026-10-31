# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare the actual animation helper against its previous forward scan."""
import pathlib, subprocess, tempfile, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class ReverseScanTest(unittest.TestCase):
    def test_forward_reference(self):
        src=(ROOT/'tests/sc64_weapon_source_resolved_trace.c').read_text()
        src=src.replace('int main(void)', 'int original_fixture_main(void)')
        src=src.replace('"../starcraft/', '"'+str(ROOT/'starcraft')+'/')
        src += r'''
#include <limits.h>
static uint16_t old(const Sc64WeaponSourceAnimation *a,uint32_t tick,uint16_t fallback) {
 if(!a)return fallback;
 if(a->event_first>SC64_WEAPON_SOURCE_EVENT_COUNT || a->event_count>SC64_WEAPON_SOURCE_EVENT_COUNT-a->event_first)return fallback;
 uint32_t bounded=tick;
 if(a->loop_pc!=UINT32_MAX && a->loop_source_tick<a->duration_ticks && bounded>=a->loop_source_tick) {
  uint32_t length=a->duration_ticks-a->loop_source_tick;
  if(length)bounded=a->loop_source_tick+(bounded-a->loop_source_tick)%length;
 }
 if(a->duration_ticks && bounded>=a->duration_ticks)bounded=a->duration_ticks-1u;
 for(uint32_t i=0;i<a->event_count;i++) {const Sc64WeaponSourceEvent *e=&sc64_weapon_source_events[a->event_first+i];if(e->opcode==0 && e->source_tick<=bounded)fallback=e->source_frame;}
 return fallback;
}
static uint32_t state=123;static uint32_t rnd(void){state=state*1664525u+1013904223u;return state;}
int main(void){
 assert(sc64_weapon_source_vfx_animation_frame(0,UINT32_MAX,123)==123);
 for(unsigned n=0;n<400000;n++) {
  Sc64WeaponSourceAnimation a={0};a.event_first=rnd()%5;a.event_count=rnd()%5;
  a.duration_ticks=(n%5==0)?UINT32_MAX:rnd()%32;a.loop_pc=(n&1)?UINT32_MAX:rnd();a.loop_source_tick=(n%7==0)?UINT32_MAX:rnd()%40;
  for(int i=0;i<2;i++){sc64_weapon_source_events[i].source_tick=(n%11==0)?UINT32_MAX:rnd()%40;sc64_weapon_source_events[i].opcode=rnd()%3;sc64_weapon_source_events[i].source_frame=(uint16_t)rnd();}
  if(n%3==0)sc64_weapon_source_events[1].source_tick=sc64_weapon_source_events[0].source_tick;
  uint32_t tick=n%13==0?UINT32_MAX:rnd();uint16_t f=(uint16_t)rnd();
  assert(old(&a,tick,f)==sc64_weapon_source_vfx_animation_frame(&a,tick,f));
 }
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            c=pathlib.Path(d)/'test.c'; exe=pathlib.Path(d)/'test';c.write_text(src)
            subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
if __name__=='__main__':unittest.main()
