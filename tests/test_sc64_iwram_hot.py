# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks for tools/gba_iwram_hot.py and common/gba_hot.ld."""
import contextlib
import io
from pathlib import Path
import re
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gba_iwram_hot as hot  # noqa: E402


def ci(frames, calls):
    """.ci text: frames {title: bytes}, calls [(caller, callee)]."""
    lines = ['graph: { title: "file.c"']
    lines += [f'node: {{ title: "{name}" label: "{hot.symbol_of(name)}\\nfile.c:1:1\\n{size} bytes (static)" }}'
              for name, size in frames.items()]
    lines += [f'edge: {{ sourcename: "{a}" targetname: "{b}" label: "file.c:2:2" }}' for a, b in calls]
    return "\n".join(lines + ["}"]) + "\n"


def document(*records):
    return "\n".join(['graph: { title: "file.c"', *records, "}"]) + "\n"


def bound(frames, calls, taken=(), root="main", **options):
    frames, edges, sites = hot.parse_callgraph([ci(frames, calls)])
    return hot.stack_bound(frames, edges, set(taken), root, sites=sites, **options)


def cycles(result, key):
    return [(c["kind"], c["functions"]) for c in result[key]]


def symtab(rows):
    """`readelf -s -W` text from (address, size, type, bind, name) rows."""
    head = "   Num:    Value  Size Type    Bind   Vis      Ndx Name\n"
    return head + "".join(f"{i:6d}: {addr:08x} {size:5d} {kind:7s} {bind:6s} DEFAULT    1 {name}\n"
                          for i, (addr, size, kind, bind, name) in enumerate(rows, 1))


class ConfigTest(unittest.TestCase):
    def test_parses_limit_names_cycles_and_comments(self):
        config = hot.parse_config("# note\nlimit 0x03005dc0\n\nrecursion destroy* 2 0123abcd\n"
                                  "callback-cycle dispatch 1 89abcdef\nmemset\nfoo_bar*  # clones\n")
        self.assertEqual(config["limit"], 0x03005DC0)
        self.assertEqual(config["names"], ["memset", "foo_bar*"])
        self.assertEqual(config["recursion"], {"destroy*": (2, "0123abcd")})
        self.assertEqual(config["callback-cycle"], {"dispatch": (1, "89abcdef")})

    def test_rejects_bad_configs(self):
        for text in ("memset\n",                               # no limit
                     "limit 0x03005dc0\nlimit 0x03005dc0\n",   # two limits
                     "limit 0x03005dc2\n",                     # not a word address
                     "limit 0x03007e04\n",                     # above the stack top
                     "limit 0x02000000\n",                     # not IWRAM
                     "limit 0x03005dc0\nmemset\nmemset\n",     # listed twice
                     "limit 0x03005dc0\n*(.text*)\n",          # not a function name
                     "limit 0x03005dc0\na b\n",
                     "limit 0x03005dc0\nrecursion f 0 0123abcd\n",   # a cycle is live at least once
                     "limit 0x03005dc0\nrecursion f -1 0123abcd\n",
                     "limit 0x03005dc0\nrecursion f 2\n",            # no fingerprint
                     "limit 0x03005dc0\nrecursion f 2 XYZ\n",
                     "limit 0x03005dc0\nrecursion f 2 0123abcd\nrecursion f 3 0123abcd\n"):
            with self.assertRaises(hot.HotError, msg=text):
                hot.parse_config(text)

    def test_fragments_place_each_section_and_define_the_limit(self):
        files = hot.fragments(0x03005DC0, ["memset", "foo*"])
        self.assertIn("*(.text.memset)\n*(.text.foo)\n*(.text.foo.*)\n", files[hot.SECTIONS_NAME])
        self.assertIn("__iwram_hot_limit = 0x03005dc0;", files[hot.LIMIT_NAME])

    def test_every_run_rewrites_both_fragments_limit_last(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp, "list.txt")
            config.write_text("limit 0x03005dc0\nmemset\n")
            out = Path(tmp, "out")
            hot.write_fragments(config, out)
            (out / hot.LIMIT_NAME).unlink()
            config.write_text("limit 0x03005d80\nmemset\n")
            hot.write_fragments(config, out)
            self.assertEqual(sorted(p.name for p in out.iterdir()), sorted([hot.LIMIT_NAME, hot.SECTIONS_NAME]))
            self.assertIn("0x03005d80", (out / hot.LIMIT_NAME).read_text())
            self.assertGreaterEqual((out / hot.LIMIT_NAME).stat().st_mtime_ns,
                                    (out / hot.SECTIONS_NAME).stat().st_mtime_ns)

    def test_checked_in_lists_parse_and_declare_their_cycles(self):
        lists = sorted((ROOT / "config" / "iwram-hot").glob("*.txt"))
        self.assertTrue(lists)
        for path in lists:
            config = hot.parse_config(path.read_text())
            self.assertTrue(config["names"], path)
            self.assertLess(config["limit"], hot.STACK_TOP, path)
            self.assertTrue(config["recursion"] and config["callback-cycle"], path)


class PatternTest(unittest.TestCase):
    def test_star_takes_compiler_clones_only(self):
        self.assertTrue(hot.matches("mask", "mask"))
        self.assertTrue(hot.matches("mask", "mask*"))
        self.assertTrue(hot.matches("mask.isra.0", "mask*"))
        self.assertTrue(hot.matches("mask.constprop.0", "mask*"))
        self.assertFalse(hot.matches("mask_reference", "mask*"))
        self.assertFalse(hot.matches("mask.isra.0", "mask"))
        self.assertFalse(hot.matches("callback", "call*"))


class BoundTest(unittest.TestCase):
    def test_longest_direct_chain(self):
        result = bound({"main": 96, "update": 100, "draw": 400, "leaf": 40},
                       [("main", "update"), ("main", "draw"), ("draw", "leaf")])
        self.assertEqual(result["bytes"], 96 + 400 + 40 + hot.HELPER_BYTES)
        self.assertEqual([step["functions"][0]["name"] for step in result["chain"]], ["main", "draw", "leaf"])
        self.assertEqual(result["undeclared"], [])

    def test_titles_map_to_symbols_and_units(self):
        self.assertEqual(hot.symbol_of("starcraft/game.c:foo.constprop.0"), "foo.constprop.0")
        self.assertEqual(hot.unit_of("starcraft/game.c:foo.constprop.0"), "game.c")
        self.assertEqual(hot.symbol_of("main"), "main")
        self.assertIsNone(hot.unit_of("main"))

    def test_pointer_call_reaches_every_taken_function(self):
        frames = {"main": 10, "dispatch": 20, "small": 30, "big": 500, "unused": 9000}
        calls = [("main", "dispatch"), ("dispatch", hot.INDIRECT)]
        self.assertEqual(bound(frames, calls, taken={"small", "big"})["bytes"], 10 + 20 + 500 + hot.HELPER_BYTES)
        # A function nobody calls and whose address nobody takes is unreachable.
        self.assertEqual(bound(frames, calls)["bytes"], 10 + 20 + hot.HELPER_BYTES)

    def test_new_plain_member_in_a_callback_cycle_changes_its_fingerprint(self):
        frames = {"main": 10, "dispatch": 100, "callback": 200, "helper": 50, "extra": 30}
        calls = [("main", "dispatch"), ("dispatch", hot.INDIRECT), ("callback", "helper"), ("helper", hot.INDIRECT)]
        seen = bound(frames, calls, taken={"callback"})["cycles"][0]["fingerprint"]
        longer = [("main", "dispatch"), ("dispatch", hot.INDIRECT), ("callback", "extra"), ("extra", "helper"),
                  ("helper", hot.INDIRECT)]
        result = bound(frames, longer, taken={"callback"}, callback_cycles={"callback": (1, seen)})
        self.assertEqual(len(result["changed"]), 1)

    def test_callback_cycle_must_be_declared(self):
        # save -> (pointer) -> callback -> validate -> (pointer) -> callback ...
        frames = {"main": 10, "save": 100, "callback": 200, "validate": 50, "tail": 7}
        calls = [("main", "save"), ("save", hot.INDIRECT), ("callback", "validate"),
                 ("validate", hot.INDIRECT), ("validate", "tail")]
        result = bound(frames, calls, taken={"callback"})
        self.assertEqual(cycles(result, "undeclared"), [("callback-cycle", ["callback", "validate"])])
        once = bound(frames, calls, taken={"callback"}, callback_cycles={"validate": 1})
        self.assertEqual(once["undeclared"], [])
        self.assertEqual(once["bytes"], 10 + 100 + (200 + 50) + 7 + hot.HELPER_BYTES)
        twice = bound(frames, calls, taken={"callback"}, callback_cycles={"callback*": 2})
        self.assertEqual(twice["bytes"], 10 + 100 + 2 * (200 + 50) + 7 + hot.HELPER_BYTES)

    def test_recursion_must_be_declared_and_multiplies_the_cycle(self):
        frames = {"main": 10, "destroy.isra.0": 40, "detach": 24, "leaf": 8}
        calls = [("main", "destroy.isra.0"), ("destroy.isra.0", "detach"), ("detach", "destroy.isra.0"),
                 ("detach", "leaf")]
        result = bound(frames, calls)
        self.assertEqual(cycles(result, "undeclared"), [("recursion", ["destroy.isra.0", "detach"])])
        twice = bound(frames, calls, recursion={"destroy*": 2})
        self.assertEqual(twice["undeclared"], [])
        self.assertEqual(twice["bytes"], 10 + 2 * (40 + 24) + 8 + hot.HELPER_BYTES)
        self.assertEqual(bound(frames, calls, recursion={"detach": 3})["bytes"],
                         10 + 3 * (40 + 24) + 8 + hot.HELPER_BYTES)
        # A recursion declaration does not cover it as a callback cycle of another kind, and vice versa.
        self.assertTrue(bound(frames, calls, callback_cycles={"detach": 1})["undeclared"])

    def test_declared_cycle_that_changed_must_be_read_again(self):
        frames = {"main": 10, "save": 100, "callback": 200, "validate": 50}
        calls = [("main", "save"), ("save", hot.INDIRECT), ("callback", "validate"), ("validate", hot.INDIRECT)]
        seen = bound(frames, calls, taken={"callback"})["cycles"][0]["fingerprint"]
        ok = bound(frames, calls, taken={"callback"}, callback_cycles={"callback": (1, seen)})
        self.assertEqual((ok["undeclared"], ok["changed"]), ([], []))
        # A member gains a pointer call: the same declaration no longer covers it.
        more = calls + [("callback", hot.INDIRECT)]
        result = bound(frames, more, taken={"callback"}, callback_cycles={"callback": (1, seen)})
        self.assertEqual(cycles(result, "changed"), [("callback-cycle", ["callback", "validate"])])
        # So does a second pointer-call site in a member that already had one.
        frames2, edges2, sites2 = hot.parse_callgraph([ci(frames, calls)])
        sites2 = dict(sites2, validate=sites2["validate"] + 1)
        result = hot.stack_bound(frames2, edges2, {"callback"}, "main", sites=sites2,
                                 callback_cycles={"callback": (1, seen)})
        self.assertEqual(len(result["changed"]), 1)
        # And another address-taken function joining the cycle.
        joined = dict(frames, other=5)
        result = bound(joined, calls + [("other", "validate")], taken={"callback", "other"},
                       callback_cycles={"callback": (1, seen)})
        self.assertEqual(len(result["changed"]), 1)

    def test_direct_call_added_inside_a_callback_cycle_changes_its_fingerprint(self):
        frames = {"main": 10, "dispatch": 100, "callback": 200, "helper": 50}
        calls = [("main", "dispatch"), ("dispatch", hot.INDIRECT), ("callback", "helper"),
                 ("helper", hot.INDIRECT)]
        seen = bound(frames, calls, taken={"callback"})["cycles"][0]["fingerprint"]
        self.assertEqual(bound(frames, calls, taken={"callback"}, callback_cycles={"callback": (1, seen)})["changed"], [])
        # helper now calls callback directly: same members, same pointer sites, new way to nest.
        result = bound(frames, calls + [("helper", "callback")], taken={"callback"},
                       recursion={"callback": 2}, callback_cycles={"callback": (1, seen)})
        self.assertEqual(len(result["changed"]), 1)

    def test_fingerprint_ignores_compiler_clone_suffixes(self):
        a = hot.cycle_fingerprint("recursion", ["game.c:destroy.isra.0", "game.c:detach"], {}, set())
        b = hot.cycle_fingerprint("recursion", ["game.c:destroy", "game.c:detach.constprop.0"], {}, set())
        c = hot.cycle_fingerprint("recursion", ["game.c:destroy", "game.c:other"], {}, set())
        self.assertEqual(a, b)
        self.assertNotEqual(a, c)
        self.assertRegex(a, hot.FINGERPRINT)

    def test_new_call_inside_a_recursion_group_changes_its_fingerprint(self):
        frames = {"main": 10, "destroy": 40, "detach": 24}
        calls = [("main", "destroy"), ("destroy", "detach"), ("detach", "destroy")]
        seen = bound(frames, calls)["cycles"][0]["fingerprint"]
        self.assertEqual(bound(frames, calls, recursion={"destroy": (2, seen)})["changed"], [])
        # A self-call keeps the members but can nest without limit.
        result = bound(frames, calls + [("destroy", "destroy")], recursion={"destroy": (2, seen)})
        self.assertEqual(cycles(result, "changed"), [("recursion", ["destroy", "detach"])])

    def test_pointer_call_back_into_a_recursion_group_needs_its_own_declaration(self):
        frames = {"main": 10, "a": 40, "b": 24}
        calls = [("main", "a"), ("a", "b"), ("b", "a")]
        plain = bound(frames, calls, taken={"b"}, recursion={"a": 2})
        self.assertEqual(plain["undeclared"], [])       # address taken, but nothing calls through a pointer
        looped = bound(frames, calls + [("b", hot.INDIRECT)], taken={"b"}, recursion={"a": 2})
        self.assertEqual(cycles(looped, "undeclared"), [("callback-cycle", ["a", "b"])])
        both = bound(frames, calls + [("b", hot.INDIRECT)], taken={"b"}, recursion={"a": 2},
                     callback_cycles={"b": 3})
        self.assertEqual((both["undeclared"], both["bytes"]), ([], 10 + 2 * 3 * (40 + 24) + hot.HELPER_BYTES))

    def test_self_recursion_is_recursion(self):
        calls = [("main", "walk"), ("walk", "walk")]
        self.assertTrue(bound({"main": 10, "walk": 16}, calls)["undeclared"])
        self.assertEqual(bound({"main": 10, "walk": 16}, calls, recursion={"walk": 4})["bytes"],
                         10 + 4 * 16 + hot.HELPER_BYTES)

    def test_recursion_inside_a_callback_cycle_multiplies_both(self):
        frames = {"main": 1, "a": 10, "b": 20, "c": 30}
        calls = [("main", "a"), ("a", "b"), ("b", "a"), ("b", hot.INDIRECT), ("c", "a")]
        result = bound(frames, calls, taken={"c"}, recursion={"a": 2}, callback_cycles={"c": 3})
        self.assertEqual(result["undeclared"], [])
        self.assertEqual(result["bytes"], 1 + 3 * (2 * 10 + 2 * 20 + 30) + hot.HELPER_BYTES)

    def test_cycles_the_root_cannot_reach_need_no_declaration(self):
        frames = {"main": 10, "leaf": 5, "x": 100, "y": 100}
        calls = [("main", "leaf"), ("x", "y"), ("y", "x")]
        result = bound(frames, calls)
        self.assertEqual((result["bytes"], result["undeclared"]), (15 + hot.HELPER_BYTES, []))

    def test_callee_without_a_frame_needs_a_known_cost(self):
        with self.assertRaises(hot.HotError):
            bound({"main": 10}, [("main", "mystery")])
        result = bound({"main": 10}, [("main", "__aeabi_idiv")],
                       frameless=lambda name: hot.LIBGCC_BYTES if hot.LIBGCC.match(name) else None)
        self.assertEqual(result["bytes"], 10 + hot.LIBGCC_BYTES + hot.HELPER_BYTES)

    def test_libgcc_and_veneer_names(self):
        for name in ("__aeabi_uldivmod", "__aeabi_idiv", "__udivmoddi4", "__divsi3", "__clzdi2", "__ctzsi2",
                     "__muldi3", "__gnu_thumb1_case_uhi"):
            self.assertTrue(hot.LIBGCC.match(name), name)
        for name in ("__sc64_private", "race_tc_prepare", "__aeabi", "memcpy"):
            self.assertFalse(hot.LIBGCC.match(name), name)
        for name in ("__memset_veneer", "__tick_combat_from_arm", "____aeabi_idiv_from_thumb"):
            self.assertTrue(hot.VENEER.match(name), name)
        self.assertFalse(hot.VENEER.match("memset"))

    def test_pointer_call_sites_are_counted_per_function(self):
        text = document(
            'edge: { sourcename: "a" targetname: "__indirect_call" label: "f.c:10:3" }',
            'edge: { sourcename: "a" targetname: "__indirect_call" label: "f.c:12:3" }',
            'edge: { sourcename: "a" targetname: "__indirect_call" label: "f.c:12:3" }',
            'edge: { sourcename: "b" targetname: "a" label: "f.c:20:1" }',
            'edge: { sourcename: "c" targetname: "__aeabi_idiv" }')
        _, edges, sites = hot.parse_callgraph([text])
        self.assertEqual(sites, {"a": 2})
        self.assertEqual(edges["c"], {"__aeabi_idiv"})

    def test_dynamic_frames_are_refused(self):
        text = document('node: { title: "f" label: "f\\nfile.c:1:1\\n64 bytes (dynamic,bounded)" }')
        with self.assertRaises(hot.HotError):
            hot.parse_callgraph([text])

    def test_truncated_or_unparsed_call_graph_is_refused(self):
        whole = ci({"main": 96, "draw": 400}, [("main", "draw")])
        self.assertEqual(hot.parse_callgraph([whole])[0], {"main": 96, "draw": 400})
        lines = whole.splitlines()
        for broken in ("\n".join(lines[:-1]) + "\n",             # no closing brace
                       "\n".join(lines[:-2]) + "\n",             # cut before the last edge
                       "\n".join(lines[:-2] + [lines[-2][:30]]),  # cut inside a record
                       "\n".join(lines[1:]) + "\n",              # no graph header
                       "\n".join(lines[:1] + ["garbage"] + lines[1:]) + "\n",
                       ""):
            with self.assertRaises(hot.HotError, msg=broken):
                hot.parse_callgraph([broken])

    def test_declarations_do_not_hide_the_defining_frame(self):
        frames, _, _ = hot.parse_callgraph([document(
            'node: { title: "game_update" label: "game_update\\ncommon/gba.h:1:1" shape : ellipse }',
            'node: { title: "game_update" label: "game_update\\ngame.c:9:1\\n72 bytes (static)" }')])
        self.assertEqual(frames, {"game_update": 72})

    def test_deep_chains_do_not_hit_the_recursion_limit(self):
        frames = {f"f{i}": 4 for i in range(5000)}
        calls = [(f"f{i}", f"f{i + 1}") for i in range(4999)]
        self.assertEqual(bound(frames, calls, root="f0")["bytes"], 5000 * 4 + hot.HELPER_BYTES)


class DumpTest(unittest.TestCase):
    def test_address_taken_functions_only(self):
        dump = """Initial Symbol table:

race_tc_prepare/28710 (race_tc_prepare)
  Type: function definition analyzed
  Visibility: semantic_interposition
  Address is taken.
  References: scratch/9160 (addr)
  Referring: race_worker_cargo_select/28711 (addr)
update_orders/100 (update_orders)
  Type: function definition analyzed
  Visibility: semantic_interposition
  Referring:
  Called by: game_update/7
table/9160 (table)
  Type: variable definition analyzed
  Address is taken.
foo.constprop.0/5 (foo.constprop)
  Type: function definition analyzed
  Address is taken.
"""
        taken, defined = hot.parse_address_taken(dump.splitlines(keepends=True))
        self.assertEqual(taken, {"race_tc_prepare", "foo.constprop.0"})
        self.assertEqual(defined, {"race_tc_prepare", "update_orders", "foo.constprop.0"})

    def test_symbols_keep_every_binding_and_skip_objects(self):
        values, functions, files = hot.parse_symbols(symtab([
            (0x08000101, 40, "FUNC", "LOCAL", "static_fn"),
            (0x08000201, 12, "FUNC", "WEAK", "weak_fn"),
            (0x08000301, 12, "FUNC", "GLOBAL", "global_fn"),
            (0x08000400, 64, "OBJECT", "LOCAL", "table"),
            (0x03005ae0, 0, "NOTYPE", "GLOBAL", "__iwram_end"),
            (0, 0, "FILE", "LOCAL", "game.c")]) +
            "     9: 00000000     0 FUNC    GLOBAL DEFAULT  UND missing\n")
        self.assertEqual(files, {"game.c"})
        self.assertEqual(values["__iwram_end"], 0x03005AE0)
        self.assertEqual(functions, [(0x08000100, 40, "static_fn"), (0x08000200, 12, "weak_fn"),
                                     (0x08000300, 12, "global_fn")])

    def test_frames_come_from_call_frame_information(self):
        listing = """Contents of the .debug_frame section:

00000000 0000000c ffffffff CIE "" cf=2 df=-4 ra=14
   LOC   CFA      
00000000 r13+0    

00000010 0000000c 00000000 FDE cie=00000000 pc=08005c48..08005c4a

00000020 00000028 00000000 FDE cie=00000000 pc=08005c4c..08005e84
   LOC   CFA      r4    r5    ra    
08005c4c r13+0    u     u     u     
08005c4e r13+20   c-20  c-16  c-4   
08005c58 r13+96   c-20  c-16  c-4   
08005c60 r13+36   c-20  c-16  c-4   
"""
        self.assertEqual(hot.parse_frames(listing), {0x08005C48: 0, 0x08005C4C: 96})
        with self.assertRaises(hot.HotError):       # a frame-pointer frame has no fixed bound here
            hot.parse_frames(listing + "08005c70 r7+8     c-20  c-16  c-4   \n")

    def test_branches_with_fixed_targets_are_found(self):
        listing = "\n".join([
            " 801594e:\tpush\t{r4, lr}",
            " 801595c:\tbl\t8015930 <sc64_direct_target_valid.isra.0>",
            " 8015962:\tbeq.n\t801596a <sc64_direct_target_can_damage+0x1e>",
            " 8015972:\tbx\tr3",
            " 3000010:\tb\t3000400 <gfx_rect>",
            " 8015974:\t.word\t0x03000400",
            " 8015978:\tbics\tr0, r1"])
        self.assertEqual(hot.parse_branches(listing),
                         [(0x801595C, 0x8015930), (0x8015962, 0x801596A), (0x3000010, 0x3000400)])

    def test_merged_function_resolves_to_the_frame_at_its_address(self):
        frames, edges, _ = hot.parse_callgraph([ci({"main": 10, "game.c:census_p8": 300},
                                                   [("main", "game.c:census_p10")])])
        _, functions, _ = hot.parse_symbols(symtab([(0x08000101, 80, "FUNC", "LOCAL", "census_p8"),
                                                 (0x08000101, 80, "FUNC", "LOCAL", "census_p10"),
                                                 (0x08000201, 20, "FUNC", "GLOBAL", "main")]))
        aliases = hot.resolve_aliases(frames, edges, functions)
        self.assertEqual(aliases, {"game.c:census_p10": "game.c:census_p8"})
        edges = {source: {aliases.get(t, t) for t in targets} for source, targets in edges.items()}
        self.assertEqual(hot.stack_bound(frames, edges, set(), "main")["bytes"], 310 + hot.HELPER_BYTES)


class CheckTest(unittest.TestCase):
    """The gate itself, on a made-up program: main -> draw -> leaf, with the
    toolchain replaced by canned readelf and objdump output."""
    MAIN, DRAW, LEAF, HOT = 0x08000200, 0x08000300, 0x08000400, 0x03004000

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        self.frames = {"main": 96, "file.c:draw": 400, "file.c:leaf": 40, "file.c:hot": 8}
        self.calls = [("main", "file.c:draw"), ("file.c:draw", "file.c:leaf"), ("main", "file.c:hot")]
        self.measured = {self.MAIN: 96, self.DRAW: 400, self.LEAF: 40, self.HOT: 8}
        self.end = 0x03004100
        self.limit = 0x03004200
        self.symbols = [(self.MAIN + 1, 64, "FUNC", "GLOBAL", "main"), (self.DRAW + 1, 64, "FUNC", "LOCAL", "draw"),
                        (self.LEAF + 1, 64, "FUNC", "LOCAL", "leaf"), (self.HOT + 1, 32, "FUNC", "LOCAL", "hot")]
        self.branches = [(self.MAIN + 8, self.DRAW), (self.DRAW + 8, self.LEAF)]
        self.config = "limit 0x03004200\nhot\n"
        self.cgraph = ("main/1 (main)\n  Type: function definition analyzed\n"
                       "draw/2 (draw)\n  Type: function definition analyzed\n"
                       "leaf/3 (leaf)\n  Type: function definition analyzed\n"
                       "hot/4 (hot)\n  Type: function definition analyzed\n")
        original = hot.tool_output
        self.addCleanup(setattr, hot, "tool_output", original)
        hot.tool_output = self.fake_tool

    def fake_tool(self, tool, *args):
        if tool == "readelf" and args[0] == "-s":
            rows = self.symbols + [(self.end, 0, "NOTYPE", "GLOBAL", "__iwram_end"),
                                   (self.HOT, 0, "NOTYPE", "GLOBAL", "__iwram_hot_start"),
                                   (self.limit, 0, "NOTYPE", "GLOBAL", "__iwram_hot_limit"),
                                   (0, 0, "FILE", "LOCAL", "file.c")]
            return symtab(rows)
        if tool == "readelf":
            return "".join(f"00000020 00000028 00000000 FDE cie=00000000 pc={start:08x}..{start + 64:08x}\n"
                           f"   LOC   CFA      ra\n{start:08x} r13+0    u\n{start + 2:08x} r13+{size}   c-4\n\n"
                           for start, size in self.measured.items())
        if tool == "objdump" and args[0] == "-h":
            return "  0 .text 00000164\n  1 .iwram 00006000\n  2 .rom 01e00000\n"
        if tool == "objdump" and ".rom" in args:
            return "".join(f" {a:x}:\tbl\t{t:x} <f>\n" for a, t in self.branches)
        return ""

    def run_check(self, margin=512):
        (self.dir / "file.ci").write_text(ci(self.frames, self.calls))
        (self.dir / "file.c.000i.cgraph").write_text(self.cgraph)
        (self.dir / "list.txt").write_text(self.config)
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return hot.main(["check", "--elf", str(self.dir / "game.elf"), "--dump-dir", str(self.dir), "--config",
                             str(self.dir / "list.txt"), "--margin", str(margin), "--json",
                             str(self.dir / "out.json")])

    def report(self):
        import json
        return json.loads((self.dir / "out.json").read_text())

    def test_a_program_that_fits_passes(self):
        self.assertEqual(self.run_check(), 0)
        report = self.report()
        self.assertEqual(report["stack_bound_bytes"], 96 + 400 + 40 + hot.HELPER_BYTES)
        self.assertEqual(report["highest_safe_iwram_end"], (hot.STACK_TOP - 544 - 512) & ~3)
        self.assertEqual((report["hot_functions"], report["errors"]), (1, []))

    def test_code_at_the_highest_safe_address_passes_and_one_word_higher_fails(self):
        highest = (hot.STACK_TOP - 544 - 512) & ~3
        self.end = self.limit = highest
        self.config = f"limit {highest:#010x}\nhot\n"
        self.assertEqual(self.run_check(), 0)
        self.end = self.limit = highest + 4
        self.config = f"limit {highest + 4:#010x}\nhot\n"
        self.assertEqual(self.run_check(), 1)
        self.assertIn("the stack may reach", self.report()["errors"][0])

    def test_the_margin_is_part_of_the_gate(self):
        self.end = self.limit = (hot.STACK_TOP - 544 - 512) & ~3
        self.config = f"limit {self.limit:#010x}\nhot\n"
        self.assertEqual(self.run_check(margin=516), 1)

    def test_linked_function_without_a_frame_record_is_refused(self):
        self.symbols.append((0x08000501, 16, "FUNC", "LOCAL", "stranger"))
        self.assertEqual(self.run_check(), 2)

    def test_frame_from_another_build_is_refused(self):
        self.measured[self.DRAW] = 800          # the linked code uses more than the dump says
        self.assertEqual(self.run_check(), 2)
        self.measured[self.DRAW] = 200          # or less
        self.assertEqual(self.run_check(), 2)

    def test_pushed_register_arguments_raise_the_frame(self):
        self.measured[self.LEAF] = 40 + hot.PRETEND_ARGS_BYTES
        self.assertEqual(self.run_check(), 0)
        report = self.report()
        self.assertEqual(report["stack_bound_bytes"], 96 + 400 + 56 + hot.HELPER_BYTES)
        self.assertEqual(report["frames_raised_from_elf"], 1)

    def test_call_missing_from_the_dump_is_taken_from_the_elf(self):
        self.calls = [("main", "file.c:hot")]   # the dump lost both calls on the deep chain
        self.assertEqual(self.run_check(), 0)
        report = self.report()
        self.assertEqual(report["stack_bound_bytes"], 96 + 400 + 40 + hot.HELPER_BYTES)
        self.assertEqual(report["edges_added_from_elf"], 2)

    def test_branch_from_a_merged_body_is_charged_to_the_function_with_the_frame(self):
        # The ELF lists the body under a twin's name first; the frame record is under "draw".
        self.symbols.insert(1, (self.DRAW + 1, 64, "FUNC", "LOCAL", "a_twin_of_draw"))
        self.calls = [("main", "file.c:draw"), ("main", "file.c:hot")]      # the dump lacks draw -> leaf
        self.assertEqual(self.run_check(), 0)
        self.assertEqual(self.report()["stack_bound_bytes"], 96 + 400 + 40 + hot.HELPER_BYTES)

    def test_linked_function_that_nothing_reaches_fails(self):
        # compiled, linked, has a frame, but only the reset code or a libgcc hook calls it
        self.frames["file.c:panic"] = 1200
        self.symbols.append((0x08000601, 64, "FUNC", "LOCAL", "panic"))
        self.measured[0x08000600] = 1200
        self.cgraph += "panic/5 (panic)\n  Type: function definition analyzed\n"
        self.assertEqual(self.run_check(), 1)
        self.assertIn("reached from no call the check can see (first: panic)", self.report()["errors"][0])

    def test_libgcc_hook_written_in_c_fails(self):
        self.frames["__aeabi_idiv0"] = 16
        self.calls.append(("file.c:leaf", "__aeabi_idiv0"))
        self.symbols.append((0x08000601, 64, "FUNC", "GLOBAL", "__aeabi_idiv0"))
        self.measured[0x08000600] = 16
        self.cgraph += "__aeabi_idiv0/5 (__aeabi_idiv0)\n  Type: function definition analyzed\n"
        self.assertEqual(self.run_check(), 1)
        self.assertTrue(any("replaces a libgcc routine" in e for e in self.report()["errors"]))

    def test_libgcc_hook_that_is_an_alias_of_a_program_function_fails(self):
        # `leaf` is also exported as __aeabi_ldiv0; the alias has no frame record of its own.
        self.symbols.append((self.LEAF + 1, 0, "FUNC", "GLOBAL", "__aeabi_ldiv0"))
        self.assertEqual(self.run_check(), 1)
        self.assertTrue(any("__aeabi_ldiv0 replaces a libgcc routine" in e for e in self.report()["errors"]))

    def test_discarded_callback_and_its_discarded_callee_do_not_fail_a_rom(self):
        self.frames.update({"file.c:dead_cb": 3000, "file.c:dead_leaf": 100})
        self.calls += [("file.c:dead_cb", "file.c:dead_leaf"), ("file.c:leaf", hot.INDIRECT)]
        self.cgraph += ("dead_cb/7 (dead_cb)\n  Type: function definition analyzed\n  Address is taken.\n"
                        "dead_leaf/8 (dead_leaf)\n  Type: function definition analyzed\n")
        self.assertEqual(self.run_check(), 0)
        self.assertEqual(self.report()["stack_bound_bytes"], 96 + 400 + 40 + hot.HELPER_BYTES)

    def test_branch_from_code_without_a_record_into_the_program_fails(self):
        self.symbols.append((0x08000701, 32, "FUNC", "GLOBAL", "__divsi3"))
        self.branches.append((0x08000708, self.LEAF))
        self.assertEqual(self.run_check(), 2)

    def test_address_taken_assembly_routine_without_a_size_fails(self):
        self.symbols.append((0x08000801, 0, "NOTYPE", "GLOBAL", "fast_fill"))
        self.cgraph += "fast_fill/9 (fast_fill)\n  Type: function\n  Address is taken.\n"
        self.assertEqual(self.run_check(), 2)

    def test_address_taken_name_that_is_not_linked_is_ignored(self):
        self.cgraph += "gone_callback/9 (gone_callback)\n  Type: function definition analyzed\n  Address is taken.\n"
        self.assertEqual(self.run_check(), 0)

    def test_weakref_with_its_address_taken_fails(self):
        self.cgraph += "wr/9 (wr)\n  Type: function weakref\n  Address is taken.\n"
        self.assertEqual(self.run_check(), 2)

    def test_direct_callee_replaced_at_link_time_fails(self):
        self.frames["file.c:wrapped"] = 8
        self.calls.append(("file.c:leaf", "file.c:wrapped"))
        self.cgraph += "wrapped/5 (wrapped)\n  Type: function definition analyzed\n"
        self.assertEqual(self.run_check(), 1)
        self.assertTrue(any("direct callees not in" in e for e in self.report()["errors"]))

    def test_declared_cycle_that_is_not_there_fails(self):
        self.config = "limit 0x03004200\nrecursion leaf 2 0123abcd\nhot\n"
        self.assertEqual(self.run_check(), 1)
        self.assertIn("matches no call cycle", self.report()["errors"][0])

    def test_undeclared_cycle_fails(self):
        self.calls.append(("file.c:leaf", "file.c:draw"))
        self.assertEqual(self.run_check(), 1)
        self.assertIn("undeclared call cycle", self.report()["errors"][0])

    def test_limit_other_than_the_linked_one_fails(self):
        self.config = "limit 0x030041fc\nhot\n"
        self.assertEqual(self.run_check(), 1)
        self.assertIn("relink", self.report()["errors"][0])

    def test_dump_that_omits_a_compiled_function_is_refused(self):
        self.cgraph = self.cgraph.replace("leaf/3 (leaf)\n  Type: function definition analyzed\n", "")
        self.assertEqual(self.run_check(), 2)

    def test_address_taken_function_is_a_pointer_target(self):
        self.frames["file.c:deep"] = 2000
        self.symbols.append((0x08000601, 64, "FUNC", "LOCAL", "deep"))
        self.measured[0x08000600] = 2000
        self.cgraph += "deep/5 (deep)\n  Type: function definition analyzed\n  Address is taken.\n"
        self.calls.append(("file.c:leaf", hot.INDIRECT))
        self.assertEqual(self.run_check(), 0)
        self.assertEqual(self.report()["stack_bound_bytes"], 96 + 400 + 40 + 2000 + hot.HELPER_BYTES)

    def test_unlisted_function_in_the_hot_range_is_reported(self):
        self.config = "limit 0x03004200\nother\n"
        self.assertEqual(self.run_check(), 0)
        warnings = self.report()["warnings"]
        self.assertTrue(any("hot is in the hot IWRAM range" in w for w in warnings))
        self.assertTrue(any("other is listed but is not in this ROM" in w for w in warnings))

    def test_missing_dumps_or_config_is_a_usage_failure(self):
        (self.dir / "list.txt").write_text(self.config)
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(hot.main(["check", "--elf", str(self.dir / "game.elf"), "--dump-dir", str(self.dir),
                                       "--config", str(self.dir / "list.txt")]), 2)
            self.assertEqual(hot.main(["check", "--elf", str(self.dir / "game.elf"), "--dump-dir", str(self.dir),
                                       "--config", str(self.dir / "absent.txt")]), 2)


class LinkerScriptTest(unittest.TestCase):
    """gba_hot.ld must stay gba.ld plus the hot section list."""

    def setUp(self):
        self.base = (ROOT / "common" / "gba.ld").read_text()
        self.hot = (ROOT / "common" / "gba_hot.ld").read_text()

    def test_memory_regions_match(self):
        memory = re.compile(r"MEMORY \{.*?\n\}", re.S)
        self.assertEqual(memory.search(self.base).group(0), memory.search(self.hot).group(0))

    def test_every_input_pattern_and_symbol_is_kept(self):
        patterns = set(re.findall(r"\*\([^)]*\)", self.base))
        self.assertEqual(patterns - set(re.findall(r"\*\([^)]*\)", self.hot)), set())
        symbols = set(re.findall(r"\b(__\w+) =", self.base))
        self.assertEqual(symbols - set(re.findall(r"\b(__\w+) =", self.hot)), set())
        self.assertIn("ASSERT(__bss_end < 0x0203e000", self.hot)

    def test_hot_sections_come_before_the_rom_text(self):
        self.assertLess(self.hot.index(f"INCLUDE {hot.SECTIONS_NAME}"), self.hot.index("*(.text*)"))
        self.assertLess(self.hot.index("KEEP(*(.gbaheader))"), self.hot.index("__iwram_start"))
        self.assertIn(f"INCLUDE {hot.LIMIT_NAME}", self.hot)

    def test_every_interrupt_handler_in_the_sources_is_bounded(self):
        stored = set()
        for path in [*(ROOT / "common").glob("*.[ch]"), *(ROOT / "starcraft").glob("*.[ch]")]:
            text = path.read_text(errors="replace")
            stored.update(re.findall(r"0x03007ffc\s*=\s*\(u32\)\s*(\w+)", text, flags=re.I))
            self.assertEqual(len(re.findall(r"0x03007ffc\s*=", text, flags=re.I)),
                             len(re.findall(r"0x03007ffc\s*=\s*\(u32\)\s*\w+", text, flags=re.I)), path)
        self.assertEqual(stored, set(hot.IRQ_HANDLERS))

    def test_inline_assembly_leaves_the_stack_and_control_flow_alone(self):
        """Stack moved or calls made inside an asm statement have no frame or
        call-graph record, so the stack bound cannot see them."""
        blocks = 0
        for path in [*(ROOT / "common").glob("*.[ch]"), *(ROOT / "starcraft").glob("*.[ch]")]:
            text = path.read_text(errors="replace")
            self.assertNotRegex(text, r"__attribute__\s*\(\(\s*naked", path)
            for match in re.finditer(r"\b(?:__asm__|asm)\s*(?:volatile\s*)?\(", text):
                depth, end = 1, match.end()
                while depth and end < len(text):
                    depth += {"(": 1, ")": -1}.get(text[end], 0)
                    end += 1
                body = " ".join(re.findall(r'"((?:[^"\\]|\\.)*)"', text[match.end():end]))
                if not body.strip():
                    continue        # an empty barrier such as asm("" ::: "memory")
                blocks += 1
                # No stack, link or program-counter register, no push/pop, no call, no software interrupt.
                self.assertNotRegex(
                    body, r"(?i)\b(sp|r13|lr|r14|pc|r15)\b|\b(push|pop|blx?|bx|swi|svc)\b|"
                          r"\bbl(eq|ne|cs|hs|cc|mi|pl|vs|vc|hi|ge|gt|al)\b", f"{path}: {body[:80]}")
        self.assertGreater(blocks, 0)

    def test_stack_pointers_match_the_reset_code(self):
        start = (ROOT / "common" / "start.s").read_text()
        self.assertEqual([int(v, 16) for v in re.findall(r"ldr sp, =(0x[0-9a-f]+)", start)],
                         [hot.IRQ_STACK_TOP, hot.STACK_TOP])


if __name__ == "__main__":
    unittest.main()
