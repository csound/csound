"""What ctcsound hands a CSOUND stays alive for as long as that CSOUND can use
it, whichever wrapper handed it over, and a CSOUND outlives the performance
thread on it. A failure here is often a crash, so most of these run in a process
of their own."""

import ctypes as ct
import gc
import os
import subprocess
import sys
import unittest
import weakref

PYTHON_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)
import ctcsound

PRELUDE = '''
import ctypes as ct, gc, sys, threading, time
import ctcsound

HEADER = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
"""
OUTVALUE = HEADER + """instr 1
    outvalue("x", timeinstk())
endin
"""
def configure(cs):
    for option in ('-n', '-d', '-m0'):
        cs.set_option(option)
    return cs
cs = configure(ctcsound.Csound())
calls = []
'''

# Nothing else holds the callback once the wrapper it was set through has gone.
SET_THROUGH_A_WRAPPER = PRELUDE + '''
def set_it():
    ctcsound.Csound(pointer_=cs.csound()).set_output_channel_callback(lambda *_args: calls.append(1))
set_it()
gc.collect()
assert cs.compile_orc(OUTVALUE + 'schedule(1, 0, 1)\\n') == 0
cs.start()
cs.perform_ksmps()
assert calls, 'the callback was not called'
'''

# The same on a CSOUND no Csound owns: only the library's handle can make one.
SET_ON_ONE_MADE_IN_C = PRELUDE + '''
pointer = ctcsound.libcsound.csoundCreate(ct.py_object(None), None)
held = configure(ctcsound.Csound(pointer_=pointer))
def set_it():
    ctcsound.Csound(pointer_=pointer).set_output_channel_callback(lambda *_args: calls.append(1))
set_it()
gc.collect()
assert held.compile_orc(OUTVALUE + 'schedule(1, 0, 1)\\n') == 0
held.start()
held.perform_ksmps()
assert calls, 'the callback was not called'
ctcsound.libcsound.csoundDestroy(pointer)
'''

REMOVED_THROUGH_ANOTHER_WRAPPER = PRELUDE + '''
def key(_user_data, _data, _type):
    calls.append(1)
    return 0
ctcsound.Csound(pointer_=cs.csound()).register_keyboard_callback(key, None, ctcsound.CSOUND_CALLBACK_KBD_EVENT)
assert cs.compile_orc(HEADER + """instr 1
    k_key:k, k_down:k = sensekey()
endin
schedule(1, 0, 1)
""") == 0
cs.start()
cs.perform_ksmps()
assert calls == [1], calls
ctcsound.Csound(pointer_=cs.csound()).remove_keyboard_callback(key)
cs.perform_ksmps()
assert calls == [1], calls
'''

# Csound calls a saved callback while it is destroyed. A wrapper's destroy()
# from inside it finds the instance going, and must leave what is kept.
WRAPPER_DESTROY_INSIDE_DESTROY = PRELUDE + '''
during = []
def first(csound, _attr, _message):
    if cs._destroying:
        ctcsound.Csound(pointer_=csound).destroy()
        during.append(ctcsound.Csound(pointer_=csound)._kept is cs._kept)
cs.set_message_string_callback(first)
cs.set_option('-m7')
cs.set_option('--realtime')
assert cs.compile_orc(HEADER) == 0
assert cs.start() == 0
cs.set_message_string_callback(lambda *_args: None)
cs.destroy()
assert len(during) > 1, len(during)
assert all(during), 'a later wrapper lost the owner'
'''

# The same the other way round: the owner's destroy() from inside a wrapper's.
OWNER_DESTROY_INSIDE_WRAPPER_DESTROY = PRELUDE + '''
going, kept = [], []
def first(_csound, _attr, _message):
    if going:
        cs.destroy()
        kept.append(len(cs._kept))
cs.set_message_string_callback(first)
cs.set_option('-m7')
cs.set_option('--realtime')
assert cs.compile_orc(HEADER) == 0
assert cs.start() == 0
cs.set_message_string_callback(lambda *_args: None)
going.append(1)
ctcsound.Csound(pointer_=cs.csound()).destroy()
assert kept and min(kept) > 0, kept
'''

# A collection clears weakrefs first, so a wrapper in the same cycle cannot find
# its owner, and must still take itself for what it is.
WRAPPER_DESTROY_INSIDE_A_COLLECTION = PRELUDE + '''
kept = []
def start():
    instance = configure(ctcsound.Csound())
    wrapper = ctcsound.Csound(pointer_=instance.csound())
    def first(_csound, _attr, _message):
        if instance._destroying:
            wrapper.destroy()
            kept.append(len(wrapper._kept))
    instance.set_message_string_callback(first)
    instance.set_option('-m7')
    instance.set_option('--realtime')
    assert instance.compile_orc(HEADER) == 0
    assert instance.start() == 0
    instance.set_message_string_callback(lambda *_args: None)
start()
gc.collect()
assert kept and min(kept) > 0, kept
'''

# A process callback runs outside the API lock, so this holds the thread on the
# instance without holding off csoundDestroy, whenever the host lets it go.
THREAD_PRELUDE = PRELUDE + '''
parked, release = threading.Event(), threading.Event()
destroyed_while_parked = []
real_destroy = ctcsound.libcsound.csoundDestroy
def destroy(pointer):
    destroyed_while_parked.append(not release.is_set())
    real_destroy(pointer)
ctcsound.libcsound.csoundDestroy = destroy
def process(_data):
    parked.set()
    release.wait()
def play(csound):
    csound.compile_orc(HEADER + "instr 1\\nendin\\n")
    csound.event_string('i 1 0 100000')
    csound.start()
    thread = ctcsound.CsoundPerformanceThread(csound.csound())
    thread.set_process_cb(process, None)
    thread.play()
    parked.wait()
    return thread
'''

DROPPED_UNDER_A_THREAD = THREAD_PRELUDE + '''
thread = play(cs)
del cs
assert not destroyed_while_parked, 'the Csound was destroyed with its thread on it'
release.set()
time.sleep(0.2)
assert thread.is_running(), 'the thread stopped: status %d' % thread.status()
thread.stop()
thread.join()
assert destroyed_while_parked == [False], destroyed_while_parked
'''

DESTROYED_UNDER_A_THREAD = THREAD_PRELUDE + '''
thread = play(cs)
threading.Timer(0.2, release.set).start()
cs.destroy()
assert destroyed_while_parked == [False], 'destroyed with its thread on it'
assert not thread.is_running()
'''

DESTROYED_THROUGH_A_WRAPPER = THREAD_PRELUDE + '''
thread = play(cs)
threading.Timer(0.2, release.set).start()
ctcsound.Csound(pointer_=cs.csound()).destroy()
assert destroyed_while_parked == [False], 'destroyed with its thread on it'
assert not thread.is_running()
'''

# A thread can be made on a pointer no wrapper has yet.
THREAD_BEFORE_ANY_WRAPPER = PRELUDE + '''
pointer = ctcsound.libcsound.csoundCreate(ct.py_object(None), None)
thread = ctcsound.CsoundPerformanceThread(pointer)
assert thread.is_running()
ctcsound.Csound(pointer_=pointer).destroy()
assert not thread.is_running(), 'destroyed with its thread on it'
'''

# In a collection a wrapper cannot find its owner, which will destroy the CSOUND
# itself, after stopping a thread only it knows of.
WRAPPER_DESTROYED_IN_A_COLLECTION_WITH_A_THREAD = THREAD_PRELUDE + '''
class Host:
    def __init__(self, csound):
        self.csound = csound
        self.wrapper = ctcsound.Csound(pointer_=csound.csound())
        self.csound.set_message_string_callback(self.message)
        self.thread = play(self.csound)
    def message(self, *_args):
        pass
    def __del__(self):
        self.wrapper.destroy()
host = Host(cs)
del cs, host
threading.Timer(0.2, release.set).start()
gc.collect()
assert destroyed_while_parked == [False], destroyed_while_parked
'''

# An interrupt while destroy() waits for a thread leaves the CSOUND alive.
INTERRUPTED_WHILE_STOPPING_A_THREAD = THREAD_PRELUDE + '''
thread = play(cs)
pointer = cs.csound()
real_join = ctcsound.libcspt.csoundPerformanceThreadJoin
def interrupted(_thread):
    raise KeyboardInterrupt
ctcsound.libcspt.csoundPerformanceThreadJoin = interrupted
try:
    cs.destroy()
except KeyboardInterrupt:
    pass
assert cs.csound() == pointer, 'a CSOUND still alive was forgotten'
assert cs._kept, 'what a CSOUND still alive can call was released'
ctcsound.libcspt.csoundPerformanceThreadJoin = real_join
release.set()
thread.stop()
thread.join()
'''

# Both go in one collection, which finalizes them in no order it promises.
COLLECTED_WITH_ITS_THREAD = THREAD_PRELUDE + '''
class Host:
    def __init__(self, csound):
        self.csound = csound
        self.csound.set_message_string_callback(self.message)
        self.thread = play(self.csound)
    def message(self, *_args):
        pass
host = Host(cs)
del cs, host
threading.Timer(0.2, release.set).start()
gc.collect()
assert destroyed_while_parked == [False], destroyed_while_parked
'''

# destroy() stops what is on its list: a thread that went must be off it.
DROPPED_THREAD_IS_FORGOTTEN = THREAD_PRELUDE + '''
thread = play(cs)
release.set()
del thread
assert not cs._threads, 'a thread that went is still to be stopped'
cs.destroy()
'''

JOINED_THREAD_LETS_GO = THREAD_PRELUDE + '''
gone = __import__('weakref').ref(cs)
thread = play(cs)
release.set()
del cs
assert gone() is not None
thread.stop()
thread.join()
assert gone() is None, 'a joined thread still holds its Csound'
'''

CLOSING_OVER_ITSELF = PRELUDE + '''
gone = __import__('weakref').ref(cs)
cs.set_output_channel_callback(lambda *_args: cs)
del cs
gc.collect()
assert gone() is None, 'a callback closing over its Csound keeps it for ever'
'''


class Data:
    pass


class HostDataTests(unittest.TestCase):

    def test_host_data_given_at_creation_is_held(self):
        data = Data()
        kept = weakref.ref(data)
        cs = ctcsound.Csound(host_data=data)
        self.addCleanup(cs.destroy)
        del data
        gc.collect()
        self.assertIsNotNone(kept())
        self.assertIs(cs.host_data(), kept())

    def test_host_data_set_later_is_held(self):
        cs = ctcsound.Csound()
        self.addCleanup(cs.destroy)
        data = Data()
        kept = weakref.ref(data)
        cs.set_host_data(data)
        del data
        gc.collect()
        self.assertIsNotNone(kept())
        self.assertIs(cs.host_data(), kept())

    def test_host_data_set_through_a_wrapper_is_held(self):
        cs = ctcsound.Csound()
        self.addCleanup(cs.destroy)
        data = Data()
        kept = weakref.ref(data)
        ctcsound.Csound(pointer_=cs.csound()).set_host_data(data)
        del data
        gc.collect()
        self.assertIsNotNone(kept())
        self.assertIs(cs.host_data(), kept())

    def test_reading_host_data_takes_no_reference_away(self):
        data = Data()
        held = [data] * 20  # So that a failure here is a number, not a crash.
        cs = ctcsound.Csound(host_data=data)
        self.addCleanup(cs.destroy)
        before = sys.getrefcount(data)
        for _ in range(10):
            cs.host_data()
        self.assertEqual(sys.getrefcount(data), before)
        self.assertEqual(len(held), 20)


class RegistryTests(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.addCleanup(self.cs.destroy)

    def test_a_pointer_of_any_form_finds_the_same_csound(self):
        pointer = ct.c_void_p(self.cs.csound())
        self.assertIs(ctcsound.Csound(pointer_=pointer)._kept, self.cs._kept)
        thread = ctcsound.CsoundPerformanceThread(pointer)
        self.assertIs(thread._csound, self.cs)
        thread.stop()
        thread.join()

    def test_a_destroyed_csound_is_forgotten(self):
        pointer = self.cs.csound()
        self.cs.destroy()
        self.assertNotIn(pointer, ctcsound.Csound._owners)

    def test_a_wrapper_destroying_it_is_the_owner_destroying_it(self):
        pointer = self.cs.csound()
        wrapper = ctcsound.Csound(pointer_=pointer)
        wrapper.destroy()
        self.assertIsNone(self.cs.csound())
        self.assertIsNone(wrapper.csound())
        self.assertNotIn(pointer, ctcsound.Csound._owners)

    def test_a_stale_wrapper_destroys_nothing(self):
        wrapper = ctcsound.Csound(pointer_=self.cs.csound())
        self.cs.destroy()
        wrapper.destroy()
        self.assertIsNone(wrapper.csound())

    def test_a_stale_wrapper_leaves_another_csound_at_its_address(self):
        wrapper = ctcsound.Csound(pointer_=self.cs.csound())
        stale = wrapper.csound()
        self.cs.destroy()
        other = ctcsound.Csound()
        self.addCleanup(other.destroy)
        ctcsound.Csound._owners[stale] = other  # What an address used again gives.
        self.addCleanup(ctcsound.Csound._owners.pop, stale, None)
        wrapper.destroy()
        self.assertIsNotNone(other.csound())

    def test_a_wrapper_of_a_csound_made_in_c_goes_when_it_is_destroyed(self):
        pointer = ctcsound.libcsound.csoundCreate(ct.py_object(None), None)
        wrapper = ctcsound.Csound(pointer_=pointer)
        kept = weakref.ref(wrapper)
        self.assertIn(pointer, ctcsound.Csound._c_owned)
        wrapper._keep(Data())
        wrapper.destroy()
        self.assertEqual(wrapper._kept, [])
        self.assertNotIn(pointer, ctcsound.Csound._owners)
        del wrapper
        gc.collect()
        self.assertIsNone(kept())


class InstanceLifetimeTests(unittest.TestCase):

    def run_alone(self, script):
        environment = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        result = subprocess.run([sys.executable, '-c', script], env=environment,
                                stdin=subprocess.DEVNULL, capture_output=True,
                                text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr[-2000:])

    def test_a_callback_outlives_the_wrapper_it_was_set_through(self):
        self.run_alone(SET_THROUGH_A_WRAPPER)

    def test_a_callback_on_a_csound_made_in_c_outlives_its_wrapper(self):
        self.run_alone(SET_ON_ONE_MADE_IN_C)

    def test_a_keyboard_callback_can_be_removed_through_another_wrapper(self):
        self.run_alone(REMOVED_THROUGH_ANOTHER_WRAPPER)

    def test_a_wrapper_destroyed_while_the_csound_goes_leaves_what_is_kept(self):
        self.run_alone(WRAPPER_DESTROY_INSIDE_DESTROY)

    def test_the_owner_destroyed_inside_a_wrapper_s_destroy_leaves_what_is_kept(self):
        self.run_alone(OWNER_DESTROY_INSIDE_WRAPPER_DESTROY)

    def test_a_wrapper_destroyed_inside_a_collection_leaves_what_is_kept(self):
        self.run_alone(WRAPPER_DESTROY_INSIDE_A_COLLECTION)

    def test_a_wrapper_destroyed_in_a_collection_leaves_it_to_the_owner(self):
        self.run_alone(WRAPPER_DESTROYED_IN_A_COLLECTION_WITH_A_THREAD)

    def test_an_interrupted_destroy_releases_nothing(self):
        self.run_alone(INTERRUPTED_WHILE_STOPPING_A_THREAD)

    def test_a_thread_made_before_any_wrapper_is_stopped_first(self):
        self.run_alone(THREAD_BEFORE_ANY_WRAPPER)

    def test_a_csound_dropped_under_a_thread_stays_until_it_is_joined(self):
        self.run_alone(DROPPED_UNDER_A_THREAD)

    def test_destroy_stops_a_thread_first(self):
        self.run_alone(DESTROYED_UNDER_A_THREAD)

    def test_destroy_through_a_wrapper_stops_a_thread_first(self):
        self.run_alone(DESTROYED_THROUGH_A_WRAPPER)

    def test_a_csound_collected_with_its_thread_goes_after_it(self):
        self.run_alone(COLLECTED_WITH_ITS_THREAD)

    def test_a_thread_that_went_is_not_stopped_again(self):
        self.run_alone(DROPPED_THREAD_IS_FORGOTTEN)

    def test_a_joined_thread_lets_its_csound_go(self):
        self.run_alone(JOINED_THREAD_LETS_GO)

    def test_a_callback_closing_over_its_csound_does_not_keep_it(self):
        self.run_alone(CLOSING_OVER_ITSELF)


if __name__ == '__main__':
    unittest.main()
