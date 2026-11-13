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

Replaying the recorded held keys from fresh HOME on the exact KODAK
Terran ROM (`cbba1c5`) reproduced the worker transition. The physical
observer read SCV 0 idle at call 3,599 and mining patch 0 at call 3,984.
Across calls 4,074–4,075 (frames 3,997–3,998) the same SCV unloaded
eight minerals and the bank rose from 1,160 to 1,168. The observer
only read guest state. The cartridge was not rebuilt.

| Input or receipt | SHA-256 |
| --- | --- |
| KODAK Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Held-key tape | `9759ddf7ea225de6219df0a1ae57263598093078e59617b17d10814284752eb4` |
| Physical receipt | `bf3ad295f9463047900978c5ac7b5c19f67d161a5dc6fb4c10e24a40efd7ee8b` |
