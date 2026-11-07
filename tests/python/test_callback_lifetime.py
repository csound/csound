"""ctcsound's callbacks stay alive for as long as Csound can call them. Csound
holds a bare pointer to the thunk ctypes makes for a callback, and a thread can
be inside one, waiting for the GIL, however late it is replaced: a thunk
released while C still has it is a crash, so most of these run in a process of
their own."""

import os
import subprocess
import sys
import unittest
import weakref

PYTHON_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)
import ctcsound

PRELUDE = '''
import sys, time
import ctcsound

HEADER = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
"""
cs = ctcsound.Csound()
for option in ('-n', '-d', '-m0'):
    cs.set_option(option)
'''

# The main thread keeps the GIL until it blocks, so the performance thread is
# stopped inside the first callback, at the GIL, when that callback is replaced.
RESET_WHILE_WAITING = PRELUDE + '''
sys.setswitchinterval(60)
first, second = [], []
cs.set_output_channel_callback(lambda *_args: first.append(1))
cs.compile_orc(HEADER + """instr 1
    outvalue("x", timeinstk())
endin
""")
cs.event_string('i 1 0 100')
cs.start()
thread = ctcsound.CsoundPerformanceThread(cs.csound())
thread.play()
while not first:
    time.sleep(0.001)
held = time.monotonic() + 0.05
while time.monotonic() < held:
    pass
cs.set_output_channel_callback(lambda *_args: second.append(1))
deadline = time.monotonic() + 10
while not second and time.monotonic() < deadline:
    time.sleep(0.001)
thread.stop()
thread.join()
sys.exit(0 if second else 3)
'''

# Registering a function again updates its entry, and does not add a second.
KEYBOARD = PRELUDE + '''
calls = []
def key(user_data, _data, _type):
    calls.append(user_data)
    return 0
event = ctcsound.CSOUND_CALLBACK_KBD_EVENT.value
cs.register_keyboard_callback(key, 'first', event)
cs.register_keyboard_callback(key, 'second', event)
cs.compile_orc(HEADER + """instr 1
    k_key:k, k_down:k = sensekey()
endin
schedule(1, 0, 1)
""")
cs.start()
cs.perform_ksmps()
assert calls == ['second'], calls
cs.remove_keyboard_callback(key)
cs.perform_ksmps()
assert calls == ['second'], calls
'''

# Csound keeps the name and the types as well, which Python would reuse.
APPENDED_OPCODE = PRELUDE + '''
calls = []
def opcode(_csound, _data):
    calls.append(1)
    return 0
cs.append_opcode('hostop', 256, 0, 'kk', 'kk', opcode, opcode, opcode)
reused = [b'%d' % number for number in range(100000)]
assert any((entry['opname'], entry['outypes'], entry['intypes']) == ('hostop', 'kk', 'kk')
           for entry in ctcsound.UgenFactory(cs).list_opcodes())
assert cs.compile_orc(HEADER + """instr 1
    k_first:k, k_second:k = hostop(1, 2)
endin
schedule(1, 0, 1)
""") == 0
cs.start()
cs.perform_ksmps()
assert calls, 'the opcode was not called'
'''

# Csound takes no NULL here, and goes on calling what it has.
MESSAGE_CLEARED = PRELUDE + '''
messages = []
cs.set_message_string_callback(lambda _csound, _attr, message: messages.append(message))
cs.set_message_string_callback(0)
cs.compile_orc(HEADER)
cs.start()
assert messages, 'no message'
'''

# Csound saves the callback it finds at the start, and puts it back at the end.
MESSAGE_REPLACED_IN_REALTIME = PRELUDE + '''
first, second = [], []
cs.set_message_string_callback(lambda _csound, _attr, message: first.append(message))
cs.set_option('--realtime')
cs.compile_orc(HEADER)
cs.start()
cs.set_message_string_callback(lambda _csound, _attr, message: second.append(message))
cs.reset()
'''


class Callback:
    def __call__(self, *_args):
        return 0


class CallbackLifetimeTests(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.addCleanup(self.cs.destroy)

    def run_alone(self, script):
        environment = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        result = subprocess.run([sys.executable, '-c', script], env=environment,
                                stdin=subprocess.DEVNULL, capture_output=True,
                                text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr[-2000:])

    def test_a_callback_can_be_replaced_while_a_thread_waits_in_it(self):
        self.run_alone(RESET_WHILE_WAITING)

    def test_every_setter_keeps_the_callback_it_replaces(self):
        setters = [name for name in dir(ctcsound.Csound)
                   if name.startswith('set_') and name.endswith('_callback')]
        self.assertGreaterEqual(len(setters), 17)
        for name in setters:
            with self.subTest(name):
                replaced = Callback()
                kept = weakref.ref(replaced)
                getattr(self.cs, name)(replaced)
                del replaced
                getattr(self.cs, name)(Callback())
                self.assertIsNotNone(kept())

    def test_a_keyboard_callback_lives_until_it_is_removed(self):
        self.run_alone(KEYBOARD)

    def test_a_keyboard_callback_keeps_its_user_data(self):
        user_data = Callback()
        kept = weakref.ref(user_data)
        self.cs.register_keyboard_callback(Callback(), user_data,
                                           ctcsound.CSOUND_CALLBACK_KBD_EVENT.value)
        del user_data
        self.assertIsNotNone(kept())

    def test_an_appended_opcode_lives(self):
        self.run_alone(APPENDED_OPCODE)

    def test_a_message_callback_csound_will_not_clear_lives(self):
        self.run_alone(MESSAGE_CLEARED)

    def test_a_message_callback_csound_saved_lives(self):
        self.run_alone(MESSAGE_REPLACED_IN_REALTIME)


if __name__ == '__main__':
    unittest.main()
