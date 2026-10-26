"""What ctcsound's event and event_string say when the queue between the
host and the performance, API_MAX_QUEUE requests a k-cycle, has no room for
a request: it is dropped, as csoundEvent and csoundEventString drop it, and
their Try forms, which these call, return CSOUND_ERROR, so a host can tell.
Each case fills the queue without performing, so nothing drains it until the
k-cycle the case asks for. The four Try forms themselves are
tests/c/api_message_queue_test.cpp's."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound

QUEUE = 1024  # API_MAX_QUEUE in Top/threadsafe.c

ORCHESTRA = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
endin
"""


class AsyncStatusTests(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)
        self.assertEqual(self.cs.compile_orc(ORCHESTRA), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)

    def tearDown(self):
        self.cs.destroy()

    def filled(self, request):
        """The queue filled with `request`, each call's status checked, and
        then one call more, which finds it full."""
        statuses = {request() for _ in range(QUEUE)}
        self.assertEqual(statuses, {ctcsound.CSOUND_SUCCESS})
        self.assertEqual(request(), ctcsound.CSOUND_ERROR)
        self.cs.perform_ksmps()
        self.assertEqual(request(), ctcsound.CSOUND_SUCCESS)

    def test_an_event_string_without_room_is_refused(self):
        self.filled(lambda: self.cs.event_string('i 1 0 0', True))

    def test_an_event_without_room_is_refused(self):
        self.filled(lambda: self.cs.event(ctcsound.CS_INSTR_EVENT, (1, 0, 0), True))

    def test_a_request_that_runs_at_once_says_it_ran(self):
        self.assertEqual(self.cs.event_string('i 1 0 0'), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.event(ctcsound.CS_INSTR_EVENT, (1, 0, 0)),
                         ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.event(99, (1, 0, 0)), ctcsound.CSOUND_ERROR)


if __name__ == '__main__':
    unittest.main()
