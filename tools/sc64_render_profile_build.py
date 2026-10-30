# SPDX-License-Identifier: GPL-3.0-or-later
import pathlib,subprocess,json,sys
# Arguments: original JSON argv, private output directory, private tape directory.
root=pathlib.Path.cwd(); args_path=pathlib.Path(sys.argv[1]).resolve(); out=pathlib.Path(sys.argv[2]).resolve(); tape=pathlib.Path(sys.argv[3]).resolve(); out.mkdir(parents=True, exist_ok=False)
argv=json.loads(args_path.read_text()); argv=argv[:1]+['-DGBA_SHAPES_ROM_ARM','-DSC64_DIAGNOSTIC_TRACER_PROFILE']+argv[1:]; units=[a for a in argv[1:] if a.endswith(('.c','.s','.S'))];flags=[];skip=False
for a in argv[1:]:
 if skip:skip=False;continue
 if a=='-o':skip=True;continue
 if a in units or a=='-nostdlib' or a.startswith(('-Wl,','-T','-l')):continue
 flags.append(a)
objs={}; source_root=root
for unit in units:
 if unit=='common/main.c':continue
 obj=out/(unit.replace('/','_')+'.o');subprocess.run([argv[0],*flags,'-c',str(source_root/unit),'-o',str(obj)],cwd=root,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);objs[unit]=obj
 print('built',unit,flush=True)
game=objs['starcraft/game.c']; state=('u','b','visible','explored','minerals','gas','frame','screen','won','lost');subprocess.run(['arm-none-eabi-objcopy',*[f'--globalize-symbol={s}' for s in state],str(game)],check=True)
nm=subprocess.check_output(['arm-none-eabi-nm','-S',str(game)],text=True);sizes={p[3]:int(p[1],16) for l in nm.splitlines() if len(p:=l.split())==4 and p[3] in ('u','b','visible')};assert len(sizes)==3
probe=out/'profile.o';subprocess.run([argv[0],*flags,'-I'+str(tape),f'-DPROBE_U_BYTES={sizes["u"]}',f'-DPROBE_B_BYTES={sizes["b"]}',f'-DPROBE_GRID_BYTES={sizes["visible"]}','-c',str(root/'tools/sc64_render_profile_probe.c'),'-o',str(probe)],cwd=root,check=True)
link=[];skip=False
for a in argv:
 if skip:skip=False;continue
 if a=='-o':link+=['-o',str(out/'profile.elf')];skip=True
 elif a=='common/main.c':link.append(str(probe))
 elif a in objs:link.append(str(objs[a]))
 elif a.startswith('-Wl,-Map,'):link.append('-Wl,-Map,'+str(out/'profile.map'))
 else:link.append(a)
(out/'link.json').write_text(json.dumps(link,indent=2));(out/'compile-flags.json').write_text(json.dumps(flags,indent=2));subprocess.run(link,cwd=root,check=True);subprocess.run(['arm-none-eabi-objcopy','-O','binary',str(out/'profile.elf'),str(out/'profile.gba')],check=True);subprocess.run(['python3','tools/fix_rom.py',str(out/'profile.gba'),'SC PROFILE','SCPF'],cwd=root,check=True);print('done',flush=True)
