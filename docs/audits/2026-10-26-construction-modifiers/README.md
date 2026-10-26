# Construction effect modes

The source-frame construction tests establish frame selection and an ordinary
sprite composite. They do not establish native N64 effect rendering. A follow-up
review found that the current importer remaps every nontransparent construction
sample as an ordinary palette colour and the game uses an opaque sprite blit.
That loses the meaning of effect-strength and shadow-mask samples.

Authenticated USA ROM `images.dat` (`00C/000`) identifies Warp Anchor image 211
as modifier 9, colour shift 3. Its GRP has 14 frames and nontransparent strength
values 32 and 34–40. The 118 selected construction images comprise 63 normal
images, 53 modifier-10 shadows and two modifier-9/shift-3 effects (images 200
and 211). These are source metadata facts, not a native framebuffer comparison.

The pinned OpenBW revision
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8` supplies an executable reference:
[`load_images_dat`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/data_loading.h)
reads the modifier and colour-shift columns;
[`draw_image`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/ui/ui.h)
uses the source strength and previous destination index for modifier 9, and
uses the previous destination index with a transparency mask for modifier 10.
The current construction path does neither. This is a concrete implementation
gap even though the source GRP frames themselves were imported correctly.

The new `gfx_blit_sprite_mapped` primitive provides the needed bounded
source-row/destination-column lookup. It preserves transparent and invalid
strength samples, paints lookup output zero, respects scene clipping and uses
halfword Mode-4 VRAM writes. A scalar whole-frame oracle passes 495 clipped
and 165 full-screen cases under ASan/UBSan. Target Thumb disassembly has
halfword stores and no byte stores; the optional 460-byte body stays in ROM.
Default build/test and unified build/test pass locally. No gameplay caller is
enabled by this foundation change.

Private experiments retain raw strengths/masks and project candidate lookup
outputs into the current GBA palette. Numeric BOLT entries `00B/005` and
`00B/008` strongly resemble the dark and blue-fire tables by order and size,
but the native resource-loader association has not been established. These
bindings remain private research. GBA palette projection is lossy, and an
OpenBW reference cannot by itself establish N64 visual equivalence.

The separate missing post-timer Protoss sequence also remains: Warp Anchor
`SpecialState1`, temporary script 193 with the texture modifier, then the
building-specific `WarpIn` and materialization signal before completion.
Current gameplay finishes immediately when the build timer expires. The
existing construction lifecycle audit documents that omission. The complete
fix requires effect-mode import/rendering, source-driven finish transitions,
save/load qualification and a direct native visual comparison. A moving frame
or progress bar alone is insufficient evidence.

All checks run locally. No ROM, generated game data, reference implementation
payload or source-game screenshot is included. Actions remains disabled and
manual-only.
