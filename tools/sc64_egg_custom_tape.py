# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate a HOST controller tape; translate its input edges to physical ticks.

This does not validate a guest, save file, or gameplay equivalence.
"""
import json
import sys
from pathlib import Path


def validate(document):
    if document.get("profile") != "z7-custom-egg" or not isinstance(document.get("events"), list):
        raise ValueError("wrong tape profile")
    held = 0
    last = 0
    edges = []
    marks = {}
    for event in document["events"]:
        kind = event.get("kind")
        tick = event.get("host_tick")
        if type(tick) is not int or tick < last or tick <= 0:
            raise ValueError("invalid event timeline")
        last = tick
        if kind == "input":
            mask, pressed = event.get("held"), event.get("pressed")
            if (type(mask) is not int or not 0 <= mask <= 1023 or mask == held or
                    mask & 48 == 48 or mask & 192 == 192 or
                    pressed != (mask & ~held)):
                raise ValueError("invalid input edge")
            if edges and tick <= edges[-1][0] + 1:
                raise ValueError("duplicate input tick")
            edges.append((tick - 1, mask))
            held = mask
        elif kind in ("start", "paid_egg", "save", "birth"):
            if kind in marks:
                raise ValueError("duplicate checkpoint")
            marks[kind] = event
        elif kind != "action":
            raise ValueError("unknown event")
    if set(marks) != {"start", "paid_egg", "save", "birth"}:
        raise ValueError("missing checkpoint")
    start, paid, save, birth = (marks[k] for k in ("start", "paid_egg", "save", "birth"))
    if (start.get("human_race") != 1 or len(start.get("bots", [])) != 1 or
            not start["host_tick"] < paid["host_tick"] < save["host_tick"] < birth["host_tick"] or
            paid.get("slot") != save.get("slot") or save.get("timer", 0) <= 0 or
            paid.get("minerals_before", -1) - paid.get("minerals_after", -1) != 50 or
            paid.get("larva_before", -1) - paid.get("larva_after", -1) != 1 or
            birth.get("zerglings") != paid.get("zerglings_before", -3) + 2):
        raise ValueError("inconsistent checkpoint observations")
    return {"transitions": edges, "save_tick": save["host_tick"],
            "birth_tick": birth["host_tick"]}


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: sc64_egg_custom_tape.py TAPE.json")
    print(json.dumps(validate(json.loads(Path(sys.argv[1]).read_text())), sort_keys=True))
