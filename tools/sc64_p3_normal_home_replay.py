#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify a pinned native-modal P3 HOME tape on a normal Protoss P10 ROM."""

import argparse
import json
from pathlib import Path
import resource
import shutil
import struct

from sc64_p3_controller_replay import _queued_type_count
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, PhysicalGame
from sc64_z7_normal_replay import input_hashes, require_unchanged
from sc64_z2_normal_replay import terminal_modal_release

MISSION = 22
FIELDS = (
    "frame",
    "elapsed",
    "screen",
    "won",
    "lost",
    "p3_outcome",
    "p3_sequence",
    "p3_fired",
    "campaign_phase",
    "objective_flags",
    "action_ticks",
    "side_units",
    "side_buildings",
    "force2_units",
    "force2_buildings",
    "force3_units",
    "force3_buildings",
    "minerals",
    "gas",
    "supply",
    "supply_cap",
    "gateways",
    "cursor_x",
    "cursor_y",
    "selected_building",
    "geyser_count",
    "zealots_queued",
)


def require(ok, message):
    if not ok:
        raise ValueError(message)


def load_plan(trace, pages):
    require(trace["schema"] == 1 and trace["mission"] == MISSION, "wrong P3 trace")
    require(
        trace["policy"] == "protoss-stock-economy-cannons-assault", "wrong P3 policy"
    )
    keys, checks = {}, {}
    held, previous, start = 0, 0, None
    rows = trace["events"]
    for row in rows:
        call, kind = row["host_tick"], row["kind"]
        require(
            type(call) is int and previous <= call <= 1_100_000 and call > 0,
            "invalid P3 call order",
        )
        previous = call
        if kind == "input":
            mask = row["held"]
            require(
                type(mask) is int
                and type(row["pressed"]) is int
                and 0 <= mask <= 1023
                and mask & 48 != 48
                and mask & 192 != 192
                and (not keys or mask != held)
                and row["pressed"] == mask & ~held
                and call - 1 not in keys,
                "invalid P3 held-key edge",
            )
            keys[call - 1] = held = mask
        else:
            require(
                kind in ("start", "periodic", "won")
                and call not in checks
                and all(type(row.get(k)) is int for k in FIELDS),
                "invalid P3 checkpoint",
            )
            if kind == "start":
                require(
                    start is None and not checks and row["screen"] == 1,
                    "invalid P3 launch checkpoint",
                )
                start = call
            else:
                require(
                    start is not None
                    and kind != "won"
                    or start is not None
                    and row is rows[-1],
                    "checkpoint outside P3 play",
                )
            checks[call] = row
    final = rows[-1] if rows else {}
    require(
        final.get("kind") == "won"
        and final["won"] == 1
        and final["lost"] == 0
        and final["screen"] == 3
        and final["p3_outcome"] == 1
        and final["p3_sequence"] == 5
        and final["campaign_phase"] == 3
        and final["frame"] == final["elapsed"]
        and final["objective_flags"] & 1
        and final["force2_buildings"] == final["force3_buildings"] == 0,
        "P3 tape lacks strict terminal victory",
    )
    stop = final["host_tick"]
    require(
        list(keys.items())[:6] == list(enumerate((1, 0, 16, 0, 1, 0)))
        and max(keys) < stop,
        "wrong P3 HOME opening or trailing input",
    )
    briefs, modal = {}, {}
    for line in pages.splitlines():
        fields = line.split()
        if len(fields) == 4 and fields[1] == "briefing":
            call, page, total = map(int, (fields[0], *fields[2:]))
            require(
                6 <= call <= start and call not in briefs, "invalid briefing boundary"
            )
            briefs[call] = (page, total)
        elif len(fields) == 8 and all(s.isdecimal() for s in fields):
            call, frame, box, page, trigger, action, held, key = map(int, fields)
            require(
                0 < call <= stop - 2
                and call not in modal
                and box < 256
                and 0 <= frame <= final["frame"]
                and 0 <= page < 256
                and trigger < 256
                and action < 64
                and held <= 1023
                and key == (8 if held & 1 else 1)
                and keys.get(call) == held | key
                and keys.get(call + 1) == held,
                "invalid authored P3 input",
            )
            modal[call] = (frame, box, page, trigger, action, held, key)
    count = len(briefs)
    require(
        0 < count <= 32
        and list(briefs) == sorted(briefs)
        and list(briefs.values()) == [(i, count) for i in range(count)],
        "missing or unordered briefing",
    )
    require(
        list(modal) == sorted(modal)
        and all(b - a >= 2 for a, b in zip(modal, list(modal)[1:])),
        "overlapping modal actions",
    )
    return stop, keys, checks, briefs, modal


def run(args):
    names = ("trace", "pages", "rom", "elf", "runner")
    paths = tuple(getattr(args, k) for k in names) + (Path(__file__),)
    pins = input_hashes(paths)
    for name, path in zip(names, paths):
        require(
            pins[str(path.resolve())] == getattr(args, name + "_sha256"),
            f"SHA-256 mismatch: {name}",
        )
    stop, keys, checks, briefs, modal = load_plan(
        json.loads(args.trace.read_text()), args.pages.read_text()
    )
    cut = stop if args.stop is None else args.stop
    require(0 < cut <= stop, "invalid replay boundary")
    if args.plan:
        print(
            json.dumps(
                dict(
                    tape_calls=stop,
                    calls=cut,
                    checkpoints=len(checks),
                    briefing=len(briefs),
                    modal_pages=len(modal),
                    sha256=pins,
                ),
                indent=2,
            )
        )
        return
    require(
        args.out and shutil.disk_usage(args.out.parent).free >= int(5.25 * 1024**3),
        "new --out and 5.25 GiB free required",
    )
    args.out.mkdir()
    resource.setrlimit(resource.RLIMIT_FSIZE, (256 * 1024, 256 * 1024))
    layouts = {
        s: dwarf_struct_layout(args.elf, t)
        for s, t in (
            ("race_campaign", "Sc64RaceCampaignState"),
            ("race_p3", "Sc64P3State"),
        )
    }
    g = PhysicalGame(args.runner, args.rom, args.elf, args.out / "guest")
    r = g.rsp
    observations, acknowledged, revealed = [], [], []
    release, terminal = None, None

    def scalar(name, signed=False):
        return int.from_bytes(
            r.read(g.sym[name], g.sizes[name]), "little", signed=signed
        )

    def read_fields(symbol, names):
        size, layout = layouts[symbol]
        require(g.sizes[symbol] == size, f"invalid {symbol} size")
        raw = r.read(g.sym[symbol], size)
        out = {}
        for k in names:
            off, width = layout[k], layout.widths[k]
            require(
                width in (1, 2, 4) and 0 <= off <= size - width, f"invalid {symbol}.{k}"
            )
            out[k] = int.from_bytes(
                raw[off : off + width], "little", signed=layout.signed[k]
            )
        return out

    def observe():
        s = g.state()
        uoi, boi, bti = (
            UNIT_FIELDS.index("owner") + 1,
            BUILDING_FIELDS.index("owner") + 1,
            BUILDING_FIELDS.index("type") + 1,
        )
        c = read_fields(
            "race_campaign", ("mission", "phase", "objective_flags", "elapsed_frames")
        )
        p3 = read_fields("race_p3", ("outcome", "sequence", "fired"))
        require(
            c["mission"] == scalar("race_selected_mission") == MISSION
            and scalar("race_human") == 2
            and scalar("race_mode") == 0,
            "wrong guest mission",
        )
        observed = dict(
            frame=s["frame"],
            screen=s["screen"],
            elapsed=c["elapsed_frames"],
            campaign_phase=c["phase"],
            objective_flags=c["objective_flags"],
            gateways=int(any(b[boi] == 0 and b[bti] == 14 for b in s["buildings"])),
        )
        observed.update({f"p3_{k}": v for k, v in p3.items()})
        for key in (
            "won",
            "lost",
            "minerals",
            "gas",
            "supply_cap",
            "selected_building",
            "geyser_count",
        ):
            observed[key] = scalar(key, key == "selected_building")
        observed.update(
            supply=scalar("supply_used"),
            cursor_x=scalar("cx"),
            cursor_y=scalar("cy"),
            action_ticks=scalar("race_p3_action_ticks"),
        )
        for prefix, owner in (("side", 0), ("force2", 1), ("force3", 4)):
            observed[f"{prefix}_units"] = sum(u[uoi] == owner for u in s["units"])
            observed[f"{prefix}_buildings"] = sum(
                b[boi] == owner for b in s["buildings"]
            )
        raw = r.read(g.sym["b"], g.sizes["b"])
        layout = g.building_layout
        observed["zealots_queued"] = _queued_type_count(
            raw,
            len(raw) // g.building_stride,
            g.building_stride,
            layout,
            layout.widths,
            layout.signed,
            owner=0,
            unit_type=10,
        )
        return observed

    def report(status, **detail):
        require_unchanged(paths, pins)
        (args.out / "result.json").write_text(
            json.dumps(
                dict(
                    status=status,
                    observations=observations,
                    briefing=revealed,
                    pages=acknowledged,
                    sha256=pins,
                    **detail,
                ),
                indent=2,
            )
            + "\n"
        )

    modal_checks = {}
    for call, row in modal.items():
        frame, box, page, trigger, action, held, key = row
        modal_checks.setdefault(call, []).append(("modal", *row))
        modal_checks.setdefault(call + 1, []).append(("armed", frame, box, page, key))
        modal_checks.setdefault(call + 2, []).append(("released", frame))
    try:
        require(
            scalar("screen") == scalar("frame") == g.front_field("page") == 0,
            "guest is not at fresh HOME",
        )
        for call in range(1, cut + 1):
            if call % 500 == 0 and shutil.disk_usage(args.out).free < int(
                4.15 * 1024**3
            ):
                report("disk-floor", next_call=call)
                raise SystemExit(75)
            if call - 1 in keys and keys[call - 1] != r.held:
                r.set_keys(keys[call - 1])
            r._advance(1)
            if call in briefs:
                page, count = briefs[call]
                actual = tuple(
                    g.front_field(k)
                    for k in (
                        "briefing_page",
                        "briefing_pages",
                        "briefing_revealed",
                        "mission",
                        "page",
                    )
                )
                require(
                    actual == (page, count, 1, MISSION, 3),
                    f"briefing differs at {call}",
                )
                revealed.append([call, page])
            state = struct.unpack("<4H", r.read(0x06013FE0, 8))
            count, page, armed, box = state
            require(
                not count
                or any(q[0] in ("modal", "armed") for q in modal_checks.get(call, ())),
                f"unexpected dialogue at {call}",
            )
            for kind, *q in modal_checks.get(call, ()):
                require(scalar("frame") == q[0], f"dialogue advanced frame at {call}")
                if kind == "modal":
                    require(
                        0 < count <= 13
                        and (box, page, armed, r.held) == (q[1], q[2], 0, q[5]),
                        f"dialogue row mismatch at {call}",
                    )
                    acknowledged.append([call, *q])
                elif kind == "armed":
                    require(
                        0 < count <= 13 and (box, page, armed) == tuple(q[1:]),
                        f"dialogue arm mismatch at {call}",
                    )
                else:
                    require(not armed, f"dialogue release mismatch at {call}")
            if scalar("screen") == 3 or release is not None:
                actual = observe()
                require(
                    all(actual[k] == checks[stop][k] for k in FIELDS),
                    f"terminal state differs at {call}: {actual}",
                )
                require(
                    not scalar("mission_unsupported_count")
                    and scalar("race_completed") & (1 << MISSION),
                    "terminal faults or no completion",
                )
                if release is not None:
                    require(
                        not count and not armed and not r.held and actual == terminal,
                        f"terminal neutral tail changed at {call}",
                    )
                elif call < stop:
                    require(
                        terminal_modal_release(
                            call,
                            stop,
                            keys,
                            modal_checks,
                            state,
                            actual["frame"],
                            checks[stop],
                        ),
                        f"unexpected RESULT at {call}",
                    )
                    release, terminal = call, actual
            if call in checks:
                actual = observe()
                diff = {
                    k: [checks[call][k], actual[k]]
                    for k in FIELDS
                    if actual[k] != checks[call][k]
                }
                observations.append(dict(call=call, fields=actual, differences=diff))
                require(not diff, f"checkpoint {call} differs: {diff}")
        report(
            "won" if cut == stop else "prefix",
            calls=cut,
            terminal_modal_release=release,
        )
    except Exception as exc:
        report("diverged", call=locals().get("call"), error=str(exc))
        raise
    finally:
        g.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("trace", "pages", "rom", "elf", "runner"):
        parser.add_argument("--" + name, type=Path, required=True)
        parser.add_argument("--" + name + "-sha256", required=True)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--plan", action="store_true")
    parser.add_argument("--stop", type=int)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
