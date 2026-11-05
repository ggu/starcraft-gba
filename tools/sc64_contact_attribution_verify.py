#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare paired private P7 attribution outputs without modifying them."""
import json
import sys

baseline, diagnostic = (json.load(open(path)) for path in sys.argv[1:3])
for key in ('active_count', 'modal_acknowledged', 'checkpoints'):
    if baseline[key] != diagnostic[key]:
        raise SystemExit(f'mismatch: {key}')
if len(baseline['checkpoints']) != 90 or not baseline['active_count']:
    raise SystemExit('expected 90 checkpoints and advancing updates')
if any(baseline['attribution_totals']):
    raise SystemExit('baseline unexpectedly contains attribution counters')
a = diagnostic['attribution_totals']
if not (a[1] >= a[3] > 0 and a[3] >= a[13] and a[10] == 8 * a[3] and a[11] <= a[10]):
    raise SystemExit('inconsistent query, mask, or selection counts')
n = baseline['active_count']
b = baseline['active_full'] / n
d = diagnostic['active_full'] / n
print(json.dumps({'active_count': n, 'modal_acknowledged': baseline['modal_acknowledged'],
                  'checks': len(baseline['checkpoints']), 'baseline_mean': b,
                  'diagnostic_mean': d, 'overhead_pct': (d/b-1)*100,
                  'baseline_peak': baseline['active_peak'],
                  'diagnostic_peak': diagnostic['active_peak'],
                  'totals': a, 'peaks': diagnostic['attribution_peaks'],
                  'peak_ticks': diagnostic['attribution_peak_ticks']}, indent=2))
