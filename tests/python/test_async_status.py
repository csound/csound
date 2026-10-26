"""What an asynchronous API call says when the queue between the host and
the performance, API_MAX_QUEUE requests a k-cycle, has no room for it: the
request is dropped, as it was, and the call now returns CSOUND_ERROR rather
than nothing, so a host can tell. Each case fills the queue without
performing, so nothing drains it until the k-cycle the case asks for."""

import ctypes as ct
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
        self.assertEqual(self.cs.event_string('f 1 0 8 -2 0'), ctcsound.CSOUND_SUCCESS)
        self.cs.perform_ksmps()
        # An async copy runs at a later k-cycle, from this buffer.
        self.table = (ctcsound.cs_float * 8)()

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

    def copy(self, function):
        return lambda: function(self.cs.cs, ct.c_int32(1), self.table, ct.c_int32(1))

    def test_an_event_string_without_room_is_refused(self):
        self.filled(lambda: self.cs.event_string('i 1 0 0', True))

    def test_an_event_without_room_is_refused(self):
        self.filled(lambda: self.cs.event(ctcsound.CS_INSTR_EVENT, (1, 0, 0), True))

    def test_a_table_copy_without_room_is_refused(self):
        for function in (ctcsound.libcsound.csoundTableCopyIn,
                         ctcsound.libcsound.csoundTableCopyOut):
            function.restype = ct.c_int32
            with self.subTest(function=function.__name__):
                self.filled(self.copy(function))
                self.cs.perform_ksmps()  # the last one, run: the queue empty

    def test_a_request_that_runs_at_once_says_it_ran(self):
        self.assertEqual(self.cs.event_string('i 1 0 0'), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.event(ctcsound.CS_INSTR_EVENT, (1, 0, 0)),
                         ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.event(99, (1, 0, 0)), ctcsound.CSOUND_ERROR)


if __name__ == '__main__':
    unittest.main()
