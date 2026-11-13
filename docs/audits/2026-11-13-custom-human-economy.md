# Human economy in the ordinary Terran skirmish controller

A current Terran HOST sanitizer scout used normal HOME and field inputs
against one Zerg CPU. Three Barracks and groups of 14–22 Marines did not
produce a win on either map. The longest Outpost run lost at frame 32,519;
it spent its first attacks on an expansion and lost production at home.
A second policy built a Refinery, Academy and Bunker through the field
menu, but still lost. On Outpost one run had 14 minerals and 157 gas at
frame 11,812. It had eight to ten live units while the CPU had 28–37;
a mixed army did not come out of production in time. The controller is
unpublished; these are diagnostic losses, not proof of a gameplay bug
or a full skirmish qualification.

The engine leaves a Terran builder idle when a building finishes. AI
has its own planner for the next order. A human must issue a new gather
order; the smart mineral action yields when a selectable actor is under
the cursor. The scout tried the nearest visible patch and surrounding
points; it did not record a successful post-construction mineral order
on the retained combined-arms tape. The established campaign helper
tries the same offsets after selecting the exact idle worker and verifies
the worker order and target, so reuse that success criterion and trace
selection, any cargo already carried, and actual deposit progress in a
short isolated ordinary skirmish before another whole match trial.

This does not show whether a stock N64 SCV automatically resumes mining
after construction, nor whether the GBA should. Distinguish a helper that
never selects a legal mineral target from a successful order that fails
to deposit. Then sustain mineral income and defend the production site
while the controller assembles a legal mixed army. Do not change AI or
worker behavior just to make a replay win.

| Retained HOST tape | SHA-256 |
| --- | --- |
| Outpost, 14-Marine goal, loss at 32,519 | `e90062fb237d2e4810cb2d904a3d95270c84dbc5757b24b45f01572685c7946d` |
| Outpost, mixed-army scout, loss at 13,952 | `03b309a484e1b9a17766275c873b70976fb57555dc4ebd48993ed6dd20283270` |
| Outpost, visible patch offsets, loss at 13,476 | `a7112a039ffbfa8887461eb8d92777eeea92d6aa472ccf9e7ac352f84cf079d0` |
