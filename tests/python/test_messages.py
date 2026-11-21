"""Check the text passed from ctcsound to Csound's message buffer."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

PYTHON_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python')
sys.path.insert(0, PYTHON_DIR)
import ctcsound

# From msg_attr.h
TYPE_MASK = 0x7000
ERROR = 0x1000
INFO = 0x6000

ORCHESTRA = """sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
  out(oscili:a(0.1, 440))
endin
instr 2
  y:i = ftlen(99)
endin
"""

# Csound's own report of a run: what it says whatever the run did.
REPORT = ('--Csound version', 'ksmps = 64', 'new alloc for instr 1',
          'SECTION 2', 'overall amps', 'Elapsed time')

# A run, its messages echoed by the buffer: argv is ctcsound's directory and
# the orchestra.
ECHOED = """
import sys
sys.path.insert(0, sys.argv[1])
import ctcsound
cs = ctcsound.Csound()
cs.create_message_buffer(True)
for option in ('-n', '-d'):
    cs.set_option(option)
cs.compile_orc(sys.argv[2])
cs.event_string('i 1 0 0.01\\ns\\ni 1 0 0.01\\ne\\n')
cs.start()
while cs.perform_ksmps() == 0:
    pass
cs.reset()
"""


class TestMessages(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.cs.create_message_buffer(False)

    def tearDown(self):
        self.cs.destroy_message_buffer()
        self.cs.destroy()

    def test_message_percent_format(self):
        self.cs.message('value %d, %s\n', 42, '50% café')
        self.assertEqual(self.cs.first_message(), 'value 42, 50% café\n')

    def test_message_brace_format(self):
        self.cs.message('{} voices, {}\n', 3, '50% café')
        self.assertEqual(self.cs.first_message(), '3 voices, 50% café\n')

    def test_message_s_percent_format(self):
        self.cs.message_S(0, 'format %d\n', 42)
        self.assertEqual(self.cs.first_message(), 'format 42\n')
        self.assertEqual(self.cs.first_message_attr(), 0)

    def test_message_s_brace_format_preserves_attributes(self):
        attributes = 0x4000  # CSOUNDMSG_WARNING from msg_attr.h
        self.cs.message_S(attributes, '{} voices, {}\n', 3, '50% café')
        self.assertEqual(self.cs.first_message(), '3 voices, 50% café\n')
        self.assertEqual(self.cs.first_message_attr(), attributes)

    def performed(self, score):
        """A run of ORCHESTRA's score to its end, and what it said, as
        (type, text): the statistics come at the reset."""
        for option in ('-n', '-d'):
            self.cs.set_option(option)
        self.cs.compile_orc(ORCHESTRA)
        self.cs.event_string(score)
        self.cs.start()
        while self.cs.perform_ksmps() == 0:
            pass
        self.cs.reset()
        said = []
        while self.cs.message_cnt():
            said.append((self.cs.first_message_attr() & TYPE_MASK, self.cs.first_message()))
            self.cs.pop_first_message()
        return said

    def test_a_run_reports_itself_as_information(self):
        said = self.performed('i 1 0 0.01\ns\ni 1 0 0.01\ne\n')
        errors = ''.join(text for kind, text in said if kind == ERROR)
        information = ''.join(text for kind, text in said if kind == INFO)
        for line in REPORT:
            self.assertNotIn(line, errors)
            self.assertIn(line, information)

    def test_an_init_error_is_still_an_error(self):
        said = self.performed('i 2 0 0.01\ne\n')
        errors = ''.join(text for kind, text in said if kind == ERROR)
        self.assertIn('INIT ERROR in instr 2', errors)

    def test_echoed_information_goes_where_errors_go(self):
        """The buffer's echo sends errors to stderr and the rest to stdout,
        on every platform: information goes with the errors it was sent as,
        not into a host's stdout."""
        result = subprocess.run([sys.executable, '-c', ECHOED, PYTHON_DIR, ORCHESTRA],
                                capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr)
        for line in REPORT:
            self.assertIn(line, result.stderr)
            self.assertNotIn(line, result.stdout)

    def test_the_command_keeps_information_off_stdout(self):
        """The default callback, the csound command's: on Windows it sends the
        default type to stdout, where `-o stdout` writes the audio."""
        default = Path(__file__).resolve().parents[2] / 'build' / 'csound'
        executable = Path(os.environ.get('CSOUND_TEST_EXECUTABLE', default)).resolve()
        if not executable.is_file():
            self.skipTest('Set CSOUND_TEST_EXECUTABLE to the built csound command')
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'clean.csd'
            source.write_text('<CsoundSynthesizer>\n<CsInstruments>\n' + ORCHESTRA
                              + '</CsInstruments>\n<CsScore>\ni 1 0 0.01\ns\ni 1 0 0.01\n'
                              'e\n</CsScore>\n</CsoundSynthesizer>\n')
            result = subprocess.run([str(executable), '-n', '-d', str(source)],
                                    cwd=directory, capture_output=True, text=True,
                                    timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        for line in REPORT:
            self.assertIn(line, result.stderr)
            self.assertNotIn(line, result.stdout)


if __name__ == '__main__':
    unittest.main()
