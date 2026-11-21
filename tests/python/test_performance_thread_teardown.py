"""A CsoundPerformanceThread joined or destroyed on itself, from one of its own
callbacks, refuses rather than tears the thread down under the performance. A
failure here is a crash, so each case runs in a process of its own, with a
watchdog."""

import os
import subprocess
import sys
import unittest

PYTHON_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)

PRELUDE = '''
import faulthandler, threading, time
faulthandler.dump_traceback_later(20, exit=True)
import ctcsound

cs = ctcsound.Csound()
for option in ('-n', '-d', '-m0'):
    cs.set_option(option)
assert cs.compile_orc("""sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
    outvalue("x", timeinstk())
endin
""") == 0
cs.event_string('i 1 0 100000')
assert cs.start() == 0
thread = ctcsound.CsoundPerformanceThread(cs.csound())

def wait_for(condition):
    for _ in range(500):
        if condition():
            return True
        time.sleep(0.01)
    return False
'''

# join() in a callback on the thread returns at once, the thread goes on, and
# its Csound still stops and joins it before csoundDestroy.
JOINED_ON_ITS_THREAD = PRELUDE + '''
joined, calls, running_at_destroy = [], [], []
real_destroy = ctcsound.libcsound.csoundDestroy
def destroy(pointer):
    running_at_destroy.append(thread.is_running())
    real_destroy(pointer)
ctcsound.libcsound.csoundDestroy = destroy
def callback(*_args):
    calls.append(1)
    if not joined:
        joined.append(thread.join())
cs.set_output_channel_callback(callback)
thread.play()
assert wait_for(lambda: len(calls) > 10), 'the performance stopped'
assert joined == [0], joined
assert thread.is_running()
cs.destroy()
assert running_at_destroy == [False], 'csoundDestroy ran under the thread'
'''

# csoundDestroyPerformanceThread in a callback on the thread stops it, and the
# object, left whole, is joined and destroyed afterwards from this thread.
DESTROYED_ON_ITS_THREAD = PRELUDE + '''
destroyed = []
def callback(*_args):
    if not destroyed:
        destroyed.append(1)
        ctcsound.libcspt.csoundDestroyPerformanceThread(thread.cpt)
cs.set_output_channel_callback(callback)
thread.play()
assert wait_for(lambda: not thread.is_running()), 'the performance went on'
assert thread.join() > 0
del thread
cs.destroy()
'''


class PerformanceThreadTeardownTests(unittest.TestCase):

    def run_alone(self, script):
        environment = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        result = subprocess.run([sys.executable, '-c', script], env=environment,
                                stdin=subprocess.DEVNULL, capture_output=True,
                                text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr[-2000:])

    def test_a_join_on_its_thread_returns_at_once(self):
        self.run_alone(JOINED_ON_ITS_THREAD)

    def test_a_destroy_on_its_thread_only_stops_it(self):
        self.run_alone(DESTROYED_ON_ITS_THREAD)


if __name__ == '__main__':
    unittest.main()
