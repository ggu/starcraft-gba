# Research family facts before gameplay integration

The existing generic human weapon/armor levels do not distinguish source upgrade
families. Typed Forge/Core research needs the selected weapon's upgrade ID and
the actor's armor upgrade ID. Target flight and faction alone are insufficient.

The pinned BWAPI parser now also reads `armorUpgrade` from UnitType.cpp and
`upgrade` from WeaponType.cpp, resolving symbols through UpgradeType.h. The
optional stock generator output is a separate source facts header; it is not
included by the engine. Regenerating the existing stock profile header remains
byte-identical. No release runtime, live actor storage or save format changes.

Independent comparison with the authenticated owned N64 ROM found every one of
228 armor mappings and 130 native weapon mappings equal. BWAPI additionally has
weapon sentinel rows 130 and 131, which are not native weapon-table records.
Unit table `00C/001`, armor offset 8400, SHA-256
`54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df`;
weapon table `00C/007`, upgrade offset 2210, SHA-256
`764c6c1535a8ba5d3097c62c5755769ab9d1217e9ef4621e64720a8d2a22e730`.
The independent local script/receipt remain private under the upgrade-policy
checkout's `.local/stock-family-audit/`. Prices come from a different table,
`00C/006`; they must not be cited as the mapping source.

Witnesses include Dragoon weapon 66 using ground upgrade 13 for both target
kinds, Scout weapons 73/74 and Interceptor 79 using air upgrade 14, Probe weapon
62 retaining source ID 60, and Scarab weapon 82 retaining separate upgrade 35.
ID 60 is not the BWAPI None sentinel 61.

Checks completed:

- Parsed a clean pinned BWAPI `d727fed68558c506163048ea889131d8cbb33915` checkout;
  source file hashes are in the private facts receipt.
- Compiled C dump compared all 228 unit, 95 building and 132 weapon profiles
  with extracted source facts; all 228 armor and 132 weapon family IDs matched.
- The owned-N64 family witnesses passed ASan/UBSan. This is a source fact check,
  not an emulator, gameplay, clock or hardware-speed qualification.
- Review caught a short-array/out-of-bounds generator hazard. The generator now
  rejects wrong counts, ordering and invalid IDs before writing output. Three
  asset-free regression tests cover valid, truncated and malformed input.
- Both generated headers reproduce exactly; the existing stock header is
  unchanged. The new family header remains unused by release code.

Reproduce with a clean pinned reference checkout:

```sh
python3 tools/sc64_unit_spec_reference.py --reference /path/to/bwapi --output .local/research/stock-facts.json
python3 tools/sc64_stock_profiles_generate.py --facts .local/research/stock-facts.json --output .local/research/stock.h --upgrade-output .local/research/families.h
python3 tools/test_sc64_stock_profiles.py --facts .local/research/stock-facts.json --upgrade-families
python3 tests/test_sc64_stock_upgrade_families_generate.py
```

Native compilation and execution use the shared native lock and disk reserve.
The generator's normal output is still the ordinary stock profile table; the
extra research-family output is requested explicitly. The private facts JSON
and any owned-ROM comparison inputs are not committed.

## Remaining gameplay contract

Research must use compact owner 0–4 in custom games and authenticated raw source
slots in campaigns. Existing Protoss service slots are a different namespace:
their generic human is slot 7, and their mission bindings omit early campaigns.
Do not reuse that key directly or infer a unique raw owner from a compact enemy.
Candidate save validation must bind its own mode/mission, not live globals.

Usable upgrades require atomic payment, source level caps/initial levels,
producer-bound jobs, cancellation and lifecycle cleanup, canonical active-job
save validation, legacy generic-level/pending-job migration, every combat path
(including shields and specialized attacks), and ordinary building-context
menus. These facts and the completed-level wire experiment do not implement
any of those paths. Keep experimental flags off in normal releases until they
are integrated and qualified across the applicable products and modes.
