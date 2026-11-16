#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a pinned carried P1/P2 tape from HOME on a normal Protoss P10 ROM."""

import argparse
import json
from pathlib import Path
import resource
import shutil
import struct

from sc64_skirmish_sram_continuation import PhysicalGame
from sc64_races_profile import dwarf_struct_layout
from sc64_z7_normal_replay import input_hashes, require_unchanged


def require(condition, detail):
    if not condition:
        raise ValueError(detail)


def load_plan(mission, tape, raw, host):
    data = json.loads(raw.read_text())
    total = data["total_ticks"]
    final = data["expected"]
    require(
        type(total) is int and 1 <= total <= 200000,
        "expected: type(total) is int and 1<=total<=200000",
    )
    require(
        data["event_overflow"] == 0
        and data["mission"] == mission
        and final["won"] == 1
        and final["lost"] == 0
        and final["screen"] == 3
        and final["phase"] == 3
        and final["mission"] == mission
        and final["race_completed"] & (1 << mission),
        "expected: data['event_overflow']==0 and data['mission']==mission and final['won']==1 and final['lost']==0",
    )
    rows = [json.loads(s) for s in tape.read_text().splitlines()]
    require(
        bool(rows)
        and rows[0].get("kind") == "metadata"
        and rows[0].get("schema") == 1
        and rows[0]["mission"] == mission
        and rows[-1].get("kind") == "checkpoint"
        and rows[-1].get("label") == "final"
        and rows[-1]["call_index"] == total,
        "expected: rows[0]['mission']==mission and rows[-1]['call_index']==total",
    )
    keys = {}
    held = 0
    last = -1
    for q in rows:
        if q["kind"] != "input":
            continue
        i, v = q["call_index"], q["held"]
        require(type(i) is type(v) is int, "expected: type(i) is type(v) is int")
        require(
            last < i < total
            and 0 <= v <= 1023
            and v != held
            and v & 48 != 48
            and v & 192 != 192
            and q["pressed"] == v & ~held,
            "expected: last<i<total and 0<=v<=1023 and v!=held and v&48!=48 and v&192!=192 and q['pressed']==v&~held",
        )
        keys[i] = held = v
        last = i
    require(
        list(keys.items()) == [(q["tick"], q["held"]) for q in data["events"]],
        "expected: list(keys.items())==[(q['tick'],q['held']) for q in data['events']]",
    )
    checks = {q["tick"]: q for q in data["checkpoints"]}
    require(
        len(checks) == len(data["checkpoints"]) and list(checks) == sorted(checks),
        "expected: len(checks)==len(data['checkpoints']) and list(checks)==sorted(checks)",
    )
    require(
        all(type(t) is int and 0 < t < total for t in checks),
        "expected: all(type(t) is int and 0<t<total for t in checks)",
    )
    comparison = {}
    for line in host.read_text().splitlines():
        if not line.startswith("PX CHECK "):
            continue
        q = dict((k, int(v)) for k, v in (pair.split("=") for pair in line.split()[3:]))
        t = q["tick"]
        w = checks.get(t, final)
        require(
            t not in comparison
            and q["mission"] == mission
            and {
                "frame",
                "screen",
                "won",
                "lost",
                "faults",
                "side_units",
                "enemy_units",
                "enemy_buildings",
                "minerals",
                "gas",
            }
            <= q.keys(),
            "expected: t not in comparison and q['mission']==mission",
        )
        require(
            all(q[k] == w[k] for k in ("frame", "screen", "won", "lost")),
            "expected: all(q[k]==w[k] for k in ('frame','screen','won','lost'))",
        )
        comparison[t] = q
    require(
        set(comparison) == set(checks) | {total},
        "expected: set(comparison)==set(checks)|{total}",
    )
    require(
        list(comparison) == sorted(comparison),
        "expected: list(comparison)==sorted(comparison)",
    )
    splice = 5 if mission == 2 else 7
    require(
        list(keys.items())[: splice + 1]
        == list(
            enumerate(
                [1, 0, 1, 0, 8, 0] if mission == 2 else [1, 0, 128, 0, 1, 0, 8, 0]
            )
        ),
        "expected: list(keys.items())[:splice+1]==list(enumerate([1,0,1,0,8,0] if mission==2 else [1,0,128,0,1,0,8,0]))",
    )
    return total, final, keys, comparison, splice


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--mission",
        type=int,
        choices=(1, 2),
        required=True,
        help="campaign ordinal (global mission ID is one higher)",
    )
    for name in ("rom", "elf", "runner", "tape", "raw", "host"):
        parser.add_argument("--" + name, type=Path, required=True)
        parser.add_argument("--" + name + "-sha256", required=True)
    parser.add_argument(
        "--plan", action="store_true", help="validate without starting a guest"
    )
    parser.add_argument("--stop", type=int, help="maximum original tape call")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    mission = args.mission + 1
    rom, elf, runner = args.rom, args.elf, args.runner
    names = ("rom", "elf", "runner", "tape", "raw", "host")
    paths = tuple(getattr(args, n) for n in names) + (Path(__file__),)
    pins = input_hashes(paths)
    for name, path in zip(names, paths):
        require(
            pins[str(path.resolve())] == getattr(args, name + "_sha256"),
            f"SHA-256 mismatch: {name}",
        )
    total, final, keys, comparison, splice = load_plan(
        mission, args.tape, args.raw, args.host
    )
    stop = total if args.stop is None else args.stop
    require(splice < stop <= total, "invalid replay stop")
    if args.plan:
        print(
            json.dumps(
                dict(
                    mission=mission,
                    total=total,
                    stop=stop,
                    splice=splice,
                    key_edges=len(keys),
                    observations=len(comparison),
                    terminal=final,
                    sha256=pins,
                ),
                indent=2,
            )
        )
        return 0
    out = args.out
    require(out is not None, "--out is required when running")
    require(
        shutil.disk_usage(out.parent).free >= int(5.25 * 1024**3),
        "launch requires at least 5.25 GiB free",
    )
    out.mkdir()
    resource.setrlimit(resource.RLIMIT_FSIZE, (256 * 1024, 256 * 1024))
    size, fields = dwarf_struct_layout(elf, "Sc64RaceCampaignState")
    g = PhysicalGame(runner, rom, elf, out / "guest")
    r = g.rsp

    def n(k):
        return int.from_bytes(r.read(g.sym[k], g.sizes[k]), "little")

    def step(key):
        if key != r.held:
            r.set_keys(key)
        r._advance(1)

    def tap(key):
        step(key)
        step(0)

    def f(k):
        return g.front_field(k)

    def modal():
        return struct.unpack("<4H", r.read(0x06013FE0, 8))

    def get(buf, k):
        i, w = fields[k], fields.widths[k]
        return int.from_bytes(buf[i : i + w], "little", signed=fields.signed[k])

    pages = []
    observations = []
    brief = []
    i = -1

    def emit(status, detail=None):
        require_unchanged(paths, pins)
        (out / "result.json").write_text(
            json.dumps(
                dict(
                    status=status,
                    detail=detail,
                    last_index=i,
                    pages=pages,
                    briefing=brief,
                    observations=observations,
                    sha256=pins,
                ),
                indent=2,
            )
        )

    try:
        require(
            n("screen") == n("frame") == f("page") == 0,
            "expected: n('screen')==n('frame')==f('page')==0",
        )
        tap(1)
        require(
            f("page") == 2 and f("mission_page") == 0,
            "expected: f('page')==2 and f('mission_page')==0",
        )
        if mission == 3:
            tap(128)
        tap(1)
        require(
            f("mission") == mission and f("page") == 3,
            "expected: f('mission')==mission and f('page')==3",
        )
        count = f("briefing_pages")
        require(0 < count <= 32, "expected: 0<count<=32")
        for page in range(count):
            require(f("briefing_page") == page, "expected: f('briefing_page')==page")
            if not f("briefing_revealed"):
                tap(1)
            require(
                f("briefing_page") == page and f("briefing_revealed"),
                "expected: f('briefing_page')==page and f('briefing_revealed')",
            )
            brief.append(page)
            if page + 1 < count:
                tap(1)
        step(8)
        require(
            n("screen") == 1 and n("frame") == 0,
            "expected: n('screen')==1 and n('frame')==0",
        )
        # Splice immediately after the old START press; its neutral release still advances frame 1.
        held = 0
        for i in range(splice, stop):
            if i % 500 == 0 and shutil.disk_usage(out).free < int(4.15 * 1024**3):
                emit("disk-floor")
                raise SystemExit(75)
            held = keys.get(i, held)
            step(held)
            for _ in range(48):
                count, page, armed, row = modal()
                if not count:
                    break
                frame = n("frame")
                key = 8 if held & 1 else 1
                require(
                    count <= 13 and row < 256 and armed == 0,
                    "expected: count<=13 and row<256 and armed==0",
                )
                step(held | key)
                require(
                    modal() == (count, page, key, row) and n("frame") == frame,
                    "expected: modal()==(count,page,key,row) and n('frame')==frame",
                )
                step(held)
                nxt = modal()
                require(
                    n("frame") == frame
                    and not nxt[2]
                    and (not nxt[0] or nxt[1::2] != (page, row)),
                    "expected: n('frame')==frame and not nxt[2] and (not nxt[0] or nxt[1::2]!=(page,row))",
                )
                pages.append(
                    dict(
                        tape_call=i + 1,
                        physical=r.completed,
                        frame=frame,
                        page=page,
                        row=row,
                    )
                )
            else:
                raise ValueError("modal bound")
            if n("screen") == 3 and i + 1 != total:
                emit(
                    "diverged",
                    dict(unexpected_result={k: n(k) for k in ("frame", "won", "lost")}),
                )
                raise SystemExit(4)
            if i + 1 not in comparison:
                continue
            s = g.state()
            got = {
                k: n(k) for k in ("frame", "screen", "won", "lost", "minerals", "gas")
            }
            got.update(
                mission=n("race_selected_mission"),
                faults=n("mission_unsupported_count"),
                side_units=sum(q[1] == 0 for q in s["units"]),
                enemy_units=sum(q[1] == 1 for q in s["units"]),
                enemy_buildings=sum(q[1] == 1 for q in s["buildings"]),
            )
            expected = comparison[i + 1]
            diff = {k: [expected[k], v] for k, v in got.items() if expected[k] != v}
            observations.append(
                dict(call=i + 1, physical=r.completed, got=got, diff=diff)
            )
            print(i + 1, got, diff, flush=True)
            if diff:
                emit("diverged", diff)
                raise SystemExit(4)
        if stop == total:
            require(
                len(observations) == len(comparison),
                "expected: len(observations)==len(comparison)",
            )
            c = r.read(g.sym["race_campaign"], size)
            require(
                n("race_mode") == 0
                and n("race_human") == 2
                and n("race_completed") & (1 << mission),
                "expected: n('race_mode')==0 and n('race_human')==2 and n('race_completed')&(1<<mission)",
            )
            require(
                all(
                    get(c, k) == final[k]
                    for k in ("elapsed_frames", "objective_flags", "phase")
                ),
                "expected: all(get(c,k)==final[k] for k in ('elapsed_frames','objective_flags','phase'))",
            )
            require(
                got["screen"] == 3
                and got["won"] == 1
                and got["lost"] == 0
                and got["faults"] == 0,
                "expected: got['screen']==3 and got['won']==1 and got['lost']==0 and got['faults']==0",
            )
        emit("won" if stop == total else "prefix")
    except Exception as exc:
        emit("error", str(exc))
        raise
    finally:
        g.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, KeyError, TypeError, ValueError) as exc:
        raise SystemExit(f"early Protoss replay: {exc}") from exc
