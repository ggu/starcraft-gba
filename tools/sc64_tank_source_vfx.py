#!/usr/bin/env python3
"""Fail-closed importer for authenticated source Tank firing VFX.

Inputs are authenticated N64 BOLT records interpreted with the pinned OpenBW
schemas. The generated table-0 binding is explicitly inferred pending native
N64 xref evidence; raw source index/strength data is always retained.
"""
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import argparse, hashlib, importlib.util, json, struct, sys
ROOT = Path(__file__).resolve().parents[1]
if not (ROOT/'references/sc64-maps/extract_sc64_maps.py').is_file():
    raise RuntimeError(f"repository root missing authenticated BOLT extractor: {ROOT}")
if not __debug__:
    raise RuntimeError('this importer uses assertions for fail-closed source checks; do not run with python -O')
def generate(rom_path, palette_remap_path, palette_json_path, out_dir):
    """Generate source Tank VFX metadata and payload into *out_dir*.

    Inputs are explicit so tests and build tooling can call the same
    fail-closed importer without invoking a subprocess.
    """
    OUT=Path(out_dir)
    OUT.mkdir(parents=True,exist_ok=True)
    ROM=Path(rom_path)
    BOLT = ROOT / 'references/sc64-maps/extract_sc64_maps.py'
    spec=importlib.util.spec_from_file_location('tankbolt',BOLT); mod=importlib.util.module_from_spec(spec); sys.modules[spec.name]=mod; spec.loader.exec_module(mod)
    spec2=importlib.util.spec_from_file_location('tanksprites',ROOT/'tools/sc64_sprites.py'); spr=importlib.util.module_from_spec(spec2);sys.modules[spec2.name]=spr;spec2.loader.exec_module(spr)
    spec3=importlib.util.spec_from_file_location('tankconstruct',ROOT/'tools/sc64_construction_sprites.py'); cons=importlib.util.module_from_spec(spec3);sys.modules[spec3.name]=cons;spec3.loader.exec_module(cons)
    rom_bytes=ROM.read_bytes(); assert hashlib.sha1(rom_bytes).hexdigest()=='a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
    archive=mod.BoltArchive(rom_bytes); entries={e.path:e for e in archive.entries()}
    def get(path):
        assert path in entries,path
        return archive.read(entries[path])
    raw_images=get('00C/000')

    # Resolve both raw actor/turret and actual weapon->flingy->sprite->image chains from the ROM tables.
    def read_table(path, specs):
        data=get(path); offsets={}; off=0
        for name,fmt,count in specs:
            offsets[name]=(off,fmt,count); off+=struct.calcsize('<'+fmt)*count
        assert len(data)>=off,(path,len(data),off)
        def value(name,row):
            base,fmt,count=offsets[name]; assert 0<=row<count,(path,name,row,count)
            return struct.unpack_from('<'+fmt,data,base+row*struct.calcsize('<'+fmt))[0]
        return data,value
    N=228
    unit_specs=[('flingy','B',N),('turret','H',N),('subunit','H',N),('infestation','H',96),('construction','I',N),('direction','B',N),('shielded','B',N),('shield','H',N),('hp','i',N),('elevation','B',N),('unknown','B',N),('sub_label','B',N),('computer_idle','B',N),('human_idle','B',N),('return_idle','B',N),('attack_unit','B',N),('attack_move','B',N),('ground_weapon','B',N),('ground_hits','B',N),('air_weapon','B',N)]
    weapon_specs=[('label','H',130),('flingy','I',130),('unused','B',130),('target','H',130),('min_range','I',130),('max_range','I',130),('upgrade','B',130),('damage_type','B',130),('bullet_type','B',130),('lifetime','B',130),('hit_type','B',130),('inner','H',130),('medium','H',130),('outer','H',130),('damage','H',130),('bonus','H',130),('cooldown','B',130),('count','B',130),('angle','B',130),('head_offset','b',130),('forward','b',130),('upward','b',130),('error','H',130),('icon','H',130)]
    flingy_specs=[('sprite','H',209),('speed','i',209),('accel','h',209),('halt','i',209),('turn','B',209),('unused','B',209),('movement','B',209)]
    sprite_specs=[('image','H',517),('health','B',517),('unknown','B',517),('visible','B',517)]
    unit_data,U=read_table('00C/001',unit_specs); weapon_data,W=read_table('00C/007',weapon_specs)
    flingy_data,F=read_table('00C/002',flingy_specs); sprite_data,S=read_table('00C/003',sprite_specs)
    def unit_image(raw):
        flingy=U('flingy',raw); sprite=F('sprite',flingy); return S('image',sprite)
    def weapon_image(wid):
        flingy=W('flingy',wid); sprite=F('sprite',flingy); return S('image',sprite)
    assert U('turret',5)==6 and U('turret',30)==31
    assert U('ground_weapon',6)==11 and U('ground_weapon',31)==27
    assert (unit_image(6),unit_image(31))==(251,254)
    assert (weapon_image(11),weapon_image(27))==(531,533)
    assert (W('flingy',11),W('flingy',27))==(143,150)
    assert (F('sprite',143),F('sprite',150))==(346,349)
    verified_chain={'tank':{'hull_raw':5,'turret_raw':6,'weapon_owner_raw':6,'weapon':11,'flingy':143,'sprite':346,'image':531},
                    'siege':{'hull_raw':30,'turret_raw':31,'weapon_owner_raw':31,'weapon':27,'flingy':150,'sprite':349,'image':533}}
    # OpenBW images.dat schema: modifier, shift, iscript columns.
    img_fields=[('grp','I',999),('directional','B',999),('clickable','B',999),('anim','B',999),('always','B',999),('modifier','B',999),('shift','B',999),('script','I',999),('shield','I',999),('attack','I',999),('damage','I',999),('special','I',999),('landing','I',999),('liftoff','I',999)]
    cols={};off=0
    for name,fmt,count in img_fields: cols[name]=(off,fmt,count);off+=struct.calcsize('<'+fmt)*count
    def iv(name,i):
        o,f,n=cols[name]; return struct.unpack_from('<'+f,raw_images,o+i*struct.calcsize('<'+f))[0]
    raw_scripts=get('012/002')
    script_index=cons.script_index(raw_scripts)
    def source_stream(image_id, animation):
        sid=iv('script',image_id); off=script_index.get(sid)
        assert off is not None, (image_id,sid,'missing IScript')
        starts=cons.image_animation_starts(raw_scripts,off)
        pc=starts.get(animation); assert pc is not None,(image_id,animation)
        ops=[]; ticks=0; seen=set()
        for _ in range(128):
            if pc in seen: break
            seen.add(pc); op,args,nxt,jump=cons.decode_instruction(raw_scripts,pc)
            assert op in {0,5,7,8,13,22,24,27,36,37,42,46,47,48,52,54}, (image_id,animation,'unsupported opcode',op,pc)
            ops.append({'op':op,'args':args,'tick':ticks,'pc':pc})
            if op==5 and args: ticks+=args[0]
            if op in (22,54): break
            pc=jump if op==7 and jump is not None else nxt
        else: raise AssertionError((image_id,animation,'instruction bound'))
        return ops
    EXPECTED_PREFIXES={
     (251,'GndAttkInit'):[(5,[1]),(46,[]),(24,[68]),(8,[536,0,0]),(5,[2]),(37,[1])],
     (254,'GndAttkInit'):[(5,[1]),(46,[]),(24,[96]),(13,[537,2,0]),(5,[2]),(37,[1]),(5,[1]),(0,[0])],
     (531,'Init'):[(5,[1]),(36,[1]),(5,[1]),(0,[0]),(27,[])],
     (533,'Init'):[(5,[1]),(36,[1]),(5,[1]),(0,[0]),(24,[7]),(5,[1]),(27,[])],
     (536,'Init'):[(0,[0]),(5,[2]),(22,[])],
     (537,'Init'):[(0,[0]),(5,[1]),(0,[17]),(5,[1]),(0,[34]),(5,[1]),(0,[51]),(5,[1]),(0,[68]),(5,[1]),(0,[85]),(5,[1]),(0,[102]),(5,[1]),(22,[])]}
    source_scripts={}
    for (iid,anim),expected in EXPECTED_PREFIXES.items():
        actual=source_stream(iid,anim); got=[(x['op'],x['args']) for x in actual]
        assert got[:len(expected)]==expected,(iid,anim,got[:len(expected)],expected)
        source_scripts[f'{iid}:{anim}']=actual
    special_flat=iv('special',254)-1
    special_path=f'{0xD+special_flat//256:03X}/{special_flat%256:03X}'
    assert iv('special',254)==261 and special_path=='00E/004',(iv('special',254),special_path)
    EXPECTED={531:('00E/0FE',9,1,241,0,10),533:('00F/000',9,1,246,0,14),536:('00F/003',9,1,250,1,17),537:('00F/004',9,1,251,1,119)}
    frames={}; metadata={}
    for iid,(path,modifier,shift,script_id,directional,count) in EXPECTED.items():
        gfi=iv('grp',iid); flat=gfi-1; actual=f'{0xD+flat//256:03X}/{flat%256:03X}'
        assert actual==path,(iid,actual,path)
        assert (iv('modifier',iid),iv('shift',iid),iv('script',iid),iv('directional',iid))==(modifier,shift,script_id,directional),(iid,'image metadata mismatch')
        data=get(path); grp=spr.parse_grp(data)
        assert len(grp['frames'])==count,(iid,len(grp['frames']),count)
        cooked=[]
        for fi,f in enumerate(grp['frames']):
            w,h=grp['width'],grp['height']; pix=f['pixels']; alpha=f['alpha']
            assert len(pix)==w*h and len(alpha)==w*h
            # Source modifier-9 pixels are strengths; retain all bytes without remap.
            assert all(pix[i] <= 63 for i,a in enumerate(alpha) if a), (iid,fi,'strength>63')
            # The mapped compositor reserves source zero as transparent. Fail if source art violates that.
            assert all(pix[i] != 0 for i,a in enumerate(alpha) if a), (iid,fi,'opaque index 0')
            cropped=bytearray(w*h)
            # Crop to the frame rectangle, preserving source strength and transparent zero.
            for yy in range(f['y'],f['y']+f['frame_height']):
                for xx in range(f['x'],f['x']+f['frame_width']):
                    p=yy*w+xx
                    if alpha[p]: cropped[p]=pix[p]
            cooked.append({'x':f['x'],'y':f['y'],'w':f['frame_width'],'h':f['frame_height'],'canvas_w':w,'canvas_h':h,'pixels':bytes(cropped)})
    
        if iid in (536,537):
            for fr in cooked:
                src=fr['pixels']; w=grp['width']; h=grp['height']
                fr['flip_pixels']=b''.join(src[y*w:(y+1)*w][::-1] for y in range(h))
                fr['flip_x']=w-(fr['x']+fr['w'])
        frames[iid]=cooked
        metadata[str(iid)]={'bolt':path,'sha1':hashlib.sha1(data).hexdigest(),'bytes':len(data),'canvas':[grp['width'],grp['height']],'frame_count':count,'modifier':modifier,'color_shift':shift,'iscript_id':script_id,'directional':bool(directional),'source_pixels':'unmodified GRP palette strengths; 0 transparent'}
    # Exact source table bytes; binding as modifier-9 color_shift1 is inferred by names and size.
    raw_lut=get('00B/006'); assert len(raw_lut)==63*256
    remap=Path(palette_remap_path).read_bytes(); assert len(remap)==256
    palette=json.loads(Path(palette_json_path).read_text())
    gba_rgb=palette['gba_rgb555']; assert len(gba_rgb)==256
    # Project each GBA destination index to the closest original palette member in its remap preimage.
    reverse={d:[] for d in range(256)}
    for src,dst in enumerate(remap): reverse[dst].append(src)
    def rgb_dist(a,b):
        aa=int(gba_rgb[a]); bb=int(palette['source_rgb555'][b])
        return sum((((aa>>shift)&31)-((bb>>shift)&31))**2 for shift in (0,5,10))
    rep=[]
    for d in range(256):
        choices=reverse[d]
        if not choices: choices=range(256)
        rep.append(min(choices,key=lambda s:(rgb_dist(d,s),s)))
    projected=bytearray(63*256)
    for strength in range(1,64):
        rawrow=raw_lut[(strength-1)*256:strength*256]
        for dest in range(256): projected[(strength-1)*256+dest]=remap[rawrow[rep[dest]]]
    (OUT/'raw_modifier9_table0.bin').write_bytes(raw_lut)
    (OUT/'projected_modifier9_lut.bin').write_bytes(projected)
    (OUT/'source_frames.bin').write_bytes(b''.join(f['pixels'] for iid in sorted(frames) for f in frames[iid]))
    # LO 00E/004 is the image-254 (siege turret) one-offset attachment table.
    lo_data=get('00E/004'); lo_frames,lo_count=struct.unpack_from('<II',lo_data)
    assert (lo_frames,lo_count)==(102,1) and len(lo_data)==460
    lo=[]
    for i in range(lo_frames):
        off=struct.unpack_from('<I',lo_data,8+i*4)[0]
        assert off>=416 and off+2<=len(lo_data)
        lo.append(struct.unpack_from('<bb',lo_data,off))
    # Emit a C header with frame descriptor tables and compact frame bytes.
    lines=['/* Generated from authenticated StarCraft64 BOLT GRPs; source data retains its original terms. */','#ifndef SC64_TANK_SOURCE_VFX_GENERATED_H','#define SC64_TANK_SOURCE_VFX_GENERATED_H','#define SC64_TANK_SOURCE_VFX_VERSION 1','#include <stdint.h>','typedef struct { uint32_t offset,flip_offset; uint8_t x,y,w,h,flip_x,cw,ch; } Sc64TankSourceFrame;']
    all_bytes=bytearray(); frame_tables={}
    for iid in sorted(frames):
        arr=[]
        for fr in frames[iid]:
            off=len(all_bytes); all_bytes+=fr['pixels']; flip_off=off
            if 'flip_pixels' in fr:
                flip_off=len(all_bytes); all_bytes+=fr['flip_pixels']
            arr.append((off,flip_off,fr['x'],fr['y'],fr['w'],fr['h'],fr['flip_x'] if 'flip_pixels' in fr else fr['x'],fr['canvas_w'],fr['canvas_h']))
        frame_tables[iid]=arr
        lines.append(f'static const Sc64TankSourceFrame sc64_tank_frames_{iid}[{len(arr)}]={{')
        lines.extend(' {%du,%du,%du,%du,%du,%du,%du,%du,%du},'%x for x in arr); lines.append('};')
    lines.append('static const uint8_t sc64_tank_raw_lut[63*256]={')
    for i in range(0,len(raw_lut),24): lines.append(' '+','.join(str(x) for x in raw_lut[i:i+24])+',')
    lines.append('};')
    lines.append('static const uint8_t sc64_tank_palette_remap[256]={')
    for i in range(0,len(remap),24): lines.append(' '+','.join(str(x) for x in remap[i:i+24])+',')
    lines.append('};')
    lines.append('static const uint8_t sc64_tank_reverse_palette[256]={')
    for i in range(0,len(rep),24): lines.append(' '+','.join(str(x) for x in rep[i:i+24])+',')
    lines.append('};')
    lines.append('static const int8_t sc64_tank_siege_lo[102][2]={')
    lines.extend(' {%d,%d},'%tuple(x) for x in lo); lines.append('};')
    lines.append(f'static const uint8_t sc64_tank_source_pixels[{len(all_bytes)}]={{')
    for i in range(0,len(all_bytes),24): lines.append(' '+','.join(str(x) for x in all_bytes[i:i+24])+',')
    lines.append('};')
    lines.append('static const uint8_t sc64_tank_projected_lut[63*256]={')
    for i in range(0,len(projected),24): lines.append(' '+','.join(str(x) for x in projected[i:i+24])+',')
    lines += ['};','#endif']
    (OUT/'sc64_tank_source_vfx_generated.h').write_text('\n'.join(lines)+'\n')
    metadata.update({
        'rom_sha1': hashlib.sha1(rom_bytes).hexdigest(),
        'verified_weapon_chains': verified_chain,
        'source_hashes': {
            'scripts_012_002': hashlib.sha256(raw_scripts).hexdigest(),
            'units_00C_001': hashlib.sha256(get('00C/001')).hexdigest(),
            'weapons_00C_007': hashlib.sha256(get('00C/007')).hexdigest(),
            'flingy_00C_002': hashlib.sha256(get('00C/002')).hexdigest(),
            'sprites_00C_003': hashlib.sha256(get('00C/003')).hexdigest(),
            'images_00C_000': hashlib.sha256(raw_images).hexdigest(),
            'game.c': hashlib.sha256((ROOT/'starcraft/game.c').read_bytes()).hexdigest(),
            'gba.c': hashlib.sha256((ROOT/'common/gba.c').read_bytes()).hexdigest(),
            'gba.h': hashlib.sha256((ROOT/'common/gba.h').read_bytes()).hexdigest(),
            '00B/006': hashlib.sha256(raw_lut).hexdigest()},
        'source_lookup': {'path':'00B/006','sha256':hashlib.sha256(raw_lut).hexdigest(),
            'length':len(raw_lut),'binding':'INFERRED: modifier 9 / color_shift 1 -> light table index 0; no native N64 xref proof'},
        'projection': {'rule':'choose nearest source RGB555 palette entry in each GBA remap preimage, apply raw source LUT, then remap forward',
            'unique_gba_palette_indices':len(set(remap)), 'lut_sha256':hashlib.sha256(projected).hexdigest(),
            'raw_lut_sha256':hashlib.sha256(raw_lut).hexdigest()},
        'siege_turret_lo': {'bolt':'00E/004','frame_count':lo_frames,'offset_count':lo_count,
            'sha256':hashlib.sha256(lo_data).hexdigest(),
            'selected_per_script':'images.dat image254 special_gfi=261 resolves to 00E/004; GndAttk plays sound96 then imgoluselo(image537, LO index2, offset0), with no frame reset. The GBA parent renderer currently draws directional frame 0..16 (no animation bank), so prototype uses LO[direction_frame] to match its visible pose.'},
        'generated_header_sha256':hashlib.sha256((OUT/'sc64_tank_source_vfx_generated.h').read_bytes()).hexdigest(),
        'header_bytes':len(all_bytes),'descriptors':frame_tables,
        'pinned_openbw_revision':'4b046d5f65302b10cb0a745f0fecd37ec85b20a8',
        'authenticated_script_streams':source_scripts,
        'domissiledmg_events':{str(iid):[x['tick'] for x in source_scripts[f'{iid}:Init'] if x['op']==27] for iid in (531,533)},
        'visual_timeline': {
            'tank_mode':[
                {'gba_age_ticks':[0,0],'source_age_ticks':'0 at shot; source Init has already consumed 2 pre-windup ticks','layer':'directional muzzle','image':536,'frame':'direction_index','origin':'tank center'},
                {'gba_age_ticks':[3,27],'source_age_ticks':'(gba_age*4)/5','layer':'target impact','image':531,'frame':'min((source_age-2)/2,9)','origin':'target center','domissiledmg_source_tick':2,'domissiledmg_gba_age':3}],
            'siege_mode':[
                {'gba_age_ticks':[0,6],'source_age_ticks':'(gba_age*4)/5','layer':'directional muzzle/recoil','image':537,'frame':'(2+source_age)*17+direction_frame','origin':'tank center + image254 LO[direction_frame] offset (matches current GBA parent renderer frame 0..16)','source':'overlay attaches at source script tick1; fire at tick3'},
                {'gba_age_ticks':[3,19],'source_age_ticks':'(gba_age*4)/5','layer':'target impact','image':533,'frame':'source_age-2','origin':'target center','domissiledmg_source_tick':3,'domissiledmg_gba_age':4}]},
        'clock_conversion':{'source_rate_hz':24,'GBA_update_rate_hz':30,'source_age':'(gba_age*4)/5','gba_age_at_source_tick':'ceil(source_tick*5/4)'},
        'timing': {
            'tank_muzzle_536':'Init playframe0, wait2; parent attaches at tick1; attackwith at tick3. The two pre-windup muzzle poses are omitted because existing tracer state starts at attackwith; show frame0 on first post-fire GBA draw only.',
            'siege_muzzle_537':'Init frames 0,17,34,51,68,85,102 with one-tick waits; parent attaches at tick1 and attackwith is tick3.',
            'tank_impact_531':'Init wait1, setspawnframe1, wait1, playframe0; domissiledmg at source local tick2; frames advance every2 ticks.',
            'siege_impact_533':'Init wait1, setspawnframe1, wait1, playframe0, wait1; domissiledmg at source local tick3; frames advance every source tick; clock maps source 3 to GBA age4.',
            'event_scope':'VFX generation schedules no damage. Engine combat resolves at firing; optional Siege splash selects collateral separately. Source domissiledmg ticks describe visual timing, not queued gameplay events.'}})
    (OUT/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(json.dumps({'frames':{k:len(v) for k,v in frames.items()},'header_bytes':len(all_bytes),'lut_sha256':hashlib.sha256(projected).hexdigest(),'header_sha256':metadata['generated_header_sha256']},indent=2))
    return metadata

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom',type=Path,default=ROOT/'.local/sc64/rom/StarCraft64-USA.z64')
    parser.add_argument('--palette-remap',type=Path,default=ROOT/'.local/sc64/backwater/generated/palette-remap.bin')
    parser.add_argument('--palette-json',type=Path,default=ROOT/'.local/sc64/backwater/generated/palette.json')
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args(argv)
    generate(args.rom,args.palette_remap,args.palette_json,args.out)

if __name__ == '__main__':
    main()
