"""Orchestras compiled before csoundStart: each one's global code runs once,
at the start, in the order they were compiled - as if they were one. A
compilation after the first replaced instr 0 before anything ran it, so the
first one's global code was lost, and the last one's ran twice."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound


HEADER = '''
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
'''


def mark(n):
    """Global code that appends the digit n to the "order" channel."""
    return f'chnset(chnget:i("order") * 10 + {n}, "order")\n'


class TestCompileBeforeStart(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)

    def tearDown(self):
        self.cs.reset()

    def compile(self, *orchestras):
        for orchestra in orchestras:
            self.assertEqual(self.cs.compile_orc(orchestra), ctcsound.CSOUND_SUCCESS)

    def perform(self, *events):
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        for event in events:
            self.cs.event_string(event)
        for _ in range(2):
            self.cs.perform_ksmps()

    def test_each_compilation_s_global_code_runs_once_in_order(self):
        self.compile(HEADER + mark(1), mark(2), mark(3))
        self.assertEqual(self.cs.control_channel("order")[0], 0)
        self.perform()
        self.assertEqual(self.cs.control_channel("order")[0], 123)

    def test_a_later_compilation_reads_the_globals_of_an_earlier_one(self):
        self.compile(HEADER + '''
            given_value@global:i = 5
            given_table@global:i = ftgen(0, 0, 8, 7, 0, 8, 1)
        ''', '''
            instr Read
                chnset(given_value, "value")
                chnset(ftlen(given_table), "length")
            endin
        ''')
        self.perform('i "Read" 0 0')
        self.assertEqual(self.cs.control_channel("value")[0], 5)
        self.assertEqual(self.cs.control_channel("length")[0], 8)

    def test_eval_code_before_the_start_runs_what_came_before_it_and_once(self):
        self.compile(HEADER + mark(1))
        self.assertEqual(self.cs.eval_code(mark(2) + 'return 7\n'), 7)
        self.assertEqual(self.cs.control_channel("order")[0], 12)
        self.perform()
        self.assertEqual(self.cs.control_channel("order")[0], 12)

    def test_a_compilation_after_the_start_runs_at_once(self):
        self.compile(HEADER + mark(1))
        self.perform()
        self.compile(mark(2))
        self.assertEqual(self.cs.control_channel("order")[0], 12)


if __name__ == '__main__':
    unittest.main()
