"""Check urandom descriptor ownership while the Csound instance is alive."""

import ctypes as ct
import ctypes.util
import errno
import os
import stat
import sys
import unittest


FD_DIRECTORY = "/proc/self/fd" if sys.platform.startswith("linux") else "/dev/fd"


@unittest.skipUnless(os.path.exists("/dev/urandom") and os.path.isdir(FD_DIRECTORY),
                     "requires a Unix random device and descriptor directory")
class TestUrandomLifetime(unittest.TestCase):
    def setUp(self):
        # An explicit path also allows checking a float or sanitizer build.
        library = os.environ.get("CSOUND_TEST_LIBRARY")
        if not library:
            library = ct.util.find_library("CsoundLib64" if sys.platform == "darwin"
                                           else "csound64")
        self.assertTrue(library, "Csound library was not found")
        self.lib = ct.CDLL(library)
        signatures = {
            "csoundCreate": ([ct.c_void_p, ct.c_char_p], ct.c_void_p),
            "csoundCompileCSD": ([ct.c_void_p, ct.c_char_p, ct.c_int, ct.c_int], ct.c_int),
            "csoundStart": ([ct.c_void_p], ct.c_int),
            "csoundPerformKsmps": ([ct.c_void_p], ct.c_int),
            "csoundReset": ([ct.c_void_p], None),
            "csoundDestroy": ([ct.c_void_p], None),
        }
        for name, (args, result) in signatures.items():
            function = getattr(self.lib, name)
            function.argtypes = args
            function.restype = result
        self.cs = self.lib.csoundCreate(None, None)
        self.assertTrue(self.cs)
        self.addCleanup(self.lib.csoundDestroy, self.cs)
        self.device = os.stat("/dev/urandom").st_rdev
        self.baseline = self.random_descriptors()

    def random_descriptors(self):
        count = 0
        for entry in os.listdir(FD_DIRECTORY):
            try:
                info = os.fstat(int(entry))
            except OSError as error:
                # listdir's own descriptor has already closed.
                if error.errno == errno.EBADF:
                    continue
                raise
            if stat.S_ISCHR(info.st_mode) and info.st_rdev == self.device:
                count += 1
        return count

    def start(self, body):
        csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
kCycle init 0
kBound init 1
kCycle += 1
kBound = kCycle
if kCycle > 1 then
  reinit RANDOM
endif
RANDOM:
iBound = i(kBound)
""" + body + """
endin
</CsInstruments>
<CsScore>
i 1 0 .0625
i 1 .125 .0625
f 0 .25
e
</CsScore>
</CsoundSynthesizer>
"""
        self.assertEqual(self.lib.csoundCompileCSD(self.cs, csd.encode(), 1, 0), 0)
        self.assertEqual(self.lib.csoundStart(self.cs), 0)

    def test_init_rate_closes_each_read(self):
        self.start("""
iValue urandom iBound, iBound
if iValue != iBound then
  exitnow -1
endif
rireturn
""")
        for _ in range(64):
            self.assertEqual(self.lib.csoundPerformKsmps(self.cs), 0)
            self.assertEqual(self.random_descriptors(), self.baseline)

    def test_reinit_reuses_and_note_end_closes_descriptors(self):
        self.start("""
kValue urandom iBound, iBound
aValue urandom iBound, iBound
rireturn
kAudio downsamp aValue
if kValue != kBound || kAudio != kBound then
  exitnowk -1
endif
""")
        peak = 0
        for cycle in range(64):
            self.assertEqual(self.lib.csoundPerformKsmps(self.cs), 0)
            opened = self.random_descriptors() - self.baseline
            self.assertIn(opened, (0, 2))
            peak = max(peak, opened)
            if cycle in (24, 56):
                self.assertEqual(opened, 0)
        self.assertEqual(peak, 2)

    def test_reset_closes_active_descriptors(self):
        self.start("""
kValue urandom
aValue urandom
rireturn
""")
        self.assertEqual(self.lib.csoundPerformKsmps(self.cs), 0)
        self.assertEqual(self.random_descriptors(), self.baseline + 2)
        self.lib.csoundReset(self.cs)
        self.assertEqual(self.random_descriptors(), self.baseline)

    def test_destroy_closes_active_descriptors(self):
        self.start("""
kValue urandom
aValue urandom
rireturn
""")
        self.assertEqual(self.lib.csoundPerformKsmps(self.cs), 0)
        self.assertEqual(self.random_descriptors(), self.baseline + 2)
        self.assertTrue(self.doCleanups())
        self.assertEqual(self.random_descriptors(), self.baseline)


if __name__ == "__main__":
    unittest.main()
