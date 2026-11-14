"""What C gets from a ctcsound callback that has no answer. ctypes leaves the
result of a callback unset if it raises, or returns what that result cannot
hold, and C reads whatever was there: a count, a status, a FILE pointer. So
these run in a process of their own, which such a result can end. Each result
has the value that makes the C that reads it do the safe thing."""

import ctypes
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest
import wave
import weakref

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
PYTHON_DIR = os.path.join(TESTS_DIR, '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)
import ctcsound

HEADER = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
"""
CYCLES = 20
NOTE = 10  # k-cycles
ANSWER = 12345
ERROR = ctcsound.CSOUND_ERROR

# A thunk's result is left where the last call left it, or holds something else
# from the stack, so a first answer shows a second that is left unset. Each is:
# setter, its thunk, arguments for it, what C gets from a callback with no answer.
SETTERS = (
    ('set_external_midi_in_open_callback', 'ext_midi_in_open_cb_ref', (None, None, b''), ERROR),
    ('set_external_midi_read_callback', 'ext_midi_read_cb_ref', (None, None, None, 0), ERROR),
    ('set_external_midi_in_close_callback', 'ext_midi_in_close_cb_ref', (None, None), ERROR),
    ('set_external_midi_out_open_callback', 'ext_midi_out_open_cb_ref', (None, None, b''), ERROR),
    ('set_external_midi_write_callback', 'ext_midi_write_cb_ref', (None, None, None, 0), ERROR),
    ('set_external_midi_out_close_callback', 'ext_midi_out_close_cb_ref', (None, None), ERROR),
    ('set_midi_device_list_callback', 'midi_dev_list_cb_ref', (None, None, 0), 0),
    ('set_exit_graph_callback', 'exit_graph_cb_ref', (None,), 0),
    ('set_open_sound_file_callback', 'open_sound_file_cb_ref', (None, b'', 0, None), None),
    ('set_open_file_callback', 'open_file_cb_ref', (None, b'', b''), None),
)

# argv is ctcsound's directory, this one's, a function of this file and its arguments.
CHILD = """
import json, sys
sys.path[:0] = sys.argv[1:3]
import test_callback_results as test
print('\\nRESULT' + json.dumps(getattr(test, sys.argv[3])(*sys.argv[4:])))
"""


def thunks(make):
    """Each callback ctcsound sets as (name, thunk, arguments, what C gets if it fails)."""
    cs = ctcsound.Csound()
    for setter, attribute, arguments, failed in SETTERS:
        getattr(cs, setter)(make())
        yield setter, getattr(cs, attribute), arguments, failed
    keyboard = make()
    cs.register_keyboard_callback(keyboard, None, 0)
    yield 'keyboard', cs._keyboard_thunks[cs._keyboard_key(keyboard)], (None, None, 0), ERROR
    cs.append_opcode('hostop', 64, 0, '', '', make(), make(), make())
    for name, thunk in zip(('init', 'perf', 'deinit'), cs._kept[-3:]):
        yield name, thunk, (None, None), ERROR


def then(failure):
    """A callback that answers once, then does what failure says."""
    def make():
        calls = []

        def function(*_args):
            calls.append(1)
            return ANSWER if len(calls) == 1 else eval(failure)
        return function
    return make


def answers(failure):
    """What each thunk gives C for a first and a second call, with the result expected."""
    return {name: [thunk(*arguments), thunk(*arguments), failed]
            for name, thunk, arguments, failed in thunks(then(failure))}


def as_they_are(result):
    """What each thunk gives C for a result ctypes can take."""
    return {name: thunk(*arguments) for name, thunk, arguments, _failed
            in thunks(lambda: lambda *_args: eval(result))}


def raises(*_args):
    raise ValueError('no answer')


def csound(messages):
    """A Csound that adds its messages to messages."""
    cs = ctcsound.Csound()
    cs.set_message_string_callback(
        lambda _cs, _attr, text: messages.append(text.decode(errors='replace')))
    for option in ('-n', '-d', '-m0'):
        cs.set_option(option)
    return cs


def perform(cs, orchestra):
    """Performs CYCLES k-cycles of orchestra, if it starts, and gives what start returned."""
    cs.compile_orc(HEADER + orchestra)
    started = cs.start()
    for _ in range(0 if started else CYCLES):
        cs.perform_ksmps()
    return started


def open_file(path):
    """Writes path through a file callback that raises."""
    cs = csound([])
    cs.set_open_file_callback(raises)
    perform(cs, 'instr 1\n  fprints("%s", "written")\nendin\nschedule(1, 0, 0)\n' % path)
    cs.destroy()
    with open(path) as written:
        return written.read()


def open_sound_file(path):
    """A sample of path, read through a sound file callback that raises."""
    cs = csound([])
    cs.set_open_sound_file_callback(raises)
    perform(cs, 'instr 1\n  a_sample = diskin2("%s", 1)\n  chnset(k(a_sample), "sample")\n'
                'endin\nschedule(1, 0, 1)\n' % path)
    sample = cs.control_channel('sample')[0]
    cs.destroy()
    return sample


def midi(direction, failing):
    """What start returned and Csound's errors, with one host MIDI callback raising."""
    messages = []
    cs = csound(messages)
    cs.set_host_midi_IO()
    cs.set_option('-M0' if direction == 'in' else '-Q0')
    for name in {'in': ('in_open', 'read', 'in_close'),
                 'out': ('out_open', 'write', 'out_close')}[direction]:
        getattr(cs, 'set_external_midi_%s_callback' % name)(
            raises if name == failing else lambda *_args: 0)
    started = perform(cs, 'instr 1\nendin\n')
    cs.destroy()
    return started, sorted({line for line in messages if 'MIDI' in line})


def midi_devices():
    """The list of devices of a host whose callback raises."""
    cs = csound([])
    cs.set_midi_device_list_callback(raises)
    devices = cs.midi_dev_list()
    cs.destroy()
    return devices


def opcode(failing):
    """Two notes of an opcode whose function failing answers 0 once, then raises.

    Gives how many k-cycles got past the opcode, and its deinit errors.
    """
    messages = []
    calls = []

    def fails(*_args):
        calls.append(1)
        if len(calls) > 1:
            raise ValueError('no answer')
        return 0

    cs = csound(messages)
    cs.append_opcode('hostop', 64, 0, '', '', *(fails if name == failing else lambda *_args: 0
                                                for name in ('init', 'perf', 'deinit')))
    perform(cs, """instr 1
    hostop()
    chnset(chnget:k("after") + 1, "after")
endin
schedule(1, 0, %d / kr)
schedule(1, 0, %d / kr)
""" % (NOTE, NOTE))
    after = cs.control_channel('after')[0]
    cs.destroy()
    return after, [line for line in messages if 'deinit error' in line]


def on_csound_s_thread():
    """What reaches sys.unraisablehook of a perf function that raises on Csound's thread."""
    reported = []
    sys.unraisablehook = reported.append
    cs = csound([])
    cs.append_opcode('hostop', 64, 0, '', '', lambda *_args: 0, raises, lambda *_args: 0)
    cs.compile_orc(HEADER + 'instr 1\n  hostop()\nendin\nschedule(1, 0, 1000)\n')
    cs.start()
    thread = ctcsound.CsoundPerformanceThread(cs.csound())
    thread.play()
    deadline = time.monotonic() + 30
    while len(reported) < CYCLES and time.monotonic() < deadline:
        time.sleep(0.001)
    thread.stop()
    thread.join()
    cs.destroy()
    return sorted({unraisable.exc_type.__name__ for unraisable in reported}), len(reported)


class CallbackResultTests(unittest.TestCase):

    def child(self, function, *arguments):
        """The result of a function above in a process of its own."""
        result = subprocess.run(
            [sys.executable, '-c', CHILD, PYTHON_DIR, TESTS_DIR, function, *arguments],
            capture_output=True, text=True, timeout=60, stdin=subprocess.DEVNULL)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.stderr = result.stderr
        return json.loads(result.stdout.rpartition('\nRESULT')[2])

    def test_a_callback_that_raises_hands_c_a_defined_result(self):
        for name, (first, second, failed) in self.child('answers', '1 / 0').items():
            with self.subTest(name):
                self.assertEqual(first, ANSWER)
                self.assertEqual(second, failed)

    def test_a_result_ctypes_cannot_take_hands_c_a_defined_result(self):
        for nonsense in ('None', '1.5', '[]', "'text'"):
            for name, (_first, second, failed) in self.child('answers', nonsense).items():
                with self.subTest(name, nonsense=nonsense):
                    self.assertEqual(second, failed)

    def test_a_file_callback_that_raises_leaves_the_file_to_csound(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, 'out.txt').replace(os.sep, '/')
            self.assertEqual(self.child('open_file', path), 'written')
        self.assertIn('ValueError: no answer', self.stderr)

    def test_a_sound_file_callback_that_raises_leaves_the_file_to_csound(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, 'in.wav').replace(os.sep, '/')
            with wave.open(path, 'wb') as sound:
                sound.setnchannels(1)
                sound.setsampwidth(2)
                sound.setframerate(48000)
                sound.writeframes(b'\x00\x40' * 4800)
            self.assertAlmostEqual(self.child('open_sound_file', path), 0.5)
        self.assertIn('ValueError: no answer', self.stderr)

    def test_a_midi_open_that_raises_stops_the_start(self):
        for direction, name in (('in', 'in_open'), ('out', 'out_open')):
            with self.subTest(direction):
                started, messages = self.child('midi', direction, name)
                self.assertEqual(started, ERROR)
                self.assertEqual(messages, [' *** error opening MIDI %s device: -1 '
                                            '(Unknown MIDI error)\n' % direction])

    def test_a_midi_read_that_raises_is_an_error_and_no_count(self):
        self.assertEqual(self.child('midi', 'in', 'read'),
                         [0, [' *** error reading MIDI device: -1 (Unknown MIDI error)\n']])

    def test_a_midi_close_that_raises_is_an_error(self):
        for direction in ('in', 'out'):
            with self.subTest(direction):
                self.assertEqual(self.child('midi', direction, direction + '_close'),
                                 [0, ['Error closing MIDI %s device: -1 (Unknown MIDI error)\n'
                                      % direction]])

    def test_a_device_list_that_raises_is_empty(self):
        self.assertEqual(self.child('midi_devices'), [])

    def test_an_init_that_raises_stops_its_instrument(self):
        self.assertEqual(self.child('opcode', 'init'), [NOTE, []])

    def test_a_perf_that_raises_stops_its_k_cycle(self):
        self.assertEqual(self.child('opcode', 'perf'), [1, []])

    def test_a_deinit_that_raises_is_an_error(self):
        self.assertEqual(self.child('opcode', 'deinit'), [NOTE * 2, ['hostop deinit error\n']])

    def test_a_callback_on_csound_s_thread_is_reported(self):
        names, count = self.child('on_csound_s_thread')
        self.assertEqual(names, ['ValueError'])
        self.assertGreaterEqual(count, CYCLES)

    def test_an_int_reaches_c_as_it_is(self):
        for result, status, pointer in (('0', 0, None), ('7', 7, 7), ('True', 1, 1),
                                        ('(1 << 32) + 1', 1, (1 << 32) + 1)):
            with self.subTest(result):
                for name, answer in self.child('as_they_are', result).items():
                    expected = pointer if 'file' in name else status
                    self.assertEqual(answer, expected, name)

    def test_a_c_function_is_still_taken_by_its_address(self):
        function = ctcsound.OPENFILEFUNC(lambda *_args: ANSWER)
        cs = ctcsound.Csound()
        cs.set_open_file_callback(ctypes.cast(function, ctypes.c_void_p).value)
        self.assertEqual(cs.open_file_cb_ref(None, b'', b''), ANSWER)

    def test_the_callback_does_not_keep_csound_alive(self):
        cs = ctcsound.Csound()
        cs.set_open_file_callback(lambda *_args: None)
        collected = weakref.ref(cs)
        del cs
        self.assertIsNone(collected())


if __name__ == '__main__':
    unittest.main()
