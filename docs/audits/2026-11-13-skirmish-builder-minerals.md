# Return an idle Terran builder to minerals

In an ordinary HOME launch of skirmish map 0, Terran against one CPU,
Barracks construction finished at host tick 3,599. The selected idle
SCV's closest visible mineral field was patch 3 at (118,180). Friendly
units took precedence over minerals at all nine positions sampled by
the scout, so none offered Mine.

With the bounded alternate-field helper, the scout selected patch 0
using the cursor at (92,190), and pressed A at tick 3,984. At tick 4,075
that same SCV's carried minerals fell from eight to zero while the
Terran bank increased from 1,160 to 1,168. The run used the Terran
profile flags under HOST AddressSanitizer and UndefinedBehaviorSanitizer,
ordinary buttons only, and stopped at the fixture's 6,700-call bound.
This is a worker economy witness; it does not claim a skirmish win.
The helper is separate from existing campaign controllers and their tapes.
