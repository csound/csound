"""Score strings a host sends: csoundEventString reads its text with the score
preprocessor, where a brace in quotes was a repeat bracket and n or m and a
space began a mark's name - and through this call, a segfault."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound


STRINGS = ["/kit/tala/{1,2,3}/0/drive", "{kick,snare}", "a}b", "gain 1 bounded",
           "main 2nd=1", "drum 60"]


class TestScoreStringsFromAHost(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)

    def tearDown(self):
        self.cs.reset()

    def test_braces_and_n_or_m_arrive_whole(self):
        self.assertEqual(self.cs.compile_orc('''
            instr 1
                given:S = p4
                chnset(given, sprintf("given%d", p5))
                chnset(p6, sprintf("after%d", p5))
            endin
        '''), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        for n, text in enumerate(STRINGS):
            self.cs.event_string(f'i 1 0 0 "{text}" {n} {n + 10}')
        for _ in range(2):
            self.cs.perform_ksmps()
        for n, text in enumerate(STRINGS):
            self.assertEqual(self.cs.string_channel(f"given{n}"), text)
            self.assertEqual(self.cs.control_channel(f"after{n}")[0], n + 10)


if __name__ == '__main__':
    unittest.main()
