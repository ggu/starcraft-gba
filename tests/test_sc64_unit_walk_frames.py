"""Asset-free guards for reading source IScript Walking loops."""
import struct
import unittest

from tools.sc64_unit_walk_frames import Unsupported, walk_cycle


def playfram(frame):
    return bytes([0]) + struct.pack('<H', frame)


def wait(ticks):
    return bytes([5, ticks])


def move(pixels):
    return bytes([41, pixels])


def goto(target):
    return bytes([7]) + struct.pack('<H', target)


def script(*parts, loop_at=0):
    """Assemble parts at offset 0; the last part is a goto to loop_at."""
    return b''.join(parts) + goto(loop_at)


class WalkCycleTests(unittest.TestCase):
    def test_wait_before_frame_opens_on_previous_lap_pose(self):
        # Zergling shape: each lap moves and waits before its next playfram.
        payload = script(move(2), wait(1), playfram(102), move(8), wait(1), playfram(119),
                         move(9), wait(1), playfram(85))
        steps, ticks = walk_cycle(payload, 0)
        self.assertEqual(ticks, 3)
        self.assertEqual(steps, [(0, 85), (1, 102), (2, 119)])

    def test_frame_after_last_wait_wraps_to_tick_zero(self):
        payload = script(playfram(17), wait(2), playfram(34), wait(1), playfram(0))
        steps, ticks = walk_cycle(payload, 0)
        self.assertEqual(ticks, 3)
        # playfram 0 lands on the next lap's tick 0, then playfram 17 replaces it.
        self.assertEqual(steps, [(0, 17), (2, 34)])

    def test_setup_before_loop_label_is_not_cycled(self):
        loop = len(playfram(204))
        payload = playfram(204) + script(wait(2), playfram(0), wait(2), playfram(17),
                                         loop_at=loop)
        steps, ticks = walk_cycle(payload, 0)
        self.assertEqual(ticks, 4)
        self.assertEqual(steps, [(0, 17), (2, 0)])

    def test_single_pose_and_idle_loops_are_rejected(self):
        with self.assertRaisesRegex(Unsupported, 'one pose'):
            walk_cycle(script(playfram(0), wait(1), playfram(0), wait(1)), 0)
        with self.assertRaisesRegex(Unsupported, 'no frame'):
            walk_cycle(script(wait(125)), 0)
        with self.assertRaisesRegex(Unsupported, 'no wait'):
            walk_cycle(script(playfram(0), playfram(17)), 0)

    def test_random_waits_and_endings_are_rejected(self):
        with self.assertRaisesRegex(Unsupported, 'waitrand'):
            walk_cycle(script(playfram(0), bytes([6, 3, 6])), 0)
        with self.assertRaisesRegex(Unsupported, 'without looping'):
            walk_cycle(playfram(0) + wait(1) + bytes([22]), 0)


if __name__ == '__main__':
    unittest.main()
