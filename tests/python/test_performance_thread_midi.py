"""MIDI is set up on a Csound, and a CsoundPerformanceThread is not one: it
keeps no Csound of its own, so a copy of Csound's MIDI methods in it could only
raise AttributeError on the self.cs it never sets."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound

MIDI_SET_UP = (
    'set_host_midi_IO',
    'set_midi_module',
    'set_external_midi_in_open_callback',
    'set_external_midi_read_callback',
    'set_external_midi_in_close_callback',
    'set_external_midi_out_open_callback',
    'set_external_midi_write_callback',
    'set_external_midi_write_callbackk',
    'set_external_midi_out_close_callback',
    'set_external_midi_error_string_callback',
    'set_midi_device_list_callback',
)


class TestPerformanceThreadMidi(unittest.TestCase):

    def test_midi_is_set_up_on_csound_alone(self):
        for name in MIDI_SET_UP:
            with self.subTest(name):
                self.assertTrue(callable(getattr(ctcsound.Csound, name)))
                self.assertFalse(hasattr(ctcsound.CsoundPerformanceThread, name))


if __name__ == '__main__':
    unittest.main()
