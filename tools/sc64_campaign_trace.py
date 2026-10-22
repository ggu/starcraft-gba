#!/usr/bin/env python3
"""Join the two controller routes through the real campaign result screen."""
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def main():
    first = json.loads((ROOT / 'build/campaign-wasteland-completion-input.json').read_text())
    second = json.loads((ROOT / 'build/campaign-backwater-completion-input.json').read_text())
    assert first['expected']['won'] == second['expected']['won'] == 1
    assert first['events'][-1]['held'] == 0
    assert second['events'][:4] == [{'tick': 0, 'held': 128}, {'tick': 1, 'held': 0}, {'tick': 2, 'held': 1}, {'tick': 3, 'held': 0}]
    offset = first['total_ticks'] - 2
    events = first['events'] + [{'tick': first['total_ticks'], 'held': 1}] + [dict(tick=e['tick']+offset, held=e['held']) for e in second['events'] if e['tick'] >= 3]
    total = first['total_ticks'] + second['total_ticks'] - 2
    assert len(events) <= 2048 and all(0 <= e['tick'] < total for e in events)
    assert all(a['tick'] < b['tick'] for a, b in zip(events, events[1:]))
    target = ROOT / 'build/campaign-controller-events.txt'
    target.write_text(f'{total} {len(events)}\n' + ''.join(f'{e["tick"]} {e["held"]}\n' for e in events))
    print(f'Campaign controller route: {total} ticks, {len(events)} input changes -> {target}')
if __name__ == '__main__': main()
