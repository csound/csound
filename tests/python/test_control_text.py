"""setctrl's slider label and flashtxt's text go to a wish process, so a
stand-in on PATH records what is sent to it. A string p-field given to
flashtxt used to be read and never sent; a name too long for the opcode's
buffer used to overrun it, and is refused."""

import os
import shutil
import stat
import sys
import tempfile
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound

# The first line answers control.c's "source nsliders.tk", which waits for one.
WISH = '''#!/bin/sh
echo ready
exec cat >> "$WISH_RECORD"
'''

FITS = 'f' * 99       # the most a 100-byte buffer holds
TOO_LONG = 'g' * 100


@unittest.skipUnless(sys.platform.startswith('linux') or sys.platform == 'darwin',
                     "control.c's opcodes are registered on Linux and macOS only")
class TestControlText(unittest.TestCase):

    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.dir, True)
        wish = os.path.join(self.dir, 'wish')
        with open(wish, 'w') as stream:
            stream.write(WISH)
        os.chmod(wish, os.stat(wish).st_mode | stat.S_IXUSR)
        self.record = os.path.join(self.dir, 'record.txt')
        open(self.record, 'w').close()    # read before the stand-in writes it
        self.addCleanup(os.environ.__setitem__, 'PATH', os.environ['PATH'])
        os.environ['PATH'] = self.dir + os.pathsep + os.environ['PATH']
        os.environ['WISH_RECORD'] = self.record
        self.addCleanup(os.environ.pop, 'WISH_RECORD', None)
        self.cs = ctcsound.Csound()
        for option in ('-n', '-d', '-m0'):
            self.cs.set_option(option)

    def tearDown(self):
        self.cs.reset()     # kills the stand-in, which has its input by then

    def sent(self, *commands):
        """Every command, as the stand-in received it; what control.c sends
        arrives when the stand-in has read it, so a reader waits for it."""
        deadline = time.monotonic() + 10
        while True:
            with open(self.record) as stream:
                lines = stream.read().splitlines()
            if all(command in lines for command in commands) or time.monotonic() > deadline:
                return lines
            time.sleep(0.02)

    def render(self, orchestra, *scores):
        self.assertEqual(self.cs.compile_orc(orchestra), ctcsound.CSOUND_SUCCESS)
        self.assertEqual(self.cs.start(), ctcsound.CSOUND_SUCCESS)
        for score in scores:
            self.cs.event_string(score)
        for _ in range(4):
            self.cs.perform_ksmps()

    def test_flashtxt_sends_a_string_pfield(self):
        self.render('''
            instr 1
                flashtxt 1, p4
            endin
        ''', 'i 1 0 0.01 "the pfield text"')
        command = 'settext 1 "the pfield text"'
        self.assertIn(command, self.sent(command))

    def test_every_form_of_a_label_and_a_text(self):
        strset_label = 'label by strset number'
        self.render(f'''
            strset 5, "{strset_label}"
            instr 1
                setctrl 1, p4, 4
                setctrl 2, 5, 4
                setctrl 3, 8, 4
                setctrl 4, "label by string", 4
                flashtxt 1, "text by string"
                flashtxt 2, 0
                flashtxt 3, p5
            endin
        ''', 'i 1 0 0.01 "label by pfield" "text by pfield"')
        commands = ('setlab 1 "label by pfield"',
                    f'setlab 2 "{strset_label}"',
                    'setlab 3 "Control 8"',
                    'setlab 4 "label by string"',
                    'settext 1 "text by string"',
                    'deltext 2',
                    'settext 3 "text by pfield"')
        sent = self.sent(*commands)
        for command in commands:
            self.assertIn(command, sent)

    def test_a_name_too_long_is_refused_and_one_of_the_limit_is_kept(self):
        self.render(f'''
            strset 5, "{TOO_LONG}"
            instr 1
                setctrl 1, "{FITS}", 4
                setctrl 2, "{TOO_LONG}", 4
                setctrl 3, 5, 4
                flashtxt 1, "{FITS}"
                flashtxt 2, "{TOO_LONG}"
                flashtxt 3, p4
            endin
        ''', f'i 1 0 0.01 "{TOO_LONG}"')
        commands = (f'setlab 1 "{FITS}"', f'settext 1 "{FITS}"',
                    'setlab 2 ""', 'setlab 3 ""', 'settext 2 ""', 'settext 3 ""')
        sent = self.sent(*commands)
        for command in commands:
            self.assertIn(command, sent)


if __name__ == '__main__':
    unittest.main()
