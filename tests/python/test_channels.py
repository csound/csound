"""Tests for the ctcsound generic channel memory API."""

import os
import sys
import threading
import unittest
import ctypes as ct
from unittest import mock

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
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


class TestStringChannel(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.cs.set_option('-n')
        self.cs.set_option('-d')

    def tearDown(self):
        self.cs.reset()

    def test_string_channel_returns_what_was_set(self):
        self.cs.set_string_channel('text', 'caf\u00e9')
        self.assertEqual(self.cs.string_channel('text'), 'caf\u00e9')

    def test_string_channel_does_not_make_a_missing_channel(self):
        self.assertEqual(self.cs.string_channel('missing'), '')
        self.assertEqual(ctcsound.libcsound.csoundGetChannelDatasize(
            self.cs.cs, b'missing'), 0)

    def test_string_channel_of_another_type_is_empty(self):
        self.cs.set_control_channel('control', 1.0)
        self.assertEqual(self.cs.string_channel('control'), '')

    def test_string_grown_after_it_was_sized_stays_in_the_buffer(self):
        grown = 'x' * 4096
        guard = len(grown) + 1
        datasize = ctcsound.libcsound.csoundGetChannelDatasize
        buffers = []

        def size_then_grow(csound, name):
            size = datasize(csound, name)
            self.cs.set_string_channel('text', grown)
            return size

        def guarded_buffer(size):
            # What overruns the buffer lands in memory that is ours to check.
            whole = (ct.c_char * (size + guard))()
            ct.memset(ct.addressof(whole) + size, 0xa5, guard)
            buffers.append((whole, size))
            return (ct.c_char * size).from_buffer(whole)

        self.cs.set_string_channel('text', 'short')
        with mock.patch.object(ctcsound.libcsound, 'csoundGetChannelDatasize',
                               size_then_grow), \
             mock.patch.object(ct, 'create_string_buffer', guarded_buffer):
            value = self.cs.string_channel('text')

        self.assertEqual(value, grown)
        for whole, size in buffers:
            self.assertEqual(bytes(whole)[size:], b'\xa5' * guard)

    def test_string_channel_during_writes_from_another_thread(self):
        self.cs.set_string_channel('text', 'a')
        finished = threading.Event()

        def write():
            for length in range(128, 65537, 128):
                self.cs.set_string_channel('text', 'a' * length)
            finished.set()

        writer = threading.Thread(target=write, daemon=True)
        writer.start()
        while not finished.is_set():
            value = self.cs.string_channel('text')
            self.assertEqual(value, 'a' * len(value))
        writer.join(10)
        self.assertFalse(writer.is_alive())
        self.assertEqual(self.cs.string_channel('text'), 'a' * 65536)

    def test_string_channel_unlocks_when_the_copy_raises(self):
        self.cs.set_string_channel('text', 'short')
        unlock = mock.Mock(wraps=ctcsound.libcsound.csoundUnlockChannel)
        with mock.patch.object(ctcsound.libcsound, 'csoundUnlockChannel', unlock), \
             mock.patch.object(ctcsound, 'string_copy', side_effect=MemoryError):
            with self.assertRaises(MemoryError):
                self.cs.string_channel('text')
        unlock.assert_called_once()


if __name__ == '__main__':
    unittest.main()
