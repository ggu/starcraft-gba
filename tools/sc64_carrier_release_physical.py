#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""One bounded ordinary-key gate on a pinned Carrier release candidate."""
from __future__ import annotations

import argparse
from contextlib import contextmanager
import fcntl
import hashlib
import json
from pathlib import Path
import resource
import shutil
import signal
import struct
import sys

from emulator_smoke import symbol_sizes, symbols
from sc64_campaign_metadata import read_campaign_metadata
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import (
    BUILDING_FIELDS, ContinuationError, FRONT_HOME, KEY_A, KEY_B, KEY_DOWN, KEY_L,
    KEY_RIGHT, KEY_START, PhysicalGame, RACES, SCREEN_PAUSE, SCREEN_PLAY,
    SCREEN_TITLE, SEAT_OPTIONS, UNIT_FIELDS, entity_rows, physical_seats, summary,
    validate_launch_seats,
)
from sc64_split_smoke import FRONT_BRIEFING, FRONT_MISSION, frontend_offsets

LOCK = "/tmp/sc64-native-build-20261031.lock"
RUNNER = "4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549"
# Exact ROM/ELF pairs from the independently linked direct-gate candidates.
# Their ROMs matched the selector-linked products byte for byte.
PRODUCTS = {
    "terran": (
        "965ceacbcd5059230a90c8a0dc49c2ced6d19d95ebb08eaa493e8608c9d984d6",
        "0e2c38e0688c175a5477539c74c015fff3ef92f7b639586db8db0f72dfbd1330",
        0x76435442, 0x764D5442,
    ),
    "zerg": (
        "2719456571efc210638a832bcca8b069078a39475075e97a4193e63df66b58ed",
        "5d22af38b5091d61af2c65afa7910f9d8d78245c2dfac49b4f6b1f0dbae8daa2",
        0x56435A61, 0x564E5A61,
    ),
    "showcase": (
        "9d5dca67d8387a12f98c370d8cb43734be75a56f2586c8cc2282ad28e210139e",
        "56dfeb9ceb0003d1552cbb29b76b70c077b5af82e1a278f8dd72a5eace8021d3",
        None, 0x764C5058,
    ),
}
# November 16 regular Transport1 package (source release cb210bc). These
# executable stock writers, rather than arbitrary SRAM files or a guessed
# tag, supply the migration predecessor.
STOCK = {
    "terran": (
        "a7ce64dadab8b334bbc2f6ccdbbc0aeff06378d2f44d6ecd39702ade2825aa59",
        "d64da445d3bf3949269f6f46c50a5497d1cdf95e1f199dc4285e5039f0783ffb",
        0x76435442,
    ),
    "zerg": (
        "2b907cdecb5769dde471de04b280ffe7de79a3b43874b2eaabfc2a9b8be43eb8",
        "672ea55a9cb8d8121e452034909abe64bd5fff9aab066c011fca06afc79ea84a",
        0x56435A61,
    ),
    "showcase": (
        "8d0d9f9464629ac3c694ea812e8cd53b8d5d65839bf1de33ad891e88dc416590",
        "085417775b50a22346bb72408829d7c47ad029b99317bccc57fbb243acf0bc82",
        0x76435058,
    ),
}
CAMPAIGN_IDS = {
    "terran": tuple(range(4, 14)),
    "zerg": (0, 1, *range(14, 22)),
}
MAPS = {"outpost": 0, "blood-bath": 1}
LINEUPS = {"mixed": ("p", "tzp"), "sparse": ("z", "p-r")}
CASES = ("campaign", "migration", *(f"{m}-{lineup}" for m in MAPS for lineup in LINEUPS))
RESERVE, FILE_LIMIT, OUTPUT_LIMIT = 5 * 2**30, 8 * 2**20, 16 * 2**20
SLOT, SRAM = 16384, 32768
MAX_STEPS, CONTINUE, KEY_R = 512, 60, 256


def require(ok, message):
    if not ok:
        raise ContinuationError(message)


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def plan_case(profile, case, chapter=None):
    _, _, campaign_tag, custom_tag = PRODUCTS[profile]
    if case == "campaign":
        require(profile in CAMPAIGN_IDS, "Showcase has no campaign gate")
        chapter = 10 if chapter is None else chapter
        require(type(chapter) is int and 1 <= chapter <= 10, "campaign chapter must be 1–10")
        ordinal = chapter - 1
        return dict(case=case, mode=0, mission=CAMPAIGN_IDS[profile][ordinal], ordinal=ordinal,
                    chapter=chapter,
                    human=RACES.index(profile[0]), tag=campaign_tag, idle=30)
    require(chapter is None, "chapter applies only to campaign")
    if case == "migration":
        # Named seats preserve the noncontiguous mask without relying on
        # a randomly resolved faction differing between products.
        return dict(case=case, mode=1, map=1, human=RACES.index("z"), seats="p-t",
                    tag=custom_tag, predecessor_tag=STOCK[profile][2], idle=120)
    map_name, lineup = case.rsplit("-", 1)
    human, seats = LINEUPS[lineup]
    return dict(case=case, mode=1, map=MAPS[map_name],
                human=RACES.index(human), seats=seats, tag=custom_tag, idle=120)


def pin_inputs(args):
    plan = plan_case(args.profile, args.case, args.chapter)
    rom, elf, _, _ = PRODUCTS[args.profile]
    expected = {"rom": rom, "elf": elf, "runner": RUNNER}
    if args.case == "migration":
        require(args.stock_rom is not None and args.stock_elf is not None,
                "migration requires the pinned stock ROM and ELF")
        stock_rom, stock_elf, _ = STOCK[args.profile]
        expected.update(stock_rom=stock_rom, stock_elf=stock_elf)
    else:
        require(args.stock_rom is None and args.stock_elf is None,
                "stock ROM/ELF apply only to migration")
    pins = {}
    for role, digest in expected.items():
        path = getattr(args, role).resolve(strict=True)
        require(path.is_file() and sha(path) == digest, f"unqualified {role} for {args.profile}")
        pins[role] = {"path": str(path), "sha256": digest}
    check_layout(args.elf, plan)
    if args.case == "migration":
        check_layout(args.stock_elf, plan)
    if not plan["mode"]:
        catalog = read_campaign_metadata(args.rom, args.elf, args.profile)
        ids = catalog["enabled_mission_ids"]
        require(tuple(ids) == CAMPAIGN_IDS[args.profile] and
                [entry["chapter"] for entry in catalog["enabled_missions"]] == list(range(1, 11)),
                "compiled campaign catalog differs from pinned campaign entries")
        plan["catalog"] = ids
    return plan, pins


def check_layout(elf, plan):
    sym, sizes = symbols(elf), symbol_sizes(elf)
    scalar = ("demo_frame", "screen", "frame", "race_mode", "race_human", "race_bots", "won", "lost",
              "race_skirmish_map", "race_selected_mission", "race_completed", "minerals", "gas")
    require(all(sizes.get(name) == 4 and name in sym for name in scalar), "scalar ABI differs")
    require(0x08000000 <= sym.get("gba_keys", 0) < 0x0a000000, "input sample symbol differs")
    for name in ("m", "race_minerals", "race_gas", "u", "b", "race_front"):
        require(name in sym and 0 < sizes.get(name, 0) <= 65536, f"invalid {name} bank")
    front = frontend_offsets(elf)
    require(max(front.values()) + 4 <= sizes["race_front"], "frontend DWARF exceeds its bank")
    if plan["mode"]:
        needed = {"race_skirmish_slots": 4, "race_bot_races": 12, "race_base_x": 16, "race_base_y": 16,
                  "race_player": 20}
        require(all(name in sym and sizes.get(name) == width for name, width in needed.items()),
                "independent seat/starting-position ABI differs")
    else:
        require(sizes.get("race_campaign") == 24, "unexpected campaign state layout")
    building = "Sc64ZpCompactBuilding" if sizes["b"] == 160 * 48 else "Building"
    for bank, kind, fields in (("u", "Unit", UNIT_FIELDS), ("b", building, BUILDING_FIELDS)):
        stride, layout = dwarf_struct_layout(elf, kind)
        # An all-empty bank is enough to exercise the normal layout validator.
        entity_rows(bytes(sizes[bank]), stride, layout, fields, active_queue=bank == "b")


def same_inputs(pins):
    for role, pin in pins.items():
        require(sha(Path(pin["path"])) == pin["sha256"], f"{role} changed during the gate")


def slot(raw, index, tag, sequence):
    require(len(raw) == SRAM and index in (0, 1), "invalid SRAM size or slot")
    start = SLOT * index
    found, length, check, serial = struct.unpack_from("<4I", raw, start)
    require((found, serial) == (tag, sequence) and 1 <= length <= SLOT - 16,
            f"unexpected committed slot {index} tag, length or sequence")
    payload = raw[start + 16:start + 16 + length]
    fnv = 2166136261
    for byte in payload:
        fnv = ((fnv ^ byte) * 16777619) & 0xffffffff
    require(fnv == check, f"slot {index} checksum differs")
    return payload


def verify_journal(first, second, tag):
    a = slot(first, 0, tag, 1)
    require(first[SLOT:] == b"\xff" * SLOT, "first save did not leave its other bank erased")
    require(second[:SLOT] == first[:SLOT], "re-save changed the predecessor SRAM bank")
    require(slot(second, 1, tag, 2) == a, "immediate fresh re-save changed the payload")
    return dict(tag=f"{tag:#010x}", payload_bytes=len(a),
                payload_sha256=hashlib.sha256(a).hexdigest())


def verify_migration_journal(first, second, old_tag, new_tag):
    require(old_tag != new_tag, "migration tags must remain distinct")
    old = slot(first, 0, old_tag, 1)
    require(first[SLOT:] == b"\xff" * SLOT, "stock writer did not leave its other bank erased")
    require(second[:SLOT] == first[:SLOT], "conversion changed the original stock SRAM bank")
    new = slot(second, 1, new_tag, 2)
    return dict(stock_tag=f"{old_tag:#010x}", stock_bytes=len(old),
                stock_payload_sha256=hashlib.sha256(old).hexdigest(),
                new_tag=f"{new_tag:#010x}", new_bytes=len(new),
                new_payload_sha256=hashlib.sha256(new).hexdigest())


class BoundedGame(PhysicalGame):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.inputs = []

    def step(self, keys=0, n=1):
        before = self.updates
        require(1 <= n and before + n <= MAX_STEPS, "physical update ceiling")
        super().step(keys, n)
        self.inputs.append([before, keys, n])


@contextmanager
def open_game(args, out, source, destination, *, stock=False):
    rom, elf = (args.stock_rom, args.stock_elf) if stock else (args.rom, args.elf)
    game = BoundedGame(args.runner, rom, elf, out,
                       save_input=source, save_output=destination)
    try:
        require((game.u32("screen"), game.front_field("page")) == (SCREEN_TITLE, FRONT_HOME),
                "physical process did not start on HOME")
        yield game
    finally:
        game.close()


def launch(game, plan):
    if plan["mode"]:
        choices = [SEAT_OPTIONS.index(s) for s in plan["seats"]]
        game.launch(plan["map"], plan["human"], [i for i in choices if i != 4], seats=choices)
        validate_launch_seats(game, plan["seats"], physical_seats(game))
        return
    game.step(0, 2)
    game.tap(KEY_A)
    require(game.front_field("page") == FRONT_MISSION, "HOME did not open campaign")
    page, row = divmod(plan["ordinal"], 2)
    for _ in range(page):
        game.tap(KEY_RIGHT)
    require((game.front_field("mission_page"), game.front_field("row")) == (page, 0),
            "campaign page/row changed")
    if row:
        game.tap(KEY_DOWN)
    game.tap(KEY_A)
    require((game.front_field("page"), game.front_field("mission")) ==
            (FRONT_BRIEFING, plan["mission"]), "unexpected campaign briefing")
    game.tap(KEY_START)
    require(game.u32("screen") == SCREEN_PLAY, "campaign did not enter play")
    # Only confirm an actual authored modal, never advance the live world with A.
    if "sc64_text_box_missions" in game.sym:
        for _ in range(8):
            if not int.from_bytes(game.rsp.read(0x06013fe0, 2), "little"):
                break
            game.tap(KEY_A)
        require(not int.from_bytes(game.rsp.read(0x06013fe0, 2), "little"),
                "opening campaign dialogue exceeded its bound")


def observed(game, plan, screen):
    require(game.u32("screen") == screen and not game.u32("won") and not game.u32("lost"),
            f"unexpected screen or result (needed {screen})")
    state = game.state()
    require(state["units"] and state["buildings"], "candidate has an empty world")
    require((state["race_mode"], state["race_human"]) == (plan["mode"], plan["human"]),
            "unexpected game mode or human faction")
    state["race_selected_mission"] = game.u32("race_selected_mission")
    state["race_completed"] = game.u32("race_completed")
    if plan["mode"]:
        require(state["race_skirmish_map"] == plan["map"], "unexpected skirmish map")
        state["seats"] = physical_seats(game)
        owners = (0, 1, 3, 4)
        expected = [plan["human"], *(RACES.index(r) for r in state["seats"]["races"])]
        require([game.u32("race_player", 4 * owner) for owner in owners[:len(expected)]] == expected,
                "resolved races differ from the active controller factions")
    else:
        require(state["race_selected_mission"] == plan["mission"],
                "unexpected campaign mission")
        require(game.sizes.get("race_campaign") == 24, "unexpected campaign state layout")
        state["campaign_state"] = game.rsp.read(game.sym["race_campaign"], 24).hex()
    result = summary(state)
    if plan["mode"]:
        result["seats"] = state["seats"]
    return result


def same_world(old, new):
    return old["frame"] == new["frame"] and old["fingerprint"] == new["fingerprint"]


def pause_save(game, plan):
    game.tap(KEY_START)
    require(game.u32("screen") == SCREEN_PAUSE, "START did not pause")
    game.step(KEY_B)
    saved = observed(game, plan, SCREEN_PAUSE)
    game.rsp.export_sram()
    return saved


def advance_from_pause(game, plan, frame):
    game.step(KEY_A)
    game.step(0, CONTINUE)
    current = observed(game, plan, SCREEN_PLAY)
    require(current["frame"] == frame + CONTINUE, "neutral continuation did not advance exactly 60 frames")
    return current


def guard_output(path):
    require(shutil.disk_usage(path).free >= RESERVE, "physical gate needs 5 GiB free")
    require(sum(p.stat().st_size for p in path.rglob("*") if p.is_file()) <= OUTPUT_LIMIT,
            "physical gate output exceeded its bound")


def reporter(out, plan, pins):
    report = {"plan": plan, "pins": pins, "status": "incomplete"}

    def checkpoint(key, value):
        report[key] = value
        guard_output(out)
        (out / "result.json").write_text(json.dumps(report, indent=2) + "\n")

    return report, checkpoint


def run(args, plan, pins):
    out = args.output
    first, second = out / "first.sav", out / "second.sav"
    blank = out / "blank.sav"
    report, checkpoint = reporter(out, plan, pins)
    blank.write_bytes(b"\xff" * SRAM)
    with open_game(args, out / "initial", blank, first) as game:
        report["initial_inputs"] = game.inputs
        launch(game, plan)
        initial = observed(game, plan, SCREEN_PLAY)
        checkpoint("start", initial)
        game.step(0, plan["idle"])
        ready = observed(game, plan, SCREEN_PLAY)
        require(ready["frame"] == initial["frame"] + plan["idle"], "save cut did not advance")
        saved = pause_save(game, plan)
        require(same_world(ready, saved), "pause/save changed the game")
        checkpoint("saved", saved)
        uninterrupted = advance_from_pause(game, plan, saved["frame"])
        checkpoint("uninterrupted", uninterrupted)
    raw = first.read_bytes()
    slot(raw, 0, plan["tag"], 1)
    require(blank.read_bytes() == b"\xff" * SRAM, "the virgin SRAM input changed")
    with open_game(args, out / "fresh", first, second) as game:
        report["fresh_inputs"] = game.inputs
        game.step(0, 2)
        game.step(KEY_L)
        restored = observed(game, plan, SCREEN_PLAY)
        require(same_world(saved, restored), "fresh title L did not restore the saved world")
        checkpoint("restored", restored)
        # Release L as START rises; there is no idle play update before re-save.
        require(same_world(restored, pause_save(game, plan)), "re-save changed the game")
        checkpoint("journal", verify_journal(raw, second.read_bytes(), plan["tag"]))
        resumed = advance_from_pause(game, plan, saved["frame"])
        require(same_world(uninterrupted, resumed), "fresh continuation and uninterrupted play differ")
        checkpoint("resumed", resumed)
        if plan["mode"]:
            game.tap(KEY_START)
            require(game.u32("screen") == SCREEN_PAUSE, "restart pause failed")
            game.tap(KEY_R)
            restarted = observed(game, plan, SCREEN_PLAY)
            require(restarted["frame"] <= 2 and restarted["seats"] == initial["seats"],
                    "restart changed resolved seats/starts or failed to reset the frame")
            validate_launch_seats(game, plan["seats"], restarted["seats"])
            checkpoint("restarted", restarted)
    require(first.read_bytes() == raw, "fresh process changed its input SRAM file")
    same_inputs(pins)
    checkpoint("status", "passed")


def run_migration(args, plan, pins):
    out = args.output
    old, new, export = [out / f"{name}.sav" for name in ("stock", "converted", "observed")]
    blank = out / "blank.sav"
    report, checkpoint = reporter(out, plan, pins)
    erased = b"\xff" * SRAM
    blank.write_bytes(erased)
    with open_game(args, out / "stock", blank, old, stock=True) as game:
        report["stock_inputs"] = game.inputs
        launch(game, plan)
        start = observed(game, plan, SCREEN_PLAY)
        checkpoint("stock_launch", start)
        game.step(0, plan["idle"])
        ready = observed(game, plan, SCREEN_PLAY)
        require(ready["frame"] == start["frame"] + plan["idle"], "stock save cut did not advance")
        saved = pause_save(game, plan)
        require(same_world(saved, ready), "stock pause and save changed the world")
        checkpoint("stock_saved", saved)
    raw = old.read_bytes()
    slot(raw, 0, plan["predecessor_tag"], 1)
    require(raw[SLOT:] == erased[:SLOT] and blank.read_bytes() == erased,
            "stock predecessor is not a fresh single-bank save")
    with open_game(args, out / "converting", old, new) as game:
        report["converting_inputs"] = game.inputs
        game.step(0, 2)
        game.step(KEY_L)
        restored = observed(game, plan, SCREEN_PLAY)
        require(same_world(saved, restored), "candidate did not restore the stock custom world")
        checkpoint("loaded_stock", restored)
        # Deliberately make the two valid banks describe different game frames.
        # A later loader that silently falls back to the predecessor must fail.
        game.step(0, CONTINUE)
        advanced = observed(game, plan, SCREEN_PLAY)
        require(advanced["frame"] == saved["frame"] + CONTINUE, "converted world did not advance")
        migrated = pause_save(game, plan)
        require(same_world(advanced, migrated), "conversion save changed the world")
        checkpoint("saved_compact", migrated)
    packed = new.read_bytes()
    checkpoint("journal", verify_migration_journal(raw, packed, plan["predecessor_tag"], plan["tag"]))
    with open_game(args, out / "fresh", new, export) as game:
        report["fresh_inputs"] = game.inputs
        game.step(0, 2)
        game.step(KEY_L)
        final = observed(game, plan, SCREEN_PLAY)
        require(same_world(final, migrated) and not same_world(final, saved),
                "fresh load did not select the newer compact world")
        game.rsp.export_sram()
        checkpoint("fresh_compact", final)
    require(old.read_bytes() == raw and new.read_bytes() == packed and export.read_bytes() == packed,
            "physical load changed a stock or conversion SRAM image")
    same_inputs(pins)
    checkpoint("status", "passed")


def timeout(_number, _frame):
    raise TimeoutError("physical gate deadline or request to stop")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--profile", choices=PRODUCTS, required=True)
    p.add_argument("--case", choices=CASES, required=True)
    p.add_argument("--chapter", type=int, help="campaign chapter 1–10 (default: 10)")
    p.add_argument("--stock-rom", type=Path, help="pinned regular product ROM for migration")
    p.add_argument("--stock-elf", type=Path, help="matching regular ELF for migration")
    for name in ("rom", "elf", "runner", "output"):
        p.add_argument("--" + name, type=Path, required=name != "output")
    p.add_argument("--plan", action="store_true", help="validate pins/catalog; never launch or lock")
    p.add_argument("--deadline", type=int, default=180)
    args = p.parse_args()
    try:
        plan, pins = pin_inputs(args)
        if args.plan:
            print(json.dumps({"plan": plan, "pins": pins, "steps_per_boot": MAX_STEPS,
                              "neutral_continuation": CONTINUE}, indent=2))
            return 0
        require(30 <= args.deadline <= 240, "deadline must be 30–240 seconds")
        require(args.output is not None and not args.output.exists() and
                args.output.parent.is_dir(), "use a new output directory with an existing parent")
        require(shutil.disk_usage(args.output.parent).free >= RESERVE,
                "physical gate needs 5 GiB free")
        with open(LOCK, "a") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            args.output.mkdir()
            resource.setrlimit(resource.RLIMIT_FSIZE, (FILE_LIMIT, FILE_LIMIT))
            handlers = {sig: signal.signal(sig, timeout) for sig in (signal.SIGALRM, signal.SIGTERM)}
            signal.alarm(args.deadline)
            try:
                (run_migration if plan["case"] == "migration" else run)(args, plan, pins)
            finally:
                signal.alarm(0)
                for sig, handler in handlers.items():
                    signal.signal(sig, handler)
        return 0
    except (ContinuationError, OSError, ValueError, TimeoutError) as exc:
        print(f"Carrier physical gate: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
