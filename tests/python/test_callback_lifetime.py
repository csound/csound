"""ctcsound's callbacks stay alive for as long as Csound can call them. Csound
holds a bare pointer to the thunk ctypes makes for a callback, and a thread can
be inside one, waiting for the GIL, however late it is replaced: a thunk
released while C still has it is a crash, so most of these run in a process of
their own."""

import ctypes
import gc
import os
import re
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

# Objects that compare equal are two callbacks, hashable or not, bound or not,
# and two lookups of a bound method, or two equal addresses, are one.
KEYBOARD_IDENTITY = PRELUDE + '''
import ctypes, types
calls = []
class Key:
    def __init__(self, name):
        self.name = name
    def __eq__(self, other):
        return isinstance(other, Key)
    def __hash__(self):
        return 1
    def __call__(self, *args):
        calls.append((self.name, args[-3]))
        return 0
    def __getattr__(self, name):
        return name
    def method(self, user_data, _data, _type):
        calls.append(('method of ' + self.name, user_data))
        return 0
class Unhashable(Key):
    __hash__ = None
event = ctcsound.CSOUND_CALLBACK_KBD_EVENT.value
first, second, third = Key('first'), Key('second'), Unhashable('third')
cs.register_keyboard_callback(first, 'a', event)
cs.register_keyboard_callback(second, 'b', event)
cs.register_keyboard_callback(third, 'c', event)
cs.register_keyboard_callback(first.method, 'd', event)
cs.register_keyboard_callback(first.method, 'e', event)
fourth, fifth = Key('fourth'), Key('fifth')
cs.register_keyboard_callback(types.MethodType(fourth, first), 'f', event)
cs.register_keyboard_callback(types.MethodType(fifth, first), 'g', event)
thunk = ctcsound.KEYBOARDFUNC(Key('address'))
address = ctypes.cast(thunk, ctypes.c_void_p).value
cs.register_keyboard_callback(address, 'h', event)
cs.compile_orc(HEADER + """instr 1
    k_key:k, k_down:k = sensekey()
endin
schedule(1, 0, 1)
""")
cs.start()
cs.perform_ksmps()
assert calls == [('address', 'h'), ('fifth', 'g'), ('fourth', 'f'), ('method of first', 'e'),
                 ('third', 'c'), ('second', 'b'), ('first', 'a')], calls
calls.clear()
cs.remove_keyboard_callback(second)
cs.remove_keyboard_callback(third)
cs.remove_keyboard_callback(first.method)
cs.remove_keyboard_callback(types.MethodType(fourth, first))
cs.remove_keyboard_callback(types.MethodType(fifth, first))
cs.remove_keyboard_callback(int(str(address)))
cs.perform_ksmps()
assert calls == [('first', 'a')], calls
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


# Csound puts back the message callback it found at the start, and calls it
# while it is destroyed, once per message: that callback must still be kept
# then, the instance must still answer it, and a destroy() from inside it must
# do nothing.
DESTROY_FROM_A_KEPT_CALLBACK = PRELUDE + '''
during, errors = [], []
def first(_csound, _attr, message):
    try:
        if cs._destroying:
            during.append(cs.message_level())
            cs.destroy()
    except Exception as error:
        errors.append(error)
cs.set_message_string_callback(first)
cs.set_option('-m7')
cs.set_option('--realtime')
assert cs.compile_orc(HEADER) == 0
assert cs.start() == 0
cs.set_message_string_callback(lambda *_args: None)
cs.destroy()
assert not errors, errors
assert len(during) > 1, 'the replaced callback was called %d times while Csound was destroyed' % len(during)
assert set(during) == {7}, during
'''

# The performance thread holds a callback and a pointer of its own.
PROCESS_PRELUDE = PRELUDE + '''
import ctypes
cs.compile_orc(HEADER + """instr 1
endin
""")
cs.event_string('i 1 0 100')
cs.start()
thread = ctcsound.CsoundPerformanceThread(cs.csound())
def reader(values):
    return lambda address: values.append(ctypes.c_int.from_address(address).value)
def wait_for(values):
    deadline = time.monotonic() + 10
    while not values and time.monotonic() < deadline:
        time.sleep(0.001)
'''

# Neither the callback nor its data is held by anything but the thread.
PROCESS_CALLBACK = PROCESS_PRELUDE + '''
values = []
thread.set_process_cb(reader(values), ctypes.c_int(1))
thread.play()
wait_for(values)
thread.stop()
thread.join()
assert set(values) == {1}, set(values)
thread.set_process_cb(None, None)
assert thread.process_cb() is None
thread.set_process_cb(print, None)
assert thread.process_cb()
thread.set_process_cb(0, None)
assert thread.process_cb() is None
direct = ctcsound.PROCESSFUNC(print)
ctcsound.libcspt.csoundPerformanceThreadSetProcessCB(thread.cpt, direct, None)
address = lambda function: ctypes.cast(function, ctypes.c_void_p).value
assert address(thread.process_cb()) == address(direct)
'''

# Memory is not reused on demand, so only a weak reference shows data released.
PROCESS_DATA = PROCESS_PRELUDE + '''
import weakref
class Data(ctypes.c_int):
    pass
data = Data()
kept = weakref.ref(data)
thread.set_process_cb(reader([]), data)
del data
assert kept() is not None, 'data was released with the callback set'
thread.set_process_cb(reader([]), Data())
assert kept() is not None, 'data was released when it was replaced'
thread.stop()
thread.join()
'''

# What process_cb() returns is the callback, whatever became of the thread.
PROCESS_CALLBACK_READ_BACK = PROCESS_PRELUDE + '''
values, data = [], ctypes.c_int(1)
thread.set_process_cb(reader(values), data)
try:
    thread.set_process_cb(print, 'not a ctypes object')
except TypeError:
    pass
callback = thread.process_cb()
thread.stop()
thread.join()
del thread
reused = [ctcsound.PROCESSFUNC(print) for _ in range(100)]
callback(ctypes.addressof(data))
assert values == [1], values
'''

# As in RESET_WHILE_WAITING, the thread is stopped at the GIL inside the first
# callback, and has read the first data, when both are replaced.
PROCESS_REPLACED_WHILE_WAITING = PROCESS_PRELUDE + '''
sys.setswitchinterval(60)
first, second = [], []
thread.set_process_cb(reader(first), ctypes.c_int(1))
thread.play()
wait_for(first)
held = time.monotonic() + 0.05
while time.monotonic() < held:
    pass
thread.set_process_cb(reader(second), ctypes.c_int(2))
wait_for(second)
thread.stop()
thread.join()
assert set(first) == {1} and set(second) == {2}, (set(first), set(second))
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

    def test_keyboard_callbacks_are_told_apart_by_identity(self):
        self.run_alone(KEYBOARD_IDENTITY)

    def test_c_functions_and_methods_of_c_types_are_keyboard_callbacks_by_value(self):
        values = []
        pattern = re.compile('x')
        version = ('csoundGetVersion', ctcsound.libcsound)
        address = ctypes.cast(ctcsound.KEYBOARDFUNC(version), ctypes.c_void_p).value
        lookups = ((lambda: values.append, lambda: values.append),
                   (lambda: pattern.match, lambda: pattern.match),
                   (lambda: len.__call__, lambda: len.__call__),
                   (lambda: ('csoundGetSr', ctcsound.libcsound),
                    lambda: ('csoundGetSr', ctcsound.libcsound)),
                   (lambda: version, lambda: address))
        for pair in lookups:
            for lookup in pair:
                self.cs.register_keyboard_callback(lookup(), None,
                                                   ctcsound.CSOUND_CALLBACK_KBD_EVENT.value)
        self.assertEqual(len(self.cs._keyboard_thunks), len(lookups))

    def test_a_keyboard_callback_keeps_its_user_data(self):
        user_data = Callback()
        kept = weakref.ref(user_data)
        self.cs.register_keyboard_callback(Callback(), user_data,
                                           ctcsound.CSOUND_CALLBACK_KBD_EVENT.value)
        del user_data
        self.assertIsNotNone(kept())

    def test_destroy_releases_what_was_kept(self):
        replaced, function, user_data = Callback(), Callback(), Callback()
        kept = [weakref.ref(value) for value in (replaced, function, user_data)]
        self.cs.set_output_channel_callback(replaced)
        self.cs.set_output_channel_callback(Callback())
        self.cs.register_keyboard_callback(function, user_data,
                                           ctcsound.CSOUND_CALLBACK_KBD_EVENT.value)
        del replaced, function, user_data
        self.cs.destroy()
        self.cs.destroy()
        gc.collect()
        self.assertEqual([ref() for ref in kept], [None, None, None])

    def test_destroy_keeps_what_csound_calls_while_it_is_destroyed(self):
        self.run_alone(DESTROY_FROM_A_KEPT_CALLBACK)

    def test_an_appended_opcode_lives(self):
        self.run_alone(APPENDED_OPCODE)

    def test_a_message_callback_csound_will_not_clear_lives(self):
        self.run_alone(MESSAGE_CLEARED)

    def test_a_message_callback_csound_saved_lives(self):
        self.run_alone(MESSAGE_REPLACED_IN_REALTIME)

    def test_a_process_callback_lives(self):
        self.run_alone(PROCESS_CALLBACK)

    def test_a_process_callback_keeps_its_data(self):
        self.run_alone(PROCESS_DATA)

    def test_a_process_callback_read_back_outlives_its_thread(self):
        self.run_alone(PROCESS_CALLBACK_READ_BACK)

    def test_a_process_callback_can_be_replaced_while_a_thread_waits_in_it(self):
        self.run_alone(PROCESS_REPLACED_WHILE_WAITING)


if __name__ == '__main__':
    unittest.main()
