# Native Blood Bath: northwest minimap aim

This is the first camera calibration toward a possible worker-loss
witness. It builds on the [ordinary Barracks and Marine replay](2026-11-14-native-result-opening.md)
and the [failed SCV interruption](2026-11-17-native-scv-foundation-interruption.md).
It does not place a building or test combat.

The one bounded replay matched all 137 archived command rows, 18 full
frames, six guest rows, and the early renderer binding through 15,962.
This included the 11,900 completed-Barracks anchor and the exact
three-Marine selection at 15,962. The reference guest row has a live
opposing Marine at (220,244); that is a prior sampled position, not a
new sighting. Checked inputs stayed unchanged.

The only new inputs were eleven stick-only steps at X=-22,000 and
Y=+27,000 and twenty-two neutral steps. The frame at command end
15,995 showed the green pointer at the northwest minimap's top edge.
Its visible pixels span x=49–65, y=156–163, whereas the minimap border
starts at y=158. The pointer tip is above that border, so this frame
does not qualify an A click. No button, camera jump, unit move,
enemy contact or construction was observed after the archived prefix.

For a future interior calibration, first reauthenticate the complete
archived prefix and this exact 15,995 frame. Hold X at zero and try
only four steps at Y=+18,000 with sixteen neutral steps, capturing at
16,015. Require a visibly identifiable pointer tip within x=44–62,
y=166–175, inside the minimap and clear of the upper border. This is
a proposed pulse, not an observed coordinate mapping. If it lands
outside that interior box or the selection changes, stop without A.
Only a separately reviewed and exactly reproduced interior frame can
authorize a camera jump. A jump still would not authorize a Move,
placement, or claim about enemy contact.

| Local receipt | SHA-256 |
| --- | --- |
| West aim replay manifest | `51e44a8802b7151b1629d6ef411dfc6b642b25ec537f0817af2b9b687d052522` |
| West aim diagnostic | `38dfac4964f729ec29d5972c409e0b05b9a553f6bd9f16f002e2cd5afaece636` |
| Cursor-only frame at 15,995 | `b121db1daa98207c4a8ef9a09584d49458ea6aa4195a874a2a4beecab0d76eb6` |

## The interior pointer is blue

The next bounded replay matched all 139 earlier command rows, 19 full
frames and six byte-exact guest rows through 15,995. It sent only four
steps at Y=+18,000 and sixteen neutral, capturing at 16,015. The camera
still showed the home base and the HUD still showed three selected units.

The live gate checked for the previous green pointer and withheld A
when there was no match. Review of the new raw frame found a **blue**
pointer after it entered the minimap, at x=49–67, y=168–176, with the
tip at (49,168). That tip is ten pixels below the top border and inside
the predeclared target box. The old green template was too restrictive;
it did not report a failure to reach the interior. The run stopped
without A. There was no camera jump, unit order, enemy sighting or
placement. Checked inputs remained unchanged.

Before attempting a camera jump, reproduce the exact archived prefix,
15,995 edge frame and this 16,015 interior frame. The blue pointer and
its interior tip are the reviewed evidence. An unconditional green
template, a color change alone, or a different image cannot authorize A.
After the exact gate, send one A pulse and record where the camera lands
while the three Marines remain selected. Keep the unit-move and
placement gates separate.

| Local receipt | SHA-256 |
| --- | --- |
| Interior-aim replay manifest | `bcaceeed8adde6261abce1ea45edc17a7560ded3f2f0f0d0658a188c5b5790a1` |
| Interior-aim diagnostic | `e0b612ff7924b4b35fa4601b2d7e5021a270f6d1b3966ffbccf0f893c3d5a0df` |
| Reviewed interior frame at 16,015 | `de3ad54a3dc99dfd945dbee06c32c4662e3d0a0260f2c2eec5463552b3d32a7e` |

## One A jumps to the northwest without an order

A fresh trial matched all 141 previous command rows, all 20 full frames
and six guest rows through 16,015. The exact interior frame confirmed
the blue tip at (49,168) and the unchanged three-Marine selection.
Only after that check, the observer sent one eight-step A pulse and
twenty neutral steps, ending at 16,043 with unchanged inputs.

The white minimap viewport moved from x=124–162 to x=34–72, with its
top still at y=159. The terminal HUD retained three selected units
and the Move command. The northwest field was fogged and showed no
enemy. This establishes the camera jump and the visible retained
selection; there was no Move or attack input and no confirmed contact
or build site. No second attempt was run.

For the next scout stage, reproduce the exact landing first. Verify a
world-space cursor target separately before using the displayed Move.
Confirm a live enemy and reachable ground through ordinary scouting
before choosing any site; the fogged landing cannot qualify one.

| Local receipt | SHA-256 |
| --- | --- |
| Camera-only replay manifest | `5003893f9351202e5b0f49ddd3d4ffe5a35a17fc3c3846be800a3285a6d54a44` |
| Camera-only diagnostic | `2c7a637d5a630693beb1690784a0da880d450765004033957a3888d431a5aef4` |
| Reviewed northwest landing at 16,043 | `424577f78a64ad8db2b36e8deb417f66cd30b4f9e75ba696581a905cbc7fc7c8` |
