# Packed walking sprites in late Zerg cartridges

Terran, Protoss and Showcase already use the managed packed walking frames.
The Z7–Z10 flag chain still used the scalar drawing fallback. Enabling the
same sidecar in that chain preserves source frames and pose/heading choice;
actors outside the sidecar continue to use the existing fallback. It is a
renderer change only.

| Normal Z10 campaign window | Mean full-loop cycles, stock → packed | p95, stock → packed |
| --- | ---: | ---: |
| Calls 19,000–19,999 | 1,568,109 → 1,555,399 (−0.81%) | 1,965,123 → 1,960,472 |
| Calls 35,000–35,999 | 1,995,868 → 1,939,948 (−2.80%) | 2,375,134 → 2,309,588 |

The paired ordinary-input ARM replays load the same SRAM and match all 34
gameplay checkpoints, including the unit, building and mineral arrays. In
24 evenly spaced samples across the measured windows, the latest drawn
Mode-4 page, palette, camera, visibility and exploration all match.

The probe leaves Z10 IWRAM (`0x03004ae0`) and EWRAM (`0x0203c9b8`) unchanged.
It adds 2,010,160 bytes of loadable ROM data, with 2,579,576 bytes left to
the 32 MiB limit. Z10 packed probe ROM SHA-256:
`d7c219637dbebe8223001a35beb717678cc008ec370f2062ace095196b7aff67`.

The source-only build reuses the existing managed importer dependency and
`CANONICAL_WALK_WORDS=0` comparison switch. It adds a prepared-pixel fixture
target for the Z10 build vector. Release checks include Z7–Z10 ROM guards,
the packed-pixel fixtures, and matching the normal Z10 build to the probe.
This reduces one part of rendering cost; it does not establish 30 fps.
