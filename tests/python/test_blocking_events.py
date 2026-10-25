"""Score lines a host sends blocking - csoundEventString with async 0 - while
a performance thread performs: they are appended to the line-event buffer on
the host's thread while the performance thread empties it at the top of each
k-cycle, and with nothing locking the two, a quarter of them were lost."""

import os
import sys
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound


EVENTS = 3000


class TestBlockingEventsFromAnotherThread(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)

    def tearDown(self):
        self.cs.reset()

    def test_every_event_sent_while_performing_arrives(self):
        self.assertEqual(self.cs.compile_orc('''
            sr = 48000
            ksmps = 64
            nchnls = 1
            0dbfs = 1
            instr Count
                chnset:i(chnget:i("count") + 1, "count")
                turnoff
            endin
        '''), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        self.cs.set_control_channel("count", 0)
        thread = ctcsound.CsoundPerformanceThread(self.cs.csound())
        thread.play()
        try:
            for _ in range(EVENTS):
                self.cs.event_string('i "Count" 0 0', False)
            deadline = time.monotonic() + 10
            while (self.cs.control_channel("count")[0] < EVENTS
                   and time.monotonic() < deadline):
                time.sleep(0.05)
        finally:
            thread.stop()
            thread.join()
        self.assertEqual(int(self.cs.control_channel("count")[0]), EVENTS)


if __name__ == '__main__':
    unittest.main()
