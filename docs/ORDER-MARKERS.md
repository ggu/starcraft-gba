# Placement previews and order markers: source evidence

The StarCraft 64 USA manual, [printed pages 7–9](https://nintendo64.pl/wp-content/uploads/StarCraft-64-USA.pdf) (PDF pages 5–6), distinguishes the arrow-shaped selection cursor from a targeting cursor used for some commands. It describes moving units, waypoints and rally points, but does not specify the artwork, color, lifetime, or animation of an on-map order-confirmation marker. The optional importer authenticates N64 sprite 318 → image 445 → GRP `00E/0CE`, and IScript 288's five green cursor-marker frames. The PC reference associates this image with destination feedback; the N64 input routine and a separate red attack-marker mapping have not been traced. The GBA uses these real ROM frames for accepted move and attack confirmations, rather than inventing a red-tinted source variant.

For building placement, printed page 9 says that selecting a building brings up an outline of that building, colored red at a location where it cannot be placed and green where it can. This is evidence for a building-specific preview and those two meanings. The manual does not establish the source image ID, exact pixels, whether the outline is filled or translucent, its anchor, or how non-location failures such as insufficient resources affect its color. A generic footprint rectangle does not convey the building-specific shape described by the manual. No native placement rendering routine or frame sequence has been traced here.

In the optional imported-art profiles, `mission_building_sprite` resolves the same building sprite used for placed-building art, and `draw_mission_sprite` positions it by the frame canvas and offsets. Those are an available source-derived silhouette for an independently authored preview; a command-menu icon is not a substitute. Coloring or outlining its opaque pixels is an approximation until the N64 placement renderer is traced. The default profile uses original art and requires an authored equivalent. The snapped world position and placement eligibility must continue to agree with the actual placement path; previews must clip to the battlefield rather than the HUD in both clipped and non-clipped renderers. Source frames and generated banks remain private as described in [References](REFERENCES.md).

Useful verification covers different building types and races, allowed and rejected sites, snapping and gas sites, map and viewport edges, cancellation, and drawing order over the battlefield and under the HUD. A future exact-fidelity claim needs a native trace or clearly identified gameplay capture of the placement state, not the manual alone.

## Shared implementation

`RACES_FLAGS` and `RACES_INPUTS` include the marker bank for every derived race
product, including Showcase, custom skirmish and campaign builds. Prepare it
from the already verified local ROM with:

```sh
python3 tools/sc64_order_marker_pack.py --rom /absolute/path/to/StarCraft64.z64
```

Marker colors map each authenticated source RGB to the nearest nontransparent reserved GBA UI color. This keeps the bright green visible against terrain; the limited palette approximates its original shades and does not imply a traced N64 display palette.

The default output stays ignored in `.local/sc64/order-marker/generated/`.
Only the importer, renderer and synthetic fixtures are distributed. The default
original-art product retains its authored acknowledgement.

The marker samples five one-update source frames across seven 30 Hz gameplay
updates. It reuses the existing transient world position, color and lifetime;
there is no new mutable actor or save state. Rendering does not advance the
clock. Pausing freezes it; a successful load clears it, and a rejected load
preserves the current acknowledgement. Failed orders do not replace it. Gold
placement/gather acknowledgements retain their separate authored feedback.
Movement confirmations mark the requested group center; direct-target attacks
capture the target position when the order is accepted.

Placement feedback traces the nontransparent boundary of the actual imported
building frame, at the same canvas anchor as its placed artwork. Green means
the current placement can be issued; red means the placement path rejects it,
including global prerequisites/resources. These latter meanings are a GBA
policy beyond the manual's location wording. The outline raster and
four-neighbor hole edges are authored approximations. Missing/invalid frames
retain the older generic fallback; this is not source placement-art parity.
The original-art product keeps its authored preview. Both new renderers clip
to the battlefield explicitly, protecting HUD bands without relying on a
profile's global clipping flag.

Asset-free checks are included in `./dev test`, and can be run separately:

```sh
make test-sc64-order-marker-vfx test-sc64-building-placement-vfx
python3 -m unittest discover -s tests -p 'test_sc64_order_marker_pack.py'
```

With private inputs already prepared, `make test-sc64-order-marker-game`
checks all six race-product vectors and a separate direct-attack variant.
`make test-unified` includes its unified/direct cases. These checks do not
establish native N64 input/UI equivalence or new hardware frame-rate results.
