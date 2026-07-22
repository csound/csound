"""Tests for the ctcsound generic channel memory API."""

import os
import sys
import unittest
import ctypes as ct

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'Python'))
import ctcsound


class TestChannelVariableMemory(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.cs.set_option('-n')
        self.cs.set_option('-d')

    def tearDown(self):
        self.cs.reset()

    def create_control_channel(self, name):
        channel, error = self.cs.channel_ptr(
            name,
            ctcsound.CSOUND_CONTROL_CHANNEL |
            ctcsound.CSOUND_INPUT_CHANNEL |
            ctcsound.CSOUND_OUTPUT_CHANNEL)
        self.assertEqual(error, '')
        self.assertIsNotNone(channel)
        return channel

    def test_channel_memory_can_be_copied_between_matching_channels(self):
        source = self.create_control_channel('source')
        destination = self.create_control_channel('destination')
        source[0] = 12.5
        destination[0] = 0.0

        source_memory = self.cs.channel('source')
        self.assertTrue(source_memory)
        self.assertEqual(
            self.cs.set_channel('destination', source_memory),
            ctcsound.CSOUND_SUCCESS)
        self.assertEqual(destination[0], 12.5)

    def test_channel_returns_none_for_missing_channel(self):
        self.assertFalse(self.cs.channel('missing'))

    def test_generic_channel_pointer_is_opaque(self):
        self.assertEqual(self.cs.compile_orc('''
            instr 1
                value:Complex init 1, 1
                chnset value, "complex"
            endin
            schedule(1, 0, 1)
        '''), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.perform_ksmps(), ctcsound.CSOUND_SUCCESS)

        value, error = self.cs.channel_ptr(
            'complex',
            ctcsound.CSOUND_VAR_CHANNEL |
            ctcsound.CSOUND_OUTPUT_CHANNEL)
        self.assertEqual(error, '')
        self.assertIsInstance(value, ct.c_void_p)
        self.assertTrue(value)

    def test_set_channel_rejects_mismatched_types(self):
        self.create_control_channel('source')
        string_channel, error = self.cs.channel_ptr(
            'destination',
            ctcsound.CSOUND_STRING_CHANNEL |
            ctcsound.CSOUND_INPUT_CHANNEL)
        self.assertEqual(error, '')
        self.assertIsNotNone(string_channel)

        self.assertEqual(
            self.cs.set_channel('destination', self.cs.channel('source')),
            ctcsound.CSOUND_ERROR)


if __name__ == '__main__':
    unittest.main()
