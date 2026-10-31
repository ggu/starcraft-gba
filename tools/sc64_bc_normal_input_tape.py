#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate the source-bound BC impact witness and emit replay transitions."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
END_TICK = 17937
END_FRAME = 17658

def unique_object(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise ValueError(f'duplicate JSON key: {key}')
        out[key] = value
    return out

def integer(value, name, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'{name} must be an integer in {low}..{high}')
    return value

def convert(document):
    if not isinstance(document, dict) or set(document) != {'profile', 'events'} or document['profile'] != 'unified-race-skirmish':
        raise ValueError('invalid trace header')
    events = document['events']
    if not isinstance(events, list) or not events:
        raise ValueError('events must be a nonempty list')
    prior_tick = prior_frame = previous_held = 0
    input_tick = 0; starts = results = 0; rows = []
    for i, event in enumerate(events):
        if not isinstance(event, dict) or not isinstance(event.get('kind'), str):
            raise ValueError(f'event {i}: invalid record')
        tick = integer(event.get('host_tick'), 'host_tick', 1, END_TICK)
        frame = integer(event.get('frame'), 'frame', 0, END_FRAME)
        if tick < prior_tick or frame < prior_frame:
            raise ValueError(f'event {i}: tick/frame order regressed')
        prior_tick, prior_frame = tick, frame
        kind = event['kind']
        if kind == 'input':
            if set(event) != {'kind','host_tick','frame','held','pressed'}:
                raise ValueError('invalid input fields')
            held = integer(event['held'], 'held', 0, 0x3ff)
            pressed = integer(event['pressed'], 'pressed', 0, 0x3ff)
            if tick <= input_tick or held == previous_held or pressed != (held & ~previous_held):
                raise ValueError('invalid input ordering or key transition')
            input_tick, previous_held = tick, held
            rows.append((tick, held, pressed))
        elif kind == 'start':
            if event != {'kind':'start','host_tick':20,'frame':1,'map':1,'human_race':0,'bots':[{'owner':1,'race':1}]} or starts or results:
                raise ValueError('invalid scenario start')
            for name in ('map', 'human_race'):
                integer(event[name], name, 0, 2)
            for name in ('owner', 'race'):
                integer(event['bots'][0][name], name, 0, 4)
            starts += 1
        elif kind == 'action':
            fields={'kind','action','host_tick','frame','type','worker','x','y','minerals_before','minerals_after','gas_before','gas_after'}
            if set(event) != fields or event['action'] != 'build' or not starts or results:
                raise ValueError('invalid action record')
            for name in fields-{'kind','action','host_tick','frame'}:
                integer(event[name], name, 0, 0x7fffffff)
        elif kind == 'result':
            if event != {'kind':'result','result':'impact','host_tick':END_TICK,'frame':END_FRAME,'won':0,'lost':0,'saved_flight':1} or not starts or results or i != len(events)-1:
                raise ValueError('invalid terminal result')
            for name in ('won', 'lost', 'saved_flight'):
                integer(event[name], name, 0, 1)
            results += 1
        else:
            raise ValueError(f'unknown event kind: {kind}')
    if starts != 1 or results != 1 or not rows or rows[0][0] != 1 or previous_held != 0:
        raise ValueError('incomplete input witness')
    return ''.join(f'{tick} {held} {pressed}\n' for tick, held, pressed in rows)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    try:
        out=a.output.resolve()
        if out.is_relative_to(ROOT) and not out.is_relative_to(ROOT/'.local'):
            raise ValueError('output inside source tree must be under .local')
        with a.input.open(encoding='utf-8') as f:
            data=json.load(f, object_pairs_hook=unique_object)
        text=convert(data)
        with out.open('x',encoding='ascii') as f: f.write(text)
    except (OSError, ValueError, json.JSONDecodeError) as e:
        p.exit(1, f'bc normal input tape: {e}\n')
if __name__=='__main__':main()
