"""ctcsound's MIDI error string callback, on a host whose MIDI input fails: what
it returns reaches Csound's message, and is not leaked, and one that raises or
returns nonsense leaves Csound a message to print, not an unset pointer."""

import ctypes
import os
import subprocess
import sys
import unittest
import warnings
import weakref

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
PYTHON_DIR = os.path.join(TESTS_DIR, '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)
import ctcsound

ORCHESTRA = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
endin
"""
CYCLES = 100
UNKNOWN = ' *** error reading MIDI device: -5 (Unknown MIDI error)\n'

# argv is ctcsound's directory, this one's, and the callback as an expression.
CHILD = """
import sys
sys.path[:0] = sys.argv[1:3]
from test_midi_error_string import raises, reported
print(''.join(sorted(set(reported(eval(sys.argv[3]))))), end='')
"""


def raises(_code):
    raise ValueError('no message for that code')


def reported(error):
    """Csound's messages of a read that fails each cycle, with error as the callback."""
    messages = []

    def message(_cs, _attr, text):
        if b'reading MIDI' in text:
            messages.append(text.decode(errors='replace'))

    cs = ctcsound.Csound()
    cs.set_message_string_callback(message)
    cs.set_host_midi_IO()
    for option in ('-n', '-d', '-m0', '-M0'):
        cs.set_option(option)
    cs.set_external_midi_in_open_callback(lambda _cs, _user_data, _name: 0)
    cs.set_external_midi_read_callback(lambda _cs, _user_data, _buffer, _size: -5)
    cs.set_external_midi_error_string_callback(error)
    cs.compile_orc(ORCHESTRA)
    cs.start()
    for _ in range(CYCLES):
        cs.perform_ksmps()
    cs.destroy()
    return messages


class MidiErrorStringTests(unittest.TestCase):

    def child(self, error):
        """reported(error) in a process of its own, which an unset pointer can end."""
        result = subprocess.run([sys.executable, '-c', CHILD, PYTHON_DIR, TESTS_DIR, error],
                                capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr)
        result.stdout = result.stdout[result.stdout.find(' ***'):]  # Importing ctcsound prints on macOS.
        return result

    def test_bytes_are_the_message_and_are_not_leaked(self):
        text = 'device unplugged'.encode()
        references = sys.getrefcount(text)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter('always')
            messages = reported(lambda _code: text)
        self.assertEqual(messages, [' *** error reading MIDI device: -5 (device unplugged)\n'] * CYCLES)
        self.assertEqual([str(warning.message) for warning in caught], [])
        self.assertLessEqual(sys.getrefcount(text), references + 1)

    def test_a_str_is_the_message(self):
        self.assertEqual(self.child("lambda code: 'unplugged: %d' % code").stdout,
                         ' *** error reading MIDI device: -5 (unplugged: -5)\n')

    def test_a_callback_that_raises_leaves_a_message(self):
        result = self.child('raises')
        self.assertEqual(result.stdout, UNKNOWN)
        self.assertIn('Exception ignored', result.stderr)  # As with any callback.
        self.assertIn('ValueError: no message for that code', result.stderr)

    def test_a_failure_does_not_repeat_the_last_message(self):
        self.assertEqual(self.child("lambda code, results=iter(['first']): next(results)").stdout,
                         UNKNOWN + ' *** error reading MIDI device: -5 (first)\n')

    def test_a_result_ctypes_cannot_take_leaves_a_message(self):
        for nonsense in ('1.5', '[]'):
            with self.subTest(nonsense):
                result = self.child('lambda code: ' + nonsense)
                self.assertEqual(result.stdout, UNKNOWN)
                self.assertIn('TypeError', result.stderr)

    def test_no_address_leaves_a_message(self):
        for nothing in ('None', '0', '1 << 64'):
            with self.subTest(nothing):
                self.assertEqual(self.child('lambda code: ' + nothing).stdout, UNKNOWN)

    def test_an_empty_message_stays_empty(self):
        self.assertEqual(set(reported(lambda _code: b'')),
                         {' *** error reading MIDI device: -5 ()\n'})

    def test_an_address_is_still_taken_as_one(self):
        buffer = ctypes.create_string_buffer(b'held by the host')
        self.assertEqual(set(reported(lambda _code: ctypes.addressof(buffer))),
                         {' *** error reading MIDI device: -5 (held by the host)\n'})

    def test_a_c_function_is_still_taken_by_its_address(self):
        buffer = ctypes.create_string_buffer(b'held by the host')
        function = ctcsound.MIDIERRORFUNC(lambda _code: ctypes.addressof(buffer))
        self.assertEqual(set(reported(ctypes.cast(function, ctypes.c_void_p).value)),
                         {' *** error reading MIDI device: -5 (held by the host)\n'})

    def test_what_is_no_function_is_still_refused(self):
        cs = ctcsound.Csound()
        with self.assertRaises(TypeError):
            cs.set_external_midi_error_string_callback('no function')

    def test_the_callback_does_not_keep_csound_alive(self):
        cs = ctcsound.Csound()
        cs.set_external_midi_error_string_callback(lambda _code: b'')
        collected = weakref.ref(cs)
        del cs
        self.assertIsNone(collected())


if __name__ == '__main__':
    unittest.main()
