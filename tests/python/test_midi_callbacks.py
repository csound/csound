"""ctcsound's MIDI read and write callbacks: the host is handed Csound's own
buffer and a count, so a zero data byte passes like any other, and what a read
callback writes into the buffer reaches Csound. The buffers are binary, and
typed as C strings they were cut at the first zero, and could not be written."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound

HEADER = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
"""

# A note off at velocity 0, controller 0, and a program change of two bytes.
SENDS = HEADER + """instr 1
    midiout_i(128, 1, 60, 0)
    midiout_i(176, 1, 0, 5)
    midiout_i(192, 1, 0, 0)
endin
schedule(1, 0, 0)
"""
SENT = [bytes((0x80, 60, 0)), bytes((0xB0, 0, 5)), bytes((0xC0, 0))]

RECEIVES = HEADER + """instr 1
    chnset(notnum(), "note")
    chnset(veloc(), "velocity")
endin
"""


class MidiCallbackTests(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.cs.set_host_midi_IO()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)

    def tearDown(self):
        self.cs.destroy()

    def written(self, set_callback):
        """What Csound writes through the callback set_callback sets."""
        messages = []

        def write(_cs, _user_data, buffer, count):
            messages.append(bytes(buffer[:count]))
            return count

        self.cs.set_option('-Q0')
        self.cs.set_external_midi_out_open_callback(lambda _cs, _user_data, _name: 0)
        set_callback(write)
        self.assertEqual(self.cs.compile_orc(SENDS), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.perform_ksmps(), ctcsound.CSOUND_SUCCESS)
        return messages

    def test_a_write_hands_every_byte(self):
        self.assertEqual(self.written(self.cs.set_external_midi_write_callback), SENT)

    def test_the_misspelt_name_still_sets_the_write_callback(self):
        self.assertEqual(self.written(self.cs.set_external_midi_write_callbackk), SENT)

    def test_a_read_fills_csound_s_buffer(self):
        """Note 0, whose zero is followed by a velocity to read."""
        pending = [bytes((0x90, 0, 100))]

        def read(_cs, _user_data, buffer, _size):
            if not pending:
                return 0
            message = pending.pop()
            try:
                for n, byte in enumerate(message):
                    buffer[n] = byte
            except TypeError:  # Raising would pass Csound an arbitrary count.
                return 0
            return len(message)

        self.cs.set_option('-M0')
        self.cs.set_external_midi_in_open_callback(lambda _cs, _user_data, _name: 0)
        self.cs.set_external_midi_read_callback(read)
        self.cs.set_control_channel("note", -1)
        self.assertEqual(self.cs.compile_orc(RECEIVES), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.perform_ksmps(), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.control_channel("note")[0], 0)
        self.assertEqual(self.cs.control_channel("velocity")[0], 100)


if __name__ == '__main__':
    unittest.main()
