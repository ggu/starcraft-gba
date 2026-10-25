#!/usr/bin/env python3
"""Compare current stock-profile helper probes with pinned BWAPI defaults.

Table/helper equality is reported separately from full runtime behavior. This
does not claim native N64 equivalence, collision/pathing fidelity, or complete
special-weapon lifecycles.
"""
from pathlib import Path
import argparse
import csv
from datetime import date
import hashlib
import json


NOTES = {
    'U_RAYNOR': 'Legacy objective identity is profile-dependent and may alias a different raw unit. Numeric facts are looked up by raw ID; do not infer them from this enum name.',
    'U_FIREBAT': 'The runtime uses a special flame approximation rather than generic per-impact weapon resolution: one primary hit plus a fixed radial HP splash. Source maxGroundHits3 is not three guaranteed hits on one target; flame geometry remains incomplete.',
    'U_REAVER': 'Special Scarab lifecycle adaptation: direct100 damage/cooldown60; source Reaver has no direct weapon. Stock Scarab weapon carries100 base damage. No generic Scarab ammo/travel/splash parity established.',
    'U_SPIDER_MINE': 'Special autonomous T5 mine handler owns arm/acquire/chase/detonation. Generic weapon/motion fields are not its operating values; exclude from automatic generic verdicts.',
    'U_MUTALISK': 'Base weapon fields are read from the stock table; generic attack resolution does not implement the full Glaive Wurm bounce lifecycle.',
    'U_SIEGE_TANK': 'The stock Siege weapon helper supplies 70/150 base damage and a 64-source-pixel minimum range; the runtime tick fixture checks the half-scale dead zone. Siege splash and native mode/lifecycle behavior remain incomplete. Zero cost/time is an in-place mode-switch policy.',
    'U_SCOURGE': 'P3/P6/P7 inherit an explicit air-target bypass despite generic air helper false; Z7 enables a separate path. Other audited profiles leave it unarmed. Suicide lifecycle handled specially.',
    'U_P7_CARRIER': 'No direct stock weapon; matching zero generic damage does not establish interceptor lifecycle/combat support.',
    'U_P7_ARCHON': 'P7 profile enables a separate Archon splash adapter; generic weapon-table match alone does not qualify splash geometry or runtime parity.',
    'U_DROPSHIP': 'Raw11 import mapping and transport support depend on profile. Nominal speed comes from the raw-ID stock table; transport and cargo lifecycle remain separate.',
    'U_Z7_EGG': 'Immobile production lifecycle actor; its timers do not use ordinary unit movement/combat.',
}

SPECIAL_ATTACKS = {'U_FIREBAT', 'U_REAVER', 'U_SPIDER_MINE', 'U_P7_CARRIER', 'U_P7_ARCHON'}


def row_for(profile, engine, unit, reference, weapons):
    ref = reference[unit['raw_id']]
    flags = set(engine['defines'])
    name = unit['name']
    identity = ('canonical' if unit['canonical_type'] == unit['enum_id'] else
                'unmapped' if unit['canonical_type'] < 0 else 'noncanonical_alias')
    special_motion = name in {'U_SPIDER_MINE', 'U_Z7_EGG'}
    motion = unit['motion']
    source_rate_q10 = round(float(ref['top_speed']) * 1024)
    expected_runtime_rate_q10 = (source_rate_q10 * 2 + 2) // 5
    expected_diagonal_rate_q10 = (expected_runtime_rate_q10 * 724 + 512) >> 10
    motion_matches = (motion['available'] and
                      motion['runtime_speed_q10'] == expected_runtime_rate_q10 and
                      motion['cardinal_steps_1024'] == expected_runtime_rate_q10 and
                      motion['diagonal_steps_1024'] == expected_diagonal_rate_q10)
    motion_status = ('special_lifecycle' if special_motion else
                     'stock_immobile' if not ref['top_speed'] and motion_matches else
                     'fixedpoint_helper_matches_assumed_scale_and_clock' if motion_matches else
                     'fixedpoint_helper_rate_difference')
    row = {'profile': profile, 'engine_commit': engine['engine_commit'],
           'enum': name, 'enum_id': unit['enum_id'], 'raw_id': unit['raw_id'],
           'reference_unit': ref['name'], 'identity': identity,
           'canonical_enum_id': unit['canonical_type'], 'actor_policy_flags': unit['actor_flags']}
    for field in ('hp', 'shields', 'armor'):
        row.update({f'engine_{field}': unit[field], f'reference_{field}': ref[field],
                    f'{field}_match': unit[field] == ref[field]})
    row.update({'engine_flying': unit['flying'],
                'reference_top_speed_source_px_per_frame': ref['top_speed'],
                'reference_top_speed_q10': source_rate_q10,
                'expected_scaled_runtime_rate_q10': expected_runtime_rate_q10,
                'motion_helper_runtime_rate_q10': motion['runtime_speed_q10'],
                'motion_helper_cardinal_steps_per_1024': motion['cardinal_steps_1024'],
                'motion_helper_diagonal_steps_per_1024': motion['diagonal_steps_1024'],
                'motion_assumes_half_spatial_scale_and_24_over_30_clock': True,
                'movement_assessment': motion_status})
    notes = [NOTES[name]] if name in NOTES else []
    for target in ('ground', 'air'):
        w = weapons[ref[target + '_weapon']['id']]
        e = unit[target + '_rule']
        stock = unit[target + '_stock']
        eligible = bool(unit['air_eligible']) if target == 'air' else e['damage'] > 0
        if name == 'U_SCOURGE':
            special = ({'-DSC64_P3_ENABLED', '-DSC64_UNIFIED'} <= flags or
                       '-DSC64_Z7_SCOURGE_COMBAT' in flags)
            eligible = bool(special and target == 'air')
        has_weapon = w['id'] < 130 and w['damage_amount'] > 0
        stock_available = bool(stock['available'])
        if has_weapon and stock_available:
            matches = (stock['weapon_id'] == w['id'] and
                       stock['damage'] == w['damage_amount'] and
                       stock['upgrade_bonus'] == w['damage_bonus'] and
                       stock['damage_factor'] == w['damage_factor'] and
                       stock['damage_class'] == w['damage_type']['id'] and
                       stock['cooldown'] == w['damage_cooldown'] and
                       stock['min_range'] == w['min_range'] and
                       stock['max_range'] == w['max_range'] and
                       stock['unit_hits'] == ref[target + '_weapon_hits'] and
                       stock['impact_count'] == ref[target + '_weapon_hits'] * w['damage_factor'])
            helper_status = 'stock_attack_helper_matches_reference' if matches else 'stock_attack_helper_mismatch'
        elif not has_weapon and stock_available:
            helper_status = 'unexpected_stock_attack_helper_available'
        elif not has_weapon:
            helper_status = 'no_reference_direct_weapon'
        else:
            helper_status = 'stock_attack_helper_unavailable'
        if stock_available:
            boundaries_passed = (stock['at_min_accepted'] and stock['at_max_accepted'] and
                                 stock['above_max_rejected'] and
                                 (not stock['min_range'] or stock['below_min_rejected']))
            minrange_status = ('range_boundary_helper_passed' if boundaries_passed else
                               'range_boundary_helper_failed')
        else:
            minrange_status = 'no_minimum_range'
        if has_weapon and not eligible:
            runtime_status = 'profile_eligibility_disables_stock_attack'
        elif name in SPECIAL_ATTACKS:
            runtime_status = 'special_lifecycle_or_damage_adapter'
        elif name == 'U_SCOURGE':
            runtime_status = 'special_suicide_lifecycle_with_profile_air_eligibility'
        elif name == 'U_MUTALISK':
            runtime_status = 'stock_primary_attack_fields; bounce_lifecycle_unmodeled'
        elif name == 'U_SIEGE_TANK':
            runtime_status = 'stock_attack_fields; splash_and_mode_lifecycle_unmodeled'
        else:
            runtime_status = 'generic_stock_attack_path_with_profile_exceptions'
        for key, value in {
            'reference_weapon': w['name'],
            'race_rule_helper_class_eligible': eligible,
            'race_rule_helper_damage': e['damage'],
            'race_rule_helper_cooldown': e['cooldown'],
            'race_rule_helper_max_range': e['range'],
            'race_rule_helper_hits': e['hits'],
            'stock_helper_available': stock_available,
            'stock_helper_weapon_id': stock['weapon_id'],
            'stock_helper_damage_per_impact': stock['damage'],
            'stock_helper_upgrade_bonus': stock['upgrade_bonus'],
            'stock_helper_damage_factor': stock['damage_factor'],
            'stock_helper_damage_class_id': stock['damage_class'],
            'stock_helper_unit_max_hits': stock['unit_hits'],
            'stock_helper_impact_count': stock['impact_count'],
            'stock_helper_cooldown_source_frames': stock['cooldown'],
            'stock_helper_converted_cooldown_updates': (stock['cooldown'] * 5 + 3) // 4,
            'stock_helper_min_range_source_pixels': stock['min_range'],
            'stock_helper_half_scale_min_range': (stock['min_range'] + 1) // 2,
            'stock_helper_max_range_source_pixels': stock['max_range'],
            'stock_helper_half_scale_max_range': (stock['max_range'] + 1) // 2,
            'stock_helper_attack_range_assessment': minrange_status,
            'stock_helper_assessment': helper_status,
            'runtime_behavior_assessment': runtime_status,
            'reference_damage_amount': w['damage_amount'],
            'reference_unit_max_hits': ref[target + '_weapon_hits'],
            'reference_weapon_damage_factor': w['damage_factor'],
            'reference_base_cooldown': w['damage_cooldown'],
            'reference_converted_cooldown_updates': (w['damage_cooldown'] * 5 + 3) // 4,
            'reference_max_range': w['max_range'],
            'reference_min_range': w['min_range'],
            'reference_damage_type': w['damage_type']['name'],
            'reference_weapon_upgrade_bonus': w['damage_bonus'],
        }.items():
            row[target + '_' + key] = value
    # Costs/time/supply retained for review, not automatically called bugs:
    # Zerg paired production, heroes and transformed modes have different routes.
    for ef, rf in [('minerals', 'mineral_cost'), ('gas', 'gas_cost'),
                   ('build_frames', 'build_time'), ('supply', 'supply_required'),
                   ('supply_provided', 'supply_provided')]:
        row['rule_' + ef] = unit[ef]
        row['reference_' + ef] = ref[rf]
    row['notes'] = ' '.join(notes)
    return row


def compare(reference_path, probes_dir, output, csv_output=None):
    facts = json.loads(reference_path.read_text())
    units = {u['id']: u for u in facts['units']}
    weapons = {w['id']: w for w in facts['weapons']}
    output.mkdir(parents=True, exist_ok=True)
    rows, profiles = [], []
    for file in sorted(probes_dir.glob('*/engine-facts.json')):
        if file.parent.name.startswith('optional-'):
            continue
        engine = json.loads(file.read_text())
        if engine.get('schema') != 2 or not all(engine.get(key) for key in
                ('stock_profiles_sha256', 'stock_combat_sha256', 'stock_motion_sha256')):
            raise ValueError(f'{file} uses a stale probe schema; regenerate it with '
                             'tools/sc64_unit_spec_engine.py before comparison')
        rows.extend(row_for(file.parent.name, engine, u, units, weapons) for u in engine['units'])
        profiles.append({key: engine[key] for key in
                         ('engine_commit', 'profile_variable', 'defines', 'rules_sha256',
                          'stock_profiles_sha256', 'stock_combat_sha256', 'stock_motion_sha256')} |
                        {'profile': file.parent.name, 'unit_rows': len(engine['units']),
                         'probe_json_sha256': hashlib.sha256(file.read_bytes()).hexdigest()})
    if not rows:
        raise ValueError('No profile probes found')
    dest = output / 'unit-specs.json'
    # Columnar JSON keeps the public numeric report compact and within the
    # repository's source-text allowlist; CSV exports belong in ignored .local.
    encoded_rows = ',\n'.join(
        '    ' + json.dumps(list(row.values()), separators=(',', ':'))
        for row in rows)
    dest.write_text('{\n  "columns": ' + json.dumps(list(rows[0])) +
                    ',\n  "rows": [\n' + encoded_rows + '\n  ]\n}\n')
    if csv_output is not None:
        csv_output.parent.mkdir(parents=True, exist_ok=True)
        with csv_output.open('w', newline='') as stream:
            writer = csv.DictWriter(stream, fieldnames=rows[0].keys(), lineterminator="\n")
            writer.writeheader()
            writer.writerows(rows)
    canonical = [r for r in rows if r['identity'] == 'canonical']
    manifest = {
        'schema': 2, 'audit_date': date.today().isoformat(),
        'classification': 'stock_PC_default_table_and_authored_runtime_review_not_N64_verification',
        'reference': {k: v for k, v in facts['provenance'].items() if k != 'repository'},
        'profiles': profiles,
        'rows': len(rows), 'distinct_raw_ids': len({r['raw_id'] for r in rows}),
        'distinct_enum_names': len({r['enum'] for r in rows}),
        'canonical_rows': len(canonical),
        'canonical_matches': {field: sum(r[field + '_match'] for r in canonical)
                              for field in ('hp', 'shields', 'armor')},
        'comparison_sha256': hashlib.sha256(dest.read_bytes()).hexdigest(),
        'limitations': [
            'Movement figures probe the fixed-point nominal-rate helper over its 1024-phase cycle; travel remains affected by pathing, collision, order gating and source-vs-runtime clock assumptions.',
            'Stock attack helper values are compared separately from race-rule fallback helpers and special runtime adapters.',
            'Minimum-range helper boundary checks are not a substitute for full target geometry; the current Siege minimum-range tick behavior also has an engine runtime fixture.',
            'No native N64 default-table/timing equivalence, every spell, or generic scenario override qualification.',
            'Generic attack summaries do not establish full projectile, splash, upgrade-family or special weapon/animation/lifecycle behavior.',
        ],
    }
    (output / 'unit-specs-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({k: manifest[k] for k in ('rows', 'distinct_raw_ids', 'distinct_enum_names', 'canonical_rows', 'canonical_matches')}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-json', type=Path, required=True)
    parser.add_argument('--probes-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--csv-output', type=Path, help='Optional local spreadsheet export; use an ignored .local path')
    args = parser.parse_args()
    compare(args.reference_json, args.probes_dir, args.output, args.csv_output)
