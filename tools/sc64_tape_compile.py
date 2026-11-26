#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile a parity scenario description into an ares capture tape.

A scenario is a small JSON file: where the run starts (power-on through a
known menu route, or a state file), the camera there, and a program of
semantic steps (select a unit at a world point, order it to move or attack a
point, wait, dismiss text boxes, mark the command). The compiler writes the
line-per-command tape that experiments/ares-headless/capture.cpp runs
(format in experiments/ares-headless/CAPTURE.md). It never runs the
emulator: every input that depends on game state is a closed-loop `until`
or `untilw` line over guest RAM the S6 spike proved (docs/audits/
2026-11-26-parity-scenario-spikes.md), so the same tape reaches the same
point however long each approach takes, and a tape that cannot reach it
stops (exit 9) instead of pressing in the wrong place.

  sc64_tape_compile.py SCENARIO.json [-o TAPE]       compile one scenario
  sc64_tape_compile.py --check SCENARIO.json         validate only

Guest fields (KSEG0, big-endian), all from S6:

  cursor x, y    u16 0x800AFDCC / 0x800AFDCE, screen units 640 x 480
  camera x, y    s32 0x800A2EE8 / 0x800A2EEC, world px of the top-left corner
  world point    camera + cursor (`untilw x|y` compares it, signed)
  selection      12 x u32 at 0x800C0C00, row pointers from slot 0
  pause          u16 0x800F0C20, 1 while a text box or the pause menu is up

N64 controls the compiler relies on (S4, S6, the unit-lab exploration):
A over a unit selects it; holding A while the stick moves drags a box; with
units selected, C-Right orders a move to the point under the cursor, B an
attack on the unit or point under it, C-Left hold and C-Down stop. Z with the
stick pans the camera (40 px in x and 48 px in y per change, 2–3 steps of
lag) while the cursor stays put.

How a world point is reached:

  1. if the point may be off screen (the compiler tracks the camera exactly
     until the first pan and within PAN_SLACK after one), the cursor is
     centred and the camera panned with Z, fixed side first: `untilw` past a
     point PAN_LEAD short of the target at full stick, then onto it at the
     slow stick, so the direction never depends on where the camera was;
  2. the cursor approaches on each axis from a fixed side: `untilw` to
     APPROACH short of the target at full stick, then onto it slowly;
  3. after SETTLE idle steps two one-step `untilw` checks per axis assert the
     world point is within REACH of the target, so camera drift or a missed
     approach ends the run before any press.

Presses are held PRESS_HOLD (>= 3) steps and followed by PRESS_GAP released
steps (S6: a 1-step press is lost 40% of the time). Text boxes are modal:
`boxes N` waits for each to open and pulses Start until the pause halfword
clears; every order is preceded by a one-step `guard` that the pause
halfword is clear. `"mark": NAME` on an order puts `# mark NAME` right
before the press that issues it, which tools/sc64_parity_metrics.
tape_commands() resolves to the executed step through commands.csv.

Scenario coordinates come from captures or censuses of the cartridge, so
scenario files and compiled tapes are private (keep them under the fixture
home); this compiler and its tests are asset-free.
"""
import argparse
import hashlib
import json
import pathlib
import sys

FORMAT = 'sc64-tape-scenario/1'

CURSOR_X, CURSOR_Y = 0x800AFDCC, 0x800AFDCE
CAMERA_X, CAMERA_Y = 0x800A2EE8, 0x800A2EEC
SELECTION = 0x800C0C00
PAUSE = 0x800F0C20
SEED = 0x800EFF20

FIELDS = {
    'cursor_x': (CURSOR_X, 2), 'cursor_y': (CURSOR_Y, 2),
    'camera_x': (CAMERA_X, 4), 'camera_y': (CAMERA_Y, 4),
    'selection': (SELECTION, 4), 'pause': (PAUSE, 2), 'seed': (SEED, 4),
}
OPS = ('eq', 'ne', 'ge', 'lt', 'changed', 'set', 'clear')
WORLD_OPS = ('eq', 'ne', 'ge', 'lt')
OPTIONAL_COMMANDS = {'poke', 'pulse'}   # field-map frontend only (see Tape.require)

# Pad bits (CAPTURE.md).
BUTTONS = {'a': 1, 'start': 2, 'down': 4, 'c_left': 8, 'right': 16, 'r': 32, 'c_right': 64, 'c_up': 128,
           'c_down': 256, 'b': 512, 'z': 1024, 'l': 2048, 'up': 4096, 'left': 8192}

PRESS_HOLD = 4          # steps a press is held (S6: 3 never missed in 700 presses)
PRESS_GAP = 8           # released steps after a press
FAST, SLOW = 16000, 10000   # stick: about 5.4 and 1.9 cursor units per step (S6)
APPROACH = 48           # world px short of the target where the cursor's fast leg stops
PAN_LEAD = 160          # world px short of the target where the pan's fast leg stops
PAN_SLACK = 160         # how far a pan may leave the target from the centred cursor
EDGE_SLACK = 16         # how far past the centre the centred cursor may sit (< one 40/48-px pan step)
BOX_AFTER_PAN = 64      # largest box side after a pan: the start corner is within PAN_SLACK of centre
PAN_EDGE_STEP = 48      # one pan step: the camera's uncertainty where an edge stops it
REACH = 12              # world px the final point may miss the target by
SCREEN = (640, 480)     # cursor units across the screen
CENTRE = (320, 240)
CURSOR_SAFE = (96, 544, 96, 384)   # x0, x1, y0, y1: cursor window clear of the scrolling edge
CAMERA_MIN = -32
STEP_MAX = 600          # one cursor leg
PAN_MAX = 2000          # one pan leg
SETTLE = 12             # idle steps before the position checks
BOX_OPEN_MAX = 20000    # steps to wait for an expected text box
BOX_PULSE = (3, 20)     # Start pressed / released while dismissing (S6)
BOX_CLEAR_MAX = 3000

# Orders: the wheel button that issues each at the point under the cursor.
ORDERS = {
    'move': ('c_right', True),
    'attack': ('b', True),
    'hold': ('c_left', False),
    'stop': ('c_down', False),
}

# Menu routes from power-on, as the S4 tapes reach them (t1-move, bb-income).
BOOT_TITLE = [(700, 0), (10, 'start'), (290, 0), (10, 'start'), (290, 0), (10, 'start'), (290, 0), (10, 'start'),
              (1000, 0), (1000, 0), (290, 0)]
BOOT_ROUTES = {
    # Episode tab: Down, Down, A opens Wasteland's briefing; two Starts enter the mission.
    't1': BOOT_TITLE + [(10, 'down'), (90, 0), (10, 'down'), (90, 0), (10, 'a'), (890, 0), (10, 'start'), (590, 0),
                        (10, 'start'), (10, 0)],
}
# Any episode row, through the owner-approved guest edit (2026-11-26) the field-map
# session uses: after the title the progress bytes at 0x800D13C4 open every row of
# episodes I-III; the episode carousel turns left once per episode after the first;
# Start opens it, Down per row, A picks; Start is pulsed through the location card and
# briefing until the map mode word 0x800A5AB4 reads 3. Runs that poke are n64-staged.
PROGRESS = 0x800D13C4
PROGRESS_ALL = '0C0A0A'
MAP_MODE = 0x800A5AB4
EPISODES = {'terran': 0, 'zerg': 1, 'protoss': 2}


def episode_route(race, mission):
    if race not in EPISODES or not isinstance(mission, int) or not 1 <= mission <= 10:
        raise ScenarioError(f'unknown episode mission {race!r} {mission!r}')
    k = EPISODES[race]
    row = mission + 1 if race == 'terran' else mission - 1
    route = [(700, 0), (10, 'start'), (290, 0), (10, 'start'), (290, 0), (10, 'start'), (390, 0),
             ('poke', PROGRESS, PROGRESS_ALL)]
    route += [(12, 0, -31000, 0), (150, 0)] * k
    route += [(100, 0), (10, 'start'), (300, 0)]
    if k:
        route += [(10, 'start'), (300, 0)]
    route += [(1700, 0)] + [(10, 'down'), (90, 0)] * row + [(10, 'a'), (200, 0)]
    route += [('until', MAP_MODE, 2, 'eq', 3, 12000, 'start', (10, 290))]
    return route


LOBBY_HUMAN = {'terran': 0, 'protoss': 1, 'random': 2, 'zerg': 3}      # Rights from Terran
LOBBY_COMPUTER = {'zerg': 0, 'terran': 1, 'protoss': 2, 'random': 3}   # Rights from Zerg
BLOOD_BATH_RIGHTS = 11


class ScenarioError(ValueError):
    pass


def hex_addr(address):
    return f'{address:08X}'


def blood_bath_route(human, computer):
    """Scenario tab, Blood Bath, the human and computer races; the lobby's A is the caller's (S4 bb-income)."""
    if human not in LOBBY_HUMAN or computer not in LOBBY_COMPUTER:
        raise ScenarioError(f'unknown lobby races {human!r}, {computer!r}')
    route = BOOT_TITLE + [(10, 'right'), (90, 0), (10, 'a'), (300, 0), (10, 'down'), (60, 0)]
    route += [(10, 'right'), (50, 0)] * BLOOD_BATH_RIGHTS
    route += [(10, 'down'), (60, 0)] + [(10, 'right'), (50, 0)] * LOBBY_HUMAN[human]
    route += [(10, 'down'), (60, 0)] + [(10, 'right'), (50, 0)] * LOBBY_COMPUTER[computer]
    return route + [(10, 0)]


def mask_of(button):
    if isinstance(button, int) and not isinstance(button, bool):
        if button < 0 or button >= 1 << 14:
            raise ScenarioError(f'button mask {button} out of range')
        return button
    if not isinstance(button, str) or not button:
        raise ScenarioError(f'bad button {button!r}')
    total = 0
    for name in button.split('+'):
        if name not in BUTTONS:
            raise ScenarioError(f'unknown button {name!r}')
        total |= BUTTONS[name]
    return total


def integer(value, what, lo=None, hi=None):
    if not isinstance(value, int) or isinstance(value, bool):
        raise ScenarioError(f'{what} must be an integer')
    if (lo is not None and value < lo) or (hi is not None and value > hi):
        raise ScenarioError(f'{what} {value} is outside [{lo}, {hi}]')
    return value


def point(value, what):
    if not isinstance(value, (list, tuple)) or len(value) != 2:
        raise ScenarioError(f'{what} must be [x, y]')
    return [integer(v, what, 0, 65535) for v in value]


def stick_of(value):
    if not isinstance(value, (list, tuple)) or len(value) != 2:
        raise ScenarioError('stick must be [x, y]')
    return [integer(v, 'stick', -32767, 32767) for v in value]


def field_of(name):
    if name in FIELDS:
        return FIELDS[name]
    if isinstance(name, str) and ':' in name:
        address, width = name.split(':', 1)
        try:
            address, width = int(address, 16), int(width)
        except ValueError:
            raise ScenarioError(f'bad field {name!r}') from None
        if width not in (1, 2, 4) or address % width or not 0x80000000 <= address <= 0x80800000 - width:
            raise ScenarioError(f'bad field {name!r}')
        return address, width
    raise ScenarioError(f'unknown field {name!r}')


def name_ok(name, suffix=''):
    return (isinstance(name, str) and 0 < len(name) <= 64 and name[0] != '.' and name.endswith(suffix) and
            len(name) > len(suffix) and all(c.isalnum() or c in '._-' for c in name))


class Tape:
    """Accumulates tape lines and tracks the camera per axis: exact at the start and after a pan
    to a map edge, otherwise within PAN_SLACK."""

    def __init__(self, camera, world):
        self.lines = []
        self.camera = list(camera) if camera is not None else None
        self.exact = [camera is not None] * 2
        self.world = world
        self.pending_mark = None
        self.staged = False
        self.extra_commands = set()   # frontend commands the scenario declared (frontend_commands)

    def emit(self, line):
        self.lines.append(line)

    def step_line(self, line):
        """A step-advancing command; a pending mark lands right before it."""
        if self.pending_mark is not None:
            self.emit(f'# mark {self.pending_mark}')
            self.pending_mark = None
        self.emit(line)

    def pad(self, steps, mask=0, x=0, y=0):
        if steps < 1:
            raise ScenarioError('a pad line needs at least one step')
        self.step_line(f'{steps} {mask} {x} {y}')

    def until(self, address, width, op, value, limit, mask=0, x=0, y=0, pulse=None):
        if op not in OPS:
            raise ScenarioError(f'unknown until op {op!r}')
        value &= (1 << (8 * width)) - 1
        line = f'until {hex_addr(address)} {width} {op} {value:x} {limit} {mask} {x} {y}'
        if pulse:
            line += f' {pulse[0]} {pulse[1]}'
        self.step_line(line)

    def untilw(self, axis, op, value, limit, mask=0, x=0, y=0):
        if op not in WORLD_OPS or axis not in ('x', 'y'):
            raise ScenarioError(f'bad untilw {axis} {op}')
        self.step_line(f'untilw {axis} {op} {value} {limit} {mask} {x} {y}')

    def mark(self, name):
        if not isinstance(name, str) or not name or not all(c.isalnum() or c in '_-' for c in name):
            raise ScenarioError(f'bad mark name {name!r}')
        if self.pending_mark is not None:
            raise ScenarioError(f'mark {name} follows mark {self.pending_mark} with no command between')
        self.pending_mark = name

    def press(self, button, mark=None):
        mask = mask_of(button)
        if mark is not None:
            self.mark(mark)
        self.pad(PRESS_HOLD, mask)
        self.pad(PRESS_GAP)

    def poke(self, address, data):
        """A guest write (the field-map frontend's `poke ADDR HEX`): staged evidence only."""
        if (not isinstance(data, str) or not data or len(data) % 2 or len(data) > 512 or
                any(c not in '0123456789abcdefABCDEF' for c in data)):
            raise ScenarioError(f'poke data {data!r} must be 1-256 bytes of hex')
        if not 0x80000000 <= address <= 0x80800000 - len(data) // 2:
            raise ScenarioError(f'poke address {address:#x} is outside RDRAM')
        self.require('poke')
        self.staged = True
        self.emit(f'poke {hex_addr(address)} {data.upper()}')

    def require(self, command):
        """`poke` writes guest memory and `pulse` came with it (the field-map frontend): a
        scenario opts in to each by listing it in frontend_commands, so neither appears by accident."""
        if command not in self.extra_commands:
            raise ScenarioError(f'`{command}` needs an explicit opt-in: list it in "frontend_commands"')

    def guard(self):
        """One idle step that must end with no text box or menu open."""
        self.until(PAUSE, 2, 'eq', 0, 1)

    # ---- camera and cursor -------------------------------------------------

    def screen_of(self, wx, wy):
        return wx - self.camera[0], wy - self.camera[1]

    def visible(self, wx, wy):
        """Can the cursor reach (wx, wy) inside the safe window without a pan?"""
        if self.camera is None:
            return False
        sx, sy = self.screen_of(wx, wy)
        x0, x1, y0, y1 = CURSOR_SAFE
        sx_slack, sy_slack = (0 if exact else PAN_SLACK for exact in self.exact)
        return x0 + sx_slack <= sx <= x1 - sx_slack and y0 + sy_slack <= sy <= y1 - sy_slack

    def reach_limits(self, axis):
        """World points the centred cursor can cover by panning on this axis."""
        lo = CAMERA_MIN + CENTRE[axis]
        hi = None
        if self.world is not None:
            hi = self.world[axis] - SCREEN[axis] + CENTRE[axis] - CAMERA_MIN
        return lo, hi

    def approach(self, axis, target, fast, slow, lead, limit, mask=0):
        """Fixed-side approach of the world point on one axis: fast to `lead` short, then slowly onto it."""
        name = 'xy'[axis]
        stick = (lambda v: (v, 0)) if axis == 0 else (lambda v: (0, v))
        lo, hi = self.reach_limits(axis) if mask & BUTTONS['z'] else (None, None)
        if lo is None or target - lead > lo:
            self.untilw(name, 'lt', target - lead, limit, mask, *stick(-fast))
            self.untilw(name, 'ge', target, limit, mask, *stick(slow))
        else:
            if hi is not None and target + lead > hi:
                raise ScenarioError(f'world {name}={target} is too close to both map edges to pan onto')
            self.untilw(name, 'ge', target + lead, limit, mask, *stick(fast))
            self.untilw(name, 'lt', target + 1, limit, mask, *stick(-slow))

    def centre_cursor(self):
        """The cursor to the screen centre (screen units: the cursor fields alone)."""
        for address, centre, axis in ((CURSOR_X, CENTRE[0], 0), (CURSOR_Y, CENTRE[1], 1)):
            stick = (lambda v: (v, 0)) if axis == 0 else (lambda v: (0, v))
            self.until(address, 2, 'lt', centre - APPROACH, STEP_MAX, 0, *stick(-FAST))
            self.until(address, 2, 'ge', centre, STEP_MAX, 0, *stick(SLOW))

    def pan_to(self, wx, wy):
        """Pan with Z so the point is reachable. When the point lies within the safe window of a
        camera stopped at a map edge, the pan runs into that edge, which pins the axis exactly;
        otherwise it puts the point under the centred cursor, within PAN_SLACK."""
        plan = []
        for axis, value in ((0, wx), (1, wy)):
            lo, hi = self.reach_limits(axis)
            x0, x1 = CURSOR_SAFE[2 * axis], CURSOR_SAFE[2 * axis + 1]
            slack = 0 if self.exact[axis] else PAN_SLACK
            if x0 + slack <= value - self.camera[axis] <= x1 - slack:
                plan.append(('keep', None, self.camera[axis]))   # this axis is already safe
            elif x0 <= value - CAMERA_MIN <= x1:
                plan.append(('low', lo, CAMERA_MIN))
            elif hi is not None and x0 <= value - (hi - CENTRE[axis]) <= x1:
                plan.append(('high', hi, hi - CENTRE[axis]))
            elif (value - PAN_LEAD > lo or hi is None or value + PAN_LEAD < hi) and \
                    lo + PAN_EDGE_STEP <= value and (hi is None or value <= hi - PAN_EDGE_STEP):
                plan.append(('aim', value, value - CENTRE[axis]))
            else:
                raise ScenarioError(f'world {"xy"[axis]}={value} cannot be reached (pan range {lo}..{hi})')
        self.centre_cursor()
        z = BUTTONS['z']
        for axis, (kind, target, camera) in enumerate(plan):
            stick = (lambda v: (v, 0)) if axis == 0 else (lambda v: (0, v))
            # The centred cursor sits a few units past the centre, so the edge test allows
            # EDGE_SLACK, less than one pan step: it can only be met with the camera at its limit.
            if kind == 'keep':
                continue
            if kind == 'low':
                self.untilw('xy'[axis], 'lt', target + EDGE_SLACK + 1, PAN_MAX, z, *stick(-FAST))
            elif kind == 'high':
                self.untilw('xy'[axis], 'ge', target - EDGE_SLACK, PAN_MAX, z, *stick(FAST))
            else:
                self.approach(axis, target, FAST, SLOW, PAN_LEAD, PAN_MAX, z)
            self.camera[axis] = camera
            self.exact[axis] = kind != 'aim'
        self.pad(SETTLE)

    def check_point(self, wx, wy):
        """Two one-step checks per axis: the world point under the cursor is within REACH."""
        self.untilw('x', 'ge', wx - REACH, 1)
        self.untilw('x', 'lt', wx + REACH + 1, 1)
        self.untilw('y', 'ge', wy - REACH, 1)
        self.untilw('y', 'lt', wy + REACH + 1, 1)

    def cursor_to(self, wx, wy, mask=0):
        """Put the cursor over world point (wx, wy), panning first when it may be off the safe window."""
        if self.camera is None:
            raise ScenarioError('a cursor move needs the starting camera: give "camera"')
        if mask:
            pass   # a drag never pans: the box op checked its corner against the screen
        elif not self.visible(wx, wy):
            self.pan_to(wx, wy)
        self.approach(0, wx, FAST, SLOW, APPROACH, STEP_MAX, mask)
        self.approach(1, wy, FAST, SLOW, APPROACH, STEP_MAX, mask)
        if not mask:
            self.pad(SETTLE)
            self.check_point(wx, wy)


def compile_scenario(doc):
    if not isinstance(doc, dict) or doc.get('format') != FORMAT:
        raise ScenarioError(f'not a {FORMAT} scenario')
    start = doc.get('start')
    if not isinstance(start, dict):
        raise ScenarioError('start must be an object')
    camera = doc.get('camera')
    if camera is not None:
        if not isinstance(camera, list) or len(camera) != 2:
            raise ScenarioError('camera must be [x, y]')
        camera = [integer(v, 'camera', CAMERA_MIN, 65535) for v in camera]
    world = doc.get('map')
    if world is not None:
        world = point(world, 'map')
        if world[0] < SCREEN[0] or world[1] < SCREEN[1]:
            raise ScenarioError('map is smaller than the screen')
    tape = Tape(camera, world)
    extra = doc.get('frontend_commands', [])
    if not isinstance(extra, list) or not all(isinstance(c, str) and c in OPTIONAL_COMMANDS for c in extra):
        raise ScenarioError(f'frontend_commands may list only {sorted(OPTIONAL_COMMANDS)}')
    tape.extra_commands = set(extra)
    ident = doc.get('id', 'unnamed')
    if not name_ok(ident):
        raise ScenarioError('id must be one plain token ([A-Za-z0-9._-], at most 64 characters)')
    head = [f'# sc64_tape_compile {FORMAT} scenario {ident}']
    if 'load' in start:
        if not name_ok(start['load'], '.state'):
            raise ScenarioError('start.load needs NAME.state')
        tape.emit(f'load {start["load"]} {integer(start.get("step"), "start.step", 0, 0xFFFFFFFF)}')
        if start.get('staged'):
            # The state came from a run that wrote guest memory: the lab inherits the label.
            tape.staged = True
    elif 'boot' in start:
        boot = start['boot']
        if isinstance(boot, str) and boot in BOOT_ROUTES:
            route = BOOT_ROUTES[boot]
        elif isinstance(boot, dict) and boot.get('lobby') == 'blood-bath':
            route = blood_bath_route(boot.get('human'), boot.get('computer'))
        elif isinstance(boot, dict) and 'episode' in boot:
            route = episode_route(boot['episode'], boot.get('mission'))
        else:
            raise ScenarioError(f'unknown boot route {boot!r}')
        for item in route:
            if item[0] == 'poke':
                tape.poke(item[1], item[2])
            elif item[0] == 'until':
                _, address, width, op, value, limit, button, pulse = item
                tape.until(address, width, op, value, limit, mask_of(button), 0, 0, pulse)
            elif len(item) == 4:
                tape.pad(item[0], mask_of(item[1]) if item[1] else 0, item[2], item[3])
            else:
                tape.pad(item[0], mask_of(item[1]) if item[1] else 0)
    else:
        raise ScenarioError('start needs load or boot')
    if start.get('watch', True):
        for address, width in ((CURSOR_X, 2), (CURSOR_Y, 2), (CAMERA_X, 4), (CAMERA_Y, 4), (SELECTION, 4), (PAUSE, 2)):
            tape.emit(f'watch {hex_addr(address)} {width}')
    program = doc.get('program', [])
    if not isinstance(program, list):
        raise ScenarioError('program must be a list')
    for index, step in enumerate(program):
        if not isinstance(step, dict):
            raise ScenarioError(f'program[{index}] must be an object')
        try:
            compile_step(tape, step)
        except ScenarioError as error:
            raise ScenarioError(f'program[{index}] {step.get("op", "?")}: {error}') from None
    if tape.pending_mark is not None:
        raise ScenarioError(f'mark {tape.pending_mark} has no command after it')
    tape.emit('quit')
    if tape.staged:
        head.append('# n64-staged: this tape, or the run that wrote its start state, writes guest memory (poke)')
    return '\n'.join(head + tape.lines) + '\n'


def compile_step(tape, step):
    op = step.get('op')
    if op == 'boxes':
        for _ in range(integer(step.get('count', 1), 'count', 1, 64)):
            tape.until(PAUSE, 2, 'eq', 1, integer(step.get('max', BOX_OPEN_MAX), 'max', 1, 10000000))
            tape.until(PAUSE, 2, 'eq', 0, BOX_CLEAR_MAX, BUTTONS['start'], 0, 0, BOX_PULSE)
        tape.pad(integer(step.get('settle', 30), 'settle', 1, 100000))
    elif op == 'guard':
        tape.guard()
    elif op == 'tap':
        tape.emit('tap tick rec')
    elif op == 'wait' and step.get('dismiss'):
        # Mission text boxes come at unknown times: Start is pulsed only while the pause
        # halfword is set (the field-map frontend's `pulse`), for exactly STEPS steps.
        steps = integer(step.get('steps'), 'steps', 1, 10000000)
        tape.require('pulse')
        tape.step_line(f'pulse {hex_addr(PAUSE)} 2 ne 0 {steps} {BUTTONS["start"]} {BOX_PULSE[0]} 60')
    elif op == 'wait':
        steps = integer(step.get('steps'), 'steps', 1, 10000000)
        while steps:
            chunk = min(steps, 1000)
            tape.pad(chunk)
            steps -= chunk
    elif op == 'cursor':
        tape.cursor_to(*point(step.get('at'), 'at'))
    elif op == 'select':
        tape.cursor_to(*point(step.get('at'), 'at'))
        tape.press('a')
        if 'row' in step:
            # The selected row itself (its KSEG0 address in the unit table): a miss that left an
            # earlier selection in place stops the run instead of ordering the wrong unit.
            row, _ = field_of(f'{step["row"]}:4')
            tape.until(SELECTION, 4, 'eq', row, 30)
            tape.until(SELECTION + 4, 4, 'eq', 0, 1)   # and nothing else: no stale group
        else:
            tape.until(SELECTION, 4, 'ne', 0, 30)
    elif op == 'box':
        a, b = point(step.get('from'), 'from'), point(step.get('to'), 'to')
        tape.cursor_to(*a)
        # The drag stays on one screen: with an exact camera the far corner must be in the safe
        # window; after a pan (camera known within PAN_SLACK) the box is kept small instead.
        if all(tape.exact) and not tape.visible(*b):
            raise ScenarioError(f'box corner {b} is off the safe window')
        if not all(tape.exact) and (abs(b[0] - a[0]) > BOX_AFTER_PAN or abs(b[1] - a[1]) > BOX_AFTER_PAN):
            raise ScenarioError(f'a box drawn after a pan must stay within {BOX_AFTER_PAN} px')
        # The cursor settles within REACH of the first corner; a longer side keeps it on the
        # first corner's side of the far one, so each drag leg runs the intended way.
        if a == b or any(0 < abs(e - s0) <= 2 * REACH for s0, e in zip(a, b)):
            raise ScenarioError(f'a box needs two corners, each moving side longer than {2 * REACH} px')
        tape.pad(PRESS_HOLD, BUTTONS['a'])
        # The cursor stands on the first corner, so each axis of the drag has a known
        # direction: one slow leg per axis, never past the far corner's side.
        for axis, (start, end) in enumerate(zip(a, b)):
            stick = (lambda v: (v, 0)) if axis == 0 else (lambda v: (0, v))
            if end > start:
                tape.untilw('xy'[axis], 'ge', end, STEP_MAX, BUTTONS['a'], *stick(SLOW))
            elif end < start:
                tape.untilw('xy'[axis], 'lt', end + 1, STEP_MAX, BUTTONS['a'], *stick(-SLOW))
        tape.pad(PRESS_GAP)
        tape.until(SELECTION, 4, 'ne', 0, 30)
    elif op == 'deselect':
        tape.press('b')
    elif op == 'order':
        kind = step.get('kind')
        if kind not in ORDERS:
            raise ScenarioError(f'unknown order {kind!r}; known: {", ".join(ORDERS)}')
        button, targeted = ORDERS[kind]
        if targeted:
            tape.cursor_to(*point(step.get('at'), 'at'))
        elif 'at' in step:
            raise ScenarioError(f'{kind} takes no target point')
        tape.guard()
        tape.press(button, mark=step.get('mark'))
    elif op == 'press':
        tape.press(step.get('button'), mark=step.get('mark'))
    elif op == 'hold':
        stick = stick_of(step.get('stick', [0, 0]))
        tape.pad(integer(step.get('steps'), 'steps', 1, 10000000), mask_of(step.get('button', 0)), *stick)
    elif op == 'mark':
        tape.mark(step.get('name'))
    elif op == 'until':
        address, width = field_of(step.get('field'))
        if step.get('cmp') not in OPS:
            raise ScenarioError(f'until needs cmp in {OPS}')
        stick = stick_of(step.get('stick', [0, 0]))
        pulse = step.get('pulse')
        if pulse is not None:
            if not isinstance(pulse, list) or len(pulse) != 2:
                raise ScenarioError('pulse must be [hold, gap]')
            pulse = [integer(v, 'pulse', 1, 1000) for v in pulse]
        tape.until(address, width, step['cmp'], integer(step.get('value', 0), 'value', 0, (1 << 8 * width) - 1),
                   integer(step.get('max', 1000), 'max', 1, 10000000), mask_of(step.get('button', 0)), *stick, pulse)
    elif op == 'save':
        if not name_ok(step.get('name'), '.state'):
            raise ScenarioError('save needs NAME.state')
        tape.emit(f'save {step["name"]}')
    elif op == 'shot':
        if not name_ok(step.get('name')):
            raise ScenarioError('shot needs a plain name')
        tape.emit(f'shot {step["name"]}')
        tape.pad(2)
    elif op == 'poke':
        address, _ = field_of(f'{step.get("address", "")}:1')
        tape.poke(address, step.get('data'))
    elif op == 'peek':
        address, width = field_of(step.get('field'))
        tape.emit(f'peek {hex_addr(address)} {width}')
    else:
        raise ScenarioError(f'unknown op {op!r}')


def compile_file(path):
    raw = pathlib.Path(path).read_bytes()
    text = compile_scenario(json.loads(raw))
    first, rest = text.split('\n', 1)
    return f'{first}\n# scenario sha256 {hashlib.sha256(raw).hexdigest()}\n{rest}'


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('scenario', type=pathlib.Path)
    parser.add_argument('-o', '--output', type=pathlib.Path)
    parser.add_argument('--check', action='store_true', help='validate only')
    args = parser.parse_args(argv)
    try:
        text = compile_file(args.scenario)
    except (ScenarioError, json.JSONDecodeError, UnicodeDecodeError) as error:
        print(f'{args.scenario}: {error}', file=sys.stderr)
        return 2
    if args.check:
        return 0
    if args.output:
        try:
            with open(args.output, 'x') as handle:   # compiled tapes are never overwritten
                handle.write(text)
        except FileExistsError:
            print(f'{args.output}: exists; compiled tapes are never overwritten', file=sys.stderr)
            return 2
    else:
        sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
