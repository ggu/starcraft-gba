#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Export Yamato source frames to an ignored, locally generated header."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import sc64_weapon_source_sprites as source
import sc64_weapon_source_audit as audit

ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
IMAGES = ((541, 68, 0), (542, 68, 0), (543, 11, 9), (544, 14, 9))
COUNT = 999

def collect(get, sprites, remap, palette):
    if len(remap) != 256:
        raise ValueError('palette remap must contain 256 bytes')
    if len(palette.get('gba_rgb555', [])) != 256:
        raise ValueError('palette JSON must contain 256 GBA colors')
    dat = get('00C/000')
    grps = audit.read_column(dat, COUNT, 'I', 0, 'images.grp')
    modifiers = audit.read_column(dat, COUNT, 'B', COUNT * 8, 'images.modifier')
    shifts = audit.read_column(dat, COUNT, 'B', COUNT * 9, 'images.shift')
    pixels = bytearray(); rows = []; offsets = {}
    for image, expected_count, modifier in IMAGES:
        if modifiers[image] != modifier:
            raise ValueError(f'image {image}: unexpected modifier')
        path = audit.grp_path(grps[image])
        if not path: raise ValueError(f'image {image}: invalid GRP index')
        grp = sprites.parse_grp(get(path))
        if len(grp['frames']) != expected_count:
            raise ValueError(f'image {image}: unexpected frame count')
        for frame, item in enumerate(grp['frames']):
            x,y,w,h = (item['x'],item['y'],item['frame_width'],item['frame_height'])
            cw,ch = grp['width'],grp['height']
            if min(x,y,w,h) < 0 or x+w>cw or y+h>ch or max(x,y,w,h,cw,ch)>255:
                raise ValueError(f'image {image} frame {frame}: invalid bounds')
            indices = sprites.crop_frame(item)
            alpha = b''.join(item['alpha'][(y+r)*item['width']+x:(y+r)*item['width']+x+w] for r in range(h))
            converted = source.convert_pixels(indices,alpha,modifier,remap)
            offset = offsets.get(converted)
            if offset is None:
                offset = len(pixels); offsets[converted] = offset; pixels.extend(converted)
            rows.append((image,frame,modifier,x,y,w,h,cw,ch,offset))
    metadata = {str(image): {'modifier': modifier, 'color_shift': shifts[image]}
                for image, _, modifier in IMAGES}
    tables, lut, _ = source._project_luts(get,palette,metadata)
    if len(tables)!=1 or tables[0]['modifier']!=9 or tables[0]['rows']!=63 or len(lut)!=63*256:
        raise ValueError('unexpected Yamato light tables')
    return bytes(pixels),lut,rows

def render(pixels,lut,rows):
    lines=['/* Generated from authenticated source; local private asset. */', '#ifndef SC64_YAMATO_SOURCE_GENERATED_H', '#define SC64_YAMATO_SOURCE_GENERATED_H', '#include <stdint.h>', 'typedef struct { uint16_t image,frame; uint8_t modifier,x,y,w,h,cw,ch; uint32_t offset; } Sc64YamatoFrame;']
    for name,data in (('sc64_yamato_pixels',pixels),('sc64_yamato_light_lut',lut)):
        lines.append(f'static const uint8_t {name}[] = {{')
        lines.extend('  '+','.join(map(str,data[i:i+24]))+',' for i in range(0,len(data),24));lines.append('};')
    lines.append('static const Sc64YamatoFrame sc64_yamato_frames[] = {')
    lines.extend('  {'+','.join(map(str,row))+'},' for row in rows)
    lines.extend(['};',f'#define SC64_YAMATO_FRAME_COUNT {len(rows)}','#define SC64_YAMATO_LIGHT_ROWS 63','#endif'])
    return '\n'.join(lines)+'\n'

def generate(rom_path,out,remap_path,palette_path):
    rom=rom_path.read_bytes()
    if hashlib.sha1(rom).hexdigest()!=ROM_SHA1: raise ValueError('ROM SHA-1 does not match authenticated StarCraft 64 USA')
    remap=remap_path.read_bytes();palette=json.loads(palette_path.read_text());get,sprites=source._source_bolt(rom)
    pixels,lut,rows=collect(get,sprites,remap,palette)
    text=render(pixels,lut,rows);out.mkdir(parents=True,exist_ok=True);header=out/'yamato_source_generated.h';header.write_text(text,encoding='ascii')
    return {'frames':len(rows),'pixel_bytes':len(pixels),'lut_bytes':len(lut),'header_sha256':hashlib.sha256(header.read_bytes()).hexdigest()}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('rom','out','palette-remap','palette-json'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();print(json.dumps(generate(a.rom,a.out,a.palette_remap,a.palette_json),indent=2))
if __name__=='__main__':main()
