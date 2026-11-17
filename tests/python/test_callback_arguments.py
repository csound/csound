"""ctcsound's callback setters take what their docstrings say. None clears a
callback on every setter, though Csound takes a null pointer for that on most
of them only: it ignores one for messages, and calls four others unchecked. The
keyboard callback's types are ints that can be passed, ORed and compared. A
cleared callback can still crash, so what might runs in a process of its own."""

import os
import subprocess
import sys
import unittest
import weakref

PYTHON_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)
import ctcsound

PRELUDE = '''
import os, sys, tempfile, time, wave
import ctcsound

HEADER = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
"""
cs = ctcsound.Csound()
for option in ('-n', '-d', '-m0'):
    cs.set_option(option)

def perform(cycles):
    for _ in range(cycles):
        cs.perform_ksmps()
'''

# The main thread keeps the GIL until it blocks, so the performance thread is
# stopped inside the callback, at the GIL, when that callback is cleared.
CLEARED_WHILE_WAITING = PRELUDE + '''
sys.setswitchinterval(60)
calls = []
cs.set_output_channel_callback(lambda *_args: calls.append(1))
cs.compile_orc(HEADER + """instr 1
    outvalue("x", timeinstk())
endin
""")
cs.event_string('i 1 0 100')
cs.start()
thread = ctcsound.CsoundPerformanceThread(cs.csound())
thread.play()
while not calls:
    time.sleep(0.001)
held = time.monotonic() + 0.05
while time.monotonic() < held:
    pass
cs.set_output_channel_callback(None)
time.sleep(0.1)
left = len(calls)
time.sleep(0.1)
assert len(calls) == left, 'called after it was cleared'
thread.stop()
thread.join()
'''

CHANNELS = PRELUDE + '''
inputs, outputs = [], []
cs.set_input_channel_callback(lambda *_args: inputs.append(1))
cs.set_output_channel_callback(lambda *_args: outputs.append(1))
cs.compile_orc(HEADER + """instr 1
    k_value:k = invalue("in")
    outvalue("out", k_value)
endin
schedule(1, 0, 1)
""")
cs.start()
perform(1)
assert inputs and outputs, (inputs, outputs)
cs.set_input_channel_callback(None)
cs.set_output_channel_callback(None)
seen = len(inputs), len(outputs)
perform(1)
assert (len(inputs), len(outputs)) == seen
'''

# Csound opens a file itself when no callback is set, as when one returns NULL.
OPEN_FILES = PRELUDE + '''
opened = []
def open_sound_file(_csound, path, _flags, _info):
    opened.append(path)
def open_file(_csound, path, _mode):
    opened.append(path)
TABLES = """i_text:i = ftgen(1, 0, 0, -23, "%s")
i_sound:i = ftgen(2, 0, 0, 1, "%s", 0, 0, 0)
"""
with tempfile.TemporaryDirectory() as directory:
    text = os.path.join(directory, 'numbers.txt').replace(os.sep, '/')
    sound = os.path.join(directory, 'sound.wav').replace(os.sep, '/')
    with open(text, 'w') as file:
        file.write('1 2 3')
    with wave.open(sound, 'wb') as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(48000)
        file.writeframes(bytes(200))
    cs.set_open_sound_file_callback(open_sound_file)
    cs.set_open_file_callback(open_file)
    cs.compile_orc(HEADER)
    cs.start()
    assert cs.compile_orc(TABLES % (text, sound)) == 0
    assert len(opened) == 2, opened
    cs.set_open_sound_file_callback(None)
    cs.set_open_file_callback(None)
    opened.clear()
    assert cs.compile_orc(TABLES % (text, sound)) == 0
    assert opened == [], opened
    assert (cs.table_length(1), cs.table_length(2)) == (3, 100)
'''

# Csound checks these two callbacks when it opens the device, and never again.
MIDI_READ = PRELUDE + '''
reads = []
def read(_csound, _user_data, _buffer, _size):
    reads.append(1)
    return 0
cs.set_host_midi_IO()
cs.set_option('-M0')
cs.set_external_midi_in_open_callback(lambda *_args: 0)
cs.set_external_midi_read_callback(read)
cs.compile_orc(HEADER)
cs.start()
perform(1)
assert reads
cs.set_external_midi_read_callback(None)
seen = len(reads)
perform(1)
assert len(reads) == seen
'''

MIDI_WRITE = PRELUDE + '''
writes = []
def write(_csound, _user_data, _buffer, count):
    writes.append(1)
    return count
cs.set_host_midi_IO()
cs.set_option('-Q0')
cs.set_external_midi_out_open_callback(lambda *_args: 0)
cs.set_external_midi_write_callback(write)
cs.compile_orc(HEADER + """instr 1
    midiout(144, 1, 60, 100)
endin
schedule(1, 0, 1)
""")
cs.start()
perform(1)
assert writes
cs.set_external_midi_write_callback(None)
seen = len(writes)
perform(1)
assert len(writes) == seen
'''

# Csound calls this one for every list, which may never be null.
MIDI_DEVICES = PRELUDE + '''
cs.set_midi_device_list_callback(lambda *_args: 0)
cs.set_midi_device_list_callback(None)
assert cs.midi_dev_list() == []
assert cs.midi_dev_list(True) == []
'''

# Csound puts Ascii drawing in place of a null callback only when it starts.
DRAW_GRAPH = PRELUDE + '''
draws = []
cs.set_option('--displays')
cs.set_make_graph_callback(lambda *_args: None)
cs.set_draw_graph_callback(lambda *_args: draws.append(1))
cs.compile_orc(HEADER + """instr 1
    display(oscili(1, 100), 0.001)
endin
schedule(1, 0, 1)
""")
cs.start()
perform(100)
assert draws
cs.set_draw_graph_callback(None)
seen = len(draws)
perform(100)
assert len(draws) == seen
'''

MESSAGES_PRELUDE = PRELUDE + '''
first, second = [], []
def keep_first(_csound, _attr, message):
    first.append(message)
def keep_second(_csound, _attr, message):
    second.append(message)
'''

MESSAGE_CLEARED = MESSAGES_PRELUDE + '''
cs.set_message_string_callback(keep_first)
cs.message('to the callback')
cs.set_message_string_callback(None)
cs.message('to the terminal')
assert first == [b'to the callback'], first
cs.set_message_string_callback(keep_second)
cs.message('to the next callback')
assert first == [b'to the callback'], first
assert second == [b'to the next callback'], second
'''

# Csound keeps the cleared pointer, and saves and restores it around this mode.
MESSAGE_CLEARED_BEFORE_REALTIME = MESSAGES_PRELUDE + '''
cs.set_option('--realtime')
cs.set_option('-m1')
cs.set_message_string_callback(keep_first)
cs.set_message_string_callback(None)
cs.compile_orc(HEADER)
cs.start()
cs.message('in the performance')
cs.reset()
assert first == [], first
'''

KEYBOARD_PRELUDE = PRELUDE + '''
calls = []
def key(_user_data, _data, type_):
    calls.append(type_)
    return 0
'''

KEYBOARD_EVENT = """instr 1
    k_key:k, k_down:k = sensekey()
endin
schedule(1, 0, 1)
"""

KEYBOARD_TEXT = """instr 1
    k_key:k = sensekey()
endin
schedule(1, 0, 1)
"""

# What a callback is handed compares equal to the constant it is named by.
KEYBOARD_TYPE = KEYBOARD_PRELUDE + '''
assert cs.register_keyboard_callback(key, 'data', ctcsound.%s) == 0
cs.compile_orc(HEADER + """%s""")
cs.start()
perform(1)
assert calls == [ctcsound.%s], calls
'''

KEYBOARD_INT = KEYBOARD_PRELUDE + '''
assert cs.register_keyboard_callback(key, 'data', 1) == 0
assert cs.register_keyboard_callback(key, 'data', ctcsound.CSOUND_CALLBACK_KBD_EVENT.value) == 0
cs.compile_orc(HEADER + """%s""")
cs.start()
perform(1)
assert calls == [1], calls
''' % KEYBOARD_EVENT

KEYBOARD_MASK = KEYBOARD_PRELUDE + '''
both = ctcsound.CSOUND_CALLBACK_KBD_EVENT | ctcsound.CSOUND_CALLBACK_KBD_TEXT
assert cs.register_keyboard_callback(key, 'data', both) == 0
cs.compile_orc(HEADER + """%s""")
cs.start()
perform(1)
assert calls == [ctcsound.CSOUND_CALLBACK_KBD_TEXT], calls
''' % KEYBOARD_TEXT


class Callback:
    def __call__(self, *_args):
        return 0


class CallbackArgumentTests(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.addCleanup(self.cs.destroy)

    def run_alone(self, script):
        environment = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        result = subprocess.run([sys.executable, '-c', script], env=environment,
                                stdin=subprocess.DEVNULL, capture_output=True,
                                text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr[-2000:])
        return result

    def setters(self):
        names = [name for name in dir(ctcsound.Csound)
                 if name.startswith('set_') and name.endswith('_callback')]
        self.assertGreaterEqual(len(names), 17)
        return names

    def test_every_setter_takes_none(self):
        for name in self.setters():
            with self.subTest(name):
                getattr(self.cs, name)(Callback())
                getattr(self.cs, name)(None)

    def test_every_setter_keeps_the_callback_it_clears(self):
        for name in self.setters():
            with self.subTest(name):
                cleared = Callback()
                kept = weakref.ref(cleared)
                getattr(self.cs, name)(cleared)
                del cleared
                getattr(self.cs, name)(None)
                self.assertIsNotNone(kept())

    def test_a_callback_can_be_cleared_while_a_thread_waits_in_it(self):
        self.run_alone(CLEARED_WHILE_WAITING)

    def test_none_clears_the_channel_callbacks(self):
        self.run_alone(CHANNELS)

    def test_none_leaves_opening_a_file_to_csound(self):
        self.run_alone(OPEN_FILES)

    def test_none_clears_the_midi_read_callback_in_a_performance(self):
        self.run_alone(MIDI_READ)

    def test_none_clears_the_midi_write_callback_in_a_performance(self):
        self.run_alone(MIDI_WRITE)

    def test_none_clears_the_midi_device_list_callback(self):
        self.run_alone(MIDI_DEVICES)

    def test_none_clears_the_draw_graph_callback_in_a_performance(self):
        self.run_alone(DRAW_GRAPH)

    def test_none_sends_messages_back_to_the_terminal(self):
        result = self.run_alone(MESSAGE_CLEARED)
        printed = result.stdout + result.stderr
        self.assertIn('to the terminal', printed)
        self.assertNotIn('to the callback', printed)
        self.assertNotIn('to the next callback', printed)

    def test_a_message_callback_cleared_before_realtime_stays_cleared(self):
        result = self.run_alone(MESSAGE_CLEARED_BEFORE_REALTIME)
        self.assertIn('in the performance', result.stdout + result.stderr)

    def test_a_keyboard_constant_is_the_type_a_callback_is_handed(self):
        for name, orchestra in (('CSOUND_CALLBACK_KBD_EVENT', KEYBOARD_EVENT),
                                ('CSOUND_CALLBACK_KBD_TEXT', KEYBOARD_TEXT)):
            with self.subTest(name):
                self.run_alone(KEYBOARD_TYPE % (name, orchestra, name))

    def test_a_keyboard_type_can_still_be_an_int_or_a_value(self):
        self.run_alone(KEYBOARD_INT)

    def test_keyboard_types_can_be_ored(self):
        self.run_alone(KEYBOARD_MASK)


if __name__ == '__main__':
    unittest.main()
