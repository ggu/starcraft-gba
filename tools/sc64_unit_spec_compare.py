#!/usr/bin/env python3
"""Compare public rule-helper probes with pinned BWAPI defaults, without assets.

The CSV is a table audit, not a claim of native N64 or runtime equivalence.
Runtime exceptions below are explicit findings from the dated source review;
review them again when applying this helper to a newer engine revision.
"""
from pathlib import Path
import argparse
import csv
import hashlib
import json


NOTES = {
    'U_RAYNOR': 'Legacy compatibility actor. Raw identity depends on profile; early campaign loaders can bypass canonical mapping. Non-T4 raw20 record has armor0 and no weapon; T4 raw19 record has HP200/armor0/no weapon versus stock Vulture300/3/30. Do not substitute for dedicated T5/ZP heroes.',
    'U_FIREBAT': 'Special flame approximation: one primary8 plus authored3 HP radial splash when a unit-target primary hit reaches HP; splash can include surviving primary target and other-owner units. Source maxGroundHits3 is a maximum, not guaranteed three hits.',
    'U_REAVER': 'Special Scarab lifecycle adaptation: direct100 damage/cooldown60; source Reaver has no direct weapon. Stock Scarab weapon carries100 base damage. No generic Scarab ammo/travel/splash parity established.',
    'U_SPIDER_MINE': 'Special autonomous T5 mine handler owns arm/acquire/chase/detonation. Generic weapon/motion fields are not its operating values; exclude from automatic generic verdicts.',
    'U_MUTALISK': 'Primary base weapon matches; generic path does not implement stock bounce/glaive lifecycle.',
    'U_SIEGE_TANK': 'Base50 versus stock70. Generic path does not enforce stock64-source-pixel minimum range or stock radial splash. Zero training cost/time is an in-place mode-switch policy.',
    'U_SCOURGE': 'P3/P6/P7 inherit an explicit air-target bypass despite generic air helper false; Z7 enables a separate path. Other audited profiles leave it unarmed. Suicide lifecycle handled specially.',
    'U_P7_CARRIER': 'No direct stock weapon; matching zero generic damage does not establish interceptor lifecycle/combat support.',
    'U_P7_ARCHON': 'P7 profile enables a separate Archon splash adapter; generic weapon-table match alone does not qualify splash geometry or runtime parity.',
    'U_DROPSHIP': 'Raw11 import mapping and transport support depend on profile; T5 shared movement step2, otherwise1.',
    'U_Z7_EGG': 'Immobile production lifecycle actor; its timers do not use ordinary unit movement/combat.',
}


def row_for(profile, engine, unit, reference, weapons):
    ref = reference[unit['raw_id']]
    flags = set(engine['defines'])
    name = unit['name']
    identity = ('canonical' if unit['canonical_type'] == unit['enum_id'] else
                'unmapped' if unit['canonical_type'] < 0 else 'noncanonical_alias')
    # This is the reviewed shared move_toward path, not a new gameplay model.
    step = 2 if name in {'U_ZERGLING', 'U_FIREBAT', 'U_RAYNOR',
                         'U_ZP_RAYNOR_VULTURE', 'U_VULTURE'} else 1
    if name == 'U_DROPSHIP' and '-DSC64_T5_RUNTIME' in flags:
        step = 2
    if unit['actor_flags'] and not unit['actor_flags'] & 2:
        step = 0
    special_motion = name in {'U_SPIDER_MINE', 'U_Z7_EGG'}
    motion_status = ('special_lifecycle' if special_motion else
                     'source_immobile_order_gating_not_fully_audited' if not ref['top_speed'] else
                     'nominal_axis_rate_match_only' if abs(step - ref['top_speed'] * .4) < 1e-8 else
                     'nominal_axis_rate_difference')
    row = {'profile': profile, 'engine_commit': engine['engine_commit'],
           'enum': name, 'enum_id': unit['enum_id'], 'raw_id': unit['raw_id'],
           'reference_unit': ref['name'], 'identity': identity,
           'canonical_enum_id': unit['canonical_type'], 'actor_policy_flags': unit['actor_flags']}
    for field in ('hp', 'shields', 'armor'):
        row.update({f'engine_{field}': unit[field], f'reference_{field}': ref[field],
                    f'{field}_match': unit[field] == ref[field]})
    row.update({'engine_flying': unit['flying'],
                'reference_top_speed_source_px_per_frame': ref['top_speed'],
                'shared_motion_axis_step_px_per_update': step,
                'expected_axis_step_if_source24_engine30_scale_half': round(ref['top_speed'] * .4, 6),
                'movement_assessment': motion_status})
    notes = [NOTES[name]] if name in NOTES else []
    for target in ('ground', 'air'):
        w = weapons[ref[target + '_weapon']['id']]
        e = unit[target]
        eligible = bool(unit['air_eligible']) if target == 'air' else e['damage'] > 0
        if name == 'U_SCOURGE':
            special = ({'-DSC64_P3_ENABLED', '-DSC64_UNIFIED'} <= flags or
                       '-DSC64_Z7_SCOURGE_COMBAT' in flags)
            eligible = bool(special and target == 'air')
        has_weapon = w['id'] < 130
        difference = []
        if has_weapon:
            if not eligible:
                difference.append('no_ordinary_eligible_weapon')
            for ef, rf in [('damage', 'damage_amount'), ('cooldown', 'damage_cooldown'), ('range', 'max_range')]:
                if e[ef] != w[rf]:
                    difference.append(ef)
            if e['hits'] != ref[target + '_weapon_hits'] * w['damage_factor']:
                difference.append('attack_potential_multiplicity')
            if w['min_range']:
                difference.append('minimum_range_not_in_generic_path')
        if name in {'U_REAVER', 'U_SPIDER_MINE', 'U_P7_CARRIER'}:
            status = 'special_lifecycle_requires_runtime_review'
        elif name == 'U_FIREBAT' and target == 'ground':
            status = 'special_flame_model_difference'
        elif not has_weapon:
            status = 'no_reference_direct_weapon'
        else:
            status = ';'.join(difference) if difference else 'base_table_match_only'
        bonus = 1
        if name == 'U_SCOURGE':
            bonus = 0
        if name == 'U_RAYNOR_VULTURE' and '-DSC64_T5_RUNTIME' in flags:
            bonus = 2
        for key, value in {
            'reference_weapon': w['name'],
            'engine_class_eligible': eligible,
            'engine_damage_per_hit': e['damage'],
            'reference_damage_amount': w['damage_amount'],
            'engine_hit_calls': e['hits'],
            'reference_unit_max_hits': ref[target + '_weapon_hits'],
            'reference_weapon_damage_factor': w['damage_factor'],
            'engine_source_basis_cooldown': e['cooldown'],
            'reference_base_cooldown': w['damage_cooldown'],
            'engine_converted_cooldown_updates': (e['cooldown'] * 5 + 3) // 4,
            'reference_converted_cooldown_updates': (w['damage_cooldown'] * 5 + 3) // 4,
            'engine_source_basis_max_range': e['range'],
            'reference_max_range': w['max_range'],
            'engine_half_scale_max_range': (e['range'] + 1) // 2,
            'reference_min_range': w['min_range'],
            'reference_damage_type': w['damage_type']['name'],
            'generic_side_upgrade_increment': bonus if eligible else '',
            'reference_weapon_upgrade_bonus': w['damage_bonus'],
            'assessment': status,
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


def compare(reference_path, probes_dir, output):
    facts = json.loads(reference_path.read_text())
    units = {u['id']: u for u in facts['units']}
    weapons = {w['id']: w for w in facts['weapons']}
    output.mkdir(parents=True, exist_ok=True)
    rows, profiles = [], []
    for file in sorted(probes_dir.glob('*/engine-facts.json')):
        if file.parent.name.startswith('optional-'):
            continue
        engine = json.loads(file.read_text())
        rows.extend(row_for(file.parent.name, engine, u, units, weapons) for u in engine['units'])
        profiles.append({key: engine[key] for key in
                         ('engine_commit', 'profile_variable', 'defines', 'rules_sha256')} |
                        {'profile': file.parent.name, 'unit_rows': len(engine['units']),
                         'probe_json_sha256': hashlib.sha256(file.read_bytes()).hexdigest()})
    if not rows:
        raise ValueError('No profile probes found')
    dest = output / 'unit-specs.csv'
    with dest.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys(), lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    canonical = [r for r in rows if r['identity'] == 'canonical']
    manifest = {
        'schema': 1, 'audit_date': '2026-10-25',
        'classification': 'stock_PC_default_table_and_authored_runtime_review_not_N64_verification',
        'reference': {k: v for k, v in facts['provenance'].items() if k != 'repository'},
        'profiles': profiles,
        'rows': len(rows), 'distinct_raw_ids': len({r['raw_id'] for r in rows}),
        'distinct_enum_names': len({r['enum'] for r in rows}),
        'canonical_rows': len(canonical),
        'canonical_matches': {field: sum(r[field + '_match'] for r in canonical)
                              for field in ('hp', 'shields', 'armor')},
        'csv_sha256': hashlib.sha256(dest.read_bytes()).hexdigest(),
        'limitations': [
            'Rule helpers are compiled; full constructors and runtime behavior were source reviewed, not exhaustively executed.',
            'Profile-specific Scourge eligibility is reviewed logic, not inferred from its generic helper.',
            'Common movement axis rate assumes half spatial scale and source24/engine30 clock; not measured wall time.',
            'No native N64 default-table/timing equivalence, every spell, or generic scenario override qualification.',
            'Generic attack summaries do not replace special weapon/animation/lifecycle validation.',
        ],
    }
    (output / 'unit-specs-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({k: manifest[k] for k in ('rows', 'distinct_raw_ids', 'distinct_enum_names', 'canonical_rows', 'canonical_matches')}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-json', type=Path, required=True)
    parser.add_argument('--probes-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    compare(args.reference_json, args.probes_dir, args.output)
