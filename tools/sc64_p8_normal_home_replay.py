#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify an ordinary HOME P8 tape recorded with HOST dialogue auto-acknowledgment."""

from pathlib import Path
import argparse
import json
import resource
import shutil
import struct
from emulator_smoke import symbols, symbol_sizes
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import PhysicalGame
from sc64_p8_physical_replay import snapshot
from sc64_p8_arm_replay import load_trace
from sc64_z7_normal_replay import input_hashes, require_unchanged

CHECK_FIELDS = (
    "screen",
    "frame",
    "won",
    "lost",
    "fault",
    "outcome",
    "triggers_done",
    "stasis_hp",
    "fenix",
    "hyperion",
)


def require(ok, message):
    if not ok:
        raise ValueError(message)


def validate_rows(rows):
    require(0 < len(rows) <= 256, "invalid P8 dialogue row count")
    for row in rows:
        require(
            0 < row["string_id"] <= 65535
            and 0 <= row["trigger"] < 8
            and 0 <= row["action"] < 64
            and 1 <= row["page_count"] <= 255,
            "invalid P8 dialogue metadata",
        )


def parse_plan(tape, hostlog):
    plan = load_trace(tape)
    end = plan["stop"]
    final = plan["final"]
    require(
        final["won"] == 1
        and final["lost"] == 0
        and final["screen"] == 3
        and final["outcome"] == 1
        and final["stasis_hp"] == -1,
        "the HOST tape must end in P8 victory",
    )
    keys = dict(plan["transitions"])
    require(
        all(not (v & 48 == 48 or v & 192 == 192) for v in keys.values()),
        "opposing directional buttons",
    )
    checks, edges, briefs = {}, {}, {}
    for line in tape.read_text().splitlines():
        row = json.loads(line)
        if row["kind"] == "checkpoint":
            call = row["call_index"]
            require(
                type(call) is int
                and 0 < call <= end
                and call not in checks
                and all(k in row for k in CHECK_FIELDS),
                "invalid checkpoint",
            )
            checks[call] = row
        elif row["kind"] == "input":
            require(
                type(row.get("frame_after")) is int and row["frame_after"] >= 0,
                "invalid input-edge frame",
            )
            edges[row["call_index"]] = row
    for line in hostlog.read_text().splitlines():
        a = line.split()
        if len(a) >= 2 and a[1] == "briefing":
            require(len(a) == 4, "invalid briefing row")
            call, page, total = map(int, (a[0], a[2], a[3]))
            require(
                0 < call < min(checks) and (not briefs or call > max(briefs)),
                "unordered or out-of-range briefing",
            )
            briefs[call] = page, total
    count = len(briefs)
    require(
        0 < count <= 32 and list(briefs.values()) == [(i, count) for i in range(count)],
        "briefing pages are incomplete",
    )
    require(
        end in checks and list(checks) == sorted(checks),
        "checkpoint order or terminal checkpoint",
    )
    require(
        all(
            type(q[k]) is int
            for q in checks.values()
            for k in ("screen", "frame", "won", "lost", "outcome", "stasis_hp")
        ),
        "invalid checkpoint scalar",
    )
    require(
        all(
            isinstance(q[k], list)
            and len(q[k]) == 3
            and all(type(v) is int for v in q[k])
            for q in checks.values()
            for k in ("fault", "triggers_done", "fenix", "hyperion")
        ),
        "invalid checkpoint vector",
    )
    return plan, keys, checks, edges, briefs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "elf", "tape", "host-log", "runner"):
        parser.add_argument("--" + name, required=True, type=Path)
        parser.add_argument("--" + name + "-sha256", required=True)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--stop", type=int)
    parser.add_argument(
        "--plan", action="store_true", help="validate without launching a guest"
    )
    args = parser.parse_args()
    rom, elf, tape, runner, hostlog = (
        args.rom,
        args.elf,
        args.tape,
        args.runner,
        args.host_log,
    )
    paths = (rom, elf, tape, runner, hostlog, Path(__file__))
    pins = input_hashes(paths)
    for path, sha in (
        (rom, args.rom_sha256),
        (elf, args.elf_sha256),
        (tape, args.tape_sha256),
        (runner, args.runner_sha256),
        (hostlog, args.host_log_sha256),
    ):
        require(pins[str(path.resolve())] == sha, f"SHA-256 mismatch: {path}")
    plan, keys, checks, edges, briefchecks = parse_plan(tape, hostlog)
    stop = plan["stop"] if args.stop is None else args.stop
    require(0 < stop <= plan["stop"], "stop outside tape")
    checks = {call: row for call, row in checks.items() if call <= stop}
    rombytes = rom.read_bytes()

    def romread(addr, n):
        off = addr - 0x08000000
        require(
            0 <= off <= len(rombytes) - n,
            "P8 replay check failed: 0<=off<=len(rombytes)-n",
        )
        return rombytes[off : off + n]

    sym, sizes = symbols(elf), symbol_sizes(elf)
    ms, mf = dwarf_struct_layout(elf, "Sc64TextBoxMission")
    rs, rf = dwarf_struct_layout(elf, "Sc64TextBoxRow")
    require(
        ms > 0 and rs > 0 and sizes["sc64_text_box_missions"] >= 28 * ms,
        "P8 replay check failed: sizes['sc64_text_box_missions']>=28*ms",
    )

    def field(buf, fields, k, width=None):
        off, w = fields[k], width or fields.widths[k]
        require(
            w in (1, 2, 4) and 0 <= off <= len(buf) - w, "P8 replay check failed: w"
        )
        return int.from_bytes(
            buf[off : off + w],
            "little",
            signed=False if width else fields.signed.get(k, False),
        )

    mission = romread(sym["sc64_text_box_missions"] + 27 * ms, ms)
    rowbase, count = field(mission, mf, "rows", 4), field(mission, mf, "row_count")
    require(0 < count <= 256, "P8 replay check failed: 0<count<=256")
    auth = []
    for i in range(count):
        row = romread(rowbase + i * rs, rs)
        auth.append(
            {
                k: field(row, rf, k)
                for k in ("trigger", "action", "page_count", "string_id")
            }
        )
    validate_rows(auth)
    if args.plan:
        print(
            json.dumps(
                dict(
                    tape_calls=stop,
                    checkpoints=len(checks),
                    briefings=briefchecks,
                    authenticated_p8_rows=auth,
                    sha256=pins,
                ),
                indent=2,
            )
        )
        return 0
    out = args.out
    require(out is not None, "P8 replay check failed: out")
    require(
        shutil.disk_usage(out.parent).free >= int(5.25 * 1024**3),
        "P8 replay check failed: shutil.disk_usage(out.parent).free>=int(5.25*1024**3)",
    )
    out.mkdir()
    resource.setrlimit(resource.RLIMIT_FSIZE, (256 * 1024, 256 * 1024))
    us, uf = dwarf_struct_layout(elf, "Unit")
    g = PhysicalGame(runner, rom, elf, out / "guest")
    r = g.rsp

    def hero_fields():
        raw = r.read(g.sym["u"], g.sizes["u"])
        require(len(raw) % us == 0, "P8 replay check failed: len(raw)%us==0")
        got = {}
        for name, wanted in (("fenix", 78), ("hyperion", 28)):
            matches = []
            for i in range(0, len(raw), us):
                q = raw[i : i + us]
                if field(q, uf, "alive") and field(q, uf, "raw_type") == wanted:
                    matches.append([field(q, uf, k) for k in ("owner", "hp", "shield")])
            require(len(matches) <= 1, "P8 replay check failed: len(matches)<=1")
            got[name] = matches[0] if matches else [-1, -1, -1]
        return got

    def n(k):
        return int.from_bytes(r.read(g.sym[k], g.sizes[k]), "little")

    def step(mask):
        if mask != r.held:
            r.set_keys(mask)
        r._advance(1)

    def f(k):
        return g.front_field(k)

    obs = []
    pages = []
    brief = []
    drift = None
    mismatch = None

    def emit(status, name="result.json"):
        require_unchanged(paths, pins)
        (out / name).write_text(
            json.dumps(
                dict(
                    status=status,
                    last_index=i,
                    briefing=brief,
                    pages=pages,
                    obs=obs,
                    first_frame_drift=drift,
                    first_mismatch=mismatch,
                    sha256=pins,
                ),
                indent=2,
            )
            + "\n"
        )

    i = -1
    try:
        require(
            n("screen") == n("frame") == f("page") == 0,
            "P8 replay check failed: n('screen')==n('frame')==f('page')==0",
        )
        held = 0

        def modal():
            return struct.unpack("<4H", r.read(0x06013FE0, 8))

        for i in range(stop):
            if i % 500 == 0 and shutil.disk_usage(out).free < int(4.15 * 1024**3):
                emit("disk-floor", "partial.json")
                raise SystemExit(75)
            held = keys.get(i, held)
            step(held)
            if i + 1 in briefchecks:
                page, total = briefchecks[i + 1]
                require(
                    (
                        f("briefing_page"),
                        f("briefing_revealed"),
                        f("briefing_pages"),
                        f("mission"),
                        f("page"),
                        n("screen"),
                    )
                    == (page, 1, total, 27, 3, 0),
                    "P8 replay check failed: (f('briefing_page'),f('briefing_revealed'),f('briefing_pages'),f('mission'),f('page'),n('screen'))==(page,1,total,27,3,0)",
                )
                brief.append([i + 1, page])
            # The observer cannot stop a batched advance when a page or RESULT opens.
            # Read at each boundary so the next tape key never answers either by accident.
            for _ in range(48):
                count, page, armed, row = modal()
                if not count:
                    break
                frame = n("frame")
                key = 8 if held & 1 else 1
                require(
                    0 < count <= 13
                    and not armed
                    and row < len(auth)
                    and page < auth[row]["page_count"],
                    "P8 replay check failed: 0<count<=13",
                )
                step(held | key)
                require(
                    n("frame") == frame and modal() == (count, page, key, row),
                    "P8 replay check failed: n('frame')==frame",
                )
                step(held)
                nxt = modal()
                require(
                    n("frame") == frame
                    and not nxt[2]
                    and (not nxt[0] or (nxt[1], nxt[3]) != (page, row)),
                    "P8 replay check failed: n('frame')==frame",
                )
                pages.append(
                    dict(
                        index=i,
                        page=page,
                        row=row,
                        frame=frame,
                        physical=r.completed,
                        held=held,
                        key=key,
                        **auth[row],
                    )
                )
            else:
                raise RuntimeError("modal bound")
            edge_frame = n("frame") if i in edges else None
            if i in edges and edges[i]["frame_after"] != edge_frame:
                drift = [i, edges[i]["frame_after"], edge_frame]
                print("first key-edge frame drift", drift, flush=True)
                emit("diverged")
                raise SystemExit(4)
            if n("screen") == 3 and i + 1 != plan["stop"]:
                mismatch = dict(
                    call=i + 1, unexpected_result=snapshot(r, g.sym, g.sizes, elf)
                )
                emit("diverged")
                raise SystemExit(4)
            if i + 1 in checks:
                s = snapshot(r, g.sym, g.sizes, elf)
                w = checks[i + 1]
                got = {k: s[k] for k in ("screen", "frame", "won", "lost")}
                got.update(
                    fault=[v["fault"] for v in s["runners"]],
                    outcome=s["runners"][0]["outcome"],
                    triggers_done=[v["done"] for v in s["runners"]],
                    stasis_hp=s["stasis_cells"][0]
                    if len(s["stasis_cells"]) == 1
                    else -1,
                )
                got.update(hero_fields())
                diff = {k: [w[k], v] for k, v in got.items() if w[k] != v}
                obs.append(
                    dict(
                        call=i + 1,
                        physical=r.completed,
                        fields=got,
                        mission=s["race_selected_mission"],
                        campaign=s["campaign"],
                    )
                )
                print("check", i + 1, r.completed, got["frame"], diff, flush=True)
                require(
                    s["race_selected_mission"] == 27
                    and s["race_mode"] == 0
                    and s["campaign"]["mission"] == 27,
                    "P8 replay check failed: s['race_selected_mission']==27",
                )
                if diff:
                    mismatch = dict(call=i + 1, diff=diff)
                    emit("diverged")
                    raise SystemExit(4)
        require(
            r.completed == stop + 2 * len(pages),
            "P8 replay check failed: r.completed==stop+2*len(pages)",
        )
        if stop == plan["stop"]:
            require(
                s["screen"] == 3
                and s["won"] == 1
                and not s["lost"]
                and s["runners"][0]["outcome"] == 1,
                "P8 replay check failed: s['screen']==3",
            )
            require(
                not any(v["fault"] for v in s["runners"]) and not s["stasis_cells"],
                "P8 replay check failed: not",
            )
            require(
                s["race_completed"] & (1 << 27) and s["runners"][0]["done"] & (1 << 4),
                "P8 replay check failed: s['race_completed']&(1<<27)",
            )
            require(
                s["campaign"]["phase"] == 3 and s["campaign"]["objective_flags"] & 1,
                "P8 replay check failed: s['campaign']['phase']==3",
            )
        emit("won" if stop == plan["stop"] else "prefix")
    except Exception:
        emit("error", "partial.json")
        raise
    finally:
        g.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, KeyError, TypeError) as exc:
        raise SystemExit(f"P8 replay: {exc}") from exc
