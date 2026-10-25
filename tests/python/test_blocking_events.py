"""Score lines a host sends blocking - csoundEventString with async 0 - while
a performance thread performs: they are appended to the line-event buffer on
the host's thread while the performance thread empties it at the top of each
k-cycle, and with nothing locking the two, a quarter of them were lost."""

import os
import subprocess
import sys
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound


EVENTS = 3000

# Run in a process of its own, which a deadlock would leave spinning.
GROWING = '''
import sys
sys.path.insert(0, {path!r})
import ctcsound

cs = ctcsound.Csound()
for option in ('-n', '-d', '-m0'):
    cs.set_option(option)
sent = []

def callback(_csound, _attr, message):
    if b"realloc" in message and not sent:
        sent.append(True)
        cs.event_string('i "Count" 0 0', False)

cs.set_message_string_callback(callback)
cs.compile_orc("""
    sr = 48000
    ksmps = 64
    nchnls = 1
    0dbfs = 1
    instr Count
        chnset:i(chnget:i("count") + 1, "count")
        turnoff
    endin
""")
cs.start()
cs.set_control_channel("count", 0)
cs.set_debug(99)
cs.event_string('i "Count" 0 0\\n' * {events}, False)
cs.set_debug(0)
for _ in range(8):
    cs.perform_ksmps()
print(bool(sent), int(cs.control_channel("count")[0]))
'''


class TestBlockingEventsFromAnotherThread(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)

    def tearDown(self):
        self.cs.reset()

    def test_every_event_sent_while_performing_arrives(self):
        self.assertEqual(self.cs.compile_orc('''
            sr = 48000
            ksmps = 64
            nchnls = 1
            0dbfs = 1
            instr Count
                chnset:i(chnget:i("count") + 1, "count")
                turnoff
            endin
        '''), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        self.cs.set_control_channel("count", 0)
        thread = ctcsound.CsoundPerformanceThread(self.cs.csound())
        thread.play()
        try:
            for _ in range(EVENTS):
                self.cs.event_string('i "Count" 0 0', False)
            deadline = time.monotonic() + 10
            while (self.cs.control_channel("count")[0] < EVENTS
                   and time.monotonic() < deadline):
                time.sleep(0.05)
        finally:
            thread.stop()
            thread.join()
        self.assertEqual(int(self.cs.control_channel("count")[0]), EVENTS)


    def test_a_message_callback_may_send_an_event(self):
        """What sense_line says - here an -L line it cannot read - reaches
        the message callback on the performance thread, and a callback may
        send an event from there: the line-event lock is not held while the
        lines are read, as it is not recursive."""
        import tempfile
        with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as lines:
            lines.write("z 1 0 0\n")
        try:
            self.cs.set_option(f"--score-in={lines.name}")
            sent = []

            def callback(_csound, _attr, message):
                if b"unknown opcode" in message and not sent:
                    sent.append(True)
                    self.cs.event_string('i "Count" 0 0', False)

            self.cs.set_message_string_callback(callback)
            self.assertEqual(self.cs.compile_orc('''
                sr = 48000
                ksmps = 64
                nchnls = 1
                0dbfs = 1
                instr Count
                    chnset:i(chnget:i("count") + 1, "count")
                    turnoff
                endin
            '''), ctcsound.CSOUND_SUCCESS)
            self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
            self.cs.set_control_channel("count", 0)
            for _ in range(8):
                self.cs.perform_ksmps()
        finally:
            os.remove(lines.name)
        self.assertTrue(sent, "the bad line was not reported")
        self.assertEqual(int(self.cs.control_channel("count")[0]), 1)

    def test_a_message_from_a_growing_buffer_may_send_an_event(self):
        """An append that grows the buffer says so in a debug message, which
        reaches the message callback on the sending thread, and a callback may
        send an event from there: the append's messages wait until the lock is
        let go. The base version wrote the callback's line into the middle of
        the append, and segfaulted."""
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            '..', '..', 'languages', 'python')
        result = subprocess.run(
            [sys.executable, "-c", GROWING.format(path=path, events=EVENTS)],
            capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr[-2000:])
        self.assertEqual(result.stdout.split()[-2:], ["True", str(EVENTS + 1)])


if __name__ == '__main__':
    unittest.main()
