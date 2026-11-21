"""ctcsound's async_status: whether the queue between the host and the
performance, API_MAX_QUEUE requests a k-cycle, took the last async request.
A request it has no room for is dropped, and async_status is CSOUND_ERROR
until the next async request is taken. The queue is filled without
performing, so nothing drains it until the k-cycle the case asks for. The C
function itself is tests/c/api_message_queue_test.cpp's."""

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

    def test_a_request_without_room_is_reported(self):
        for _ in range(QUEUE):
            self.cs.event_string('i 1 0 0', True)
        self.assertEqual(self.cs.async_status(), ctcsound.CSOUND_SUCCESS)
        self.cs.event(ctcsound.CS_INSTR_EVENT, (1, 0, 0), True)
        self.assertEqual(self.cs.async_status(), ctcsound.CSOUND_ERROR)
        self.cs.perform_ksmps()
        self.cs.event_string('i 1 0 0', True)
        self.assertEqual(self.cs.async_status(), ctcsound.CSOUND_SUCCESS)


if __name__ == '__main__':
    unittest.main()
