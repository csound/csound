"""Check spectrum sample handling and analysis history."""

import math
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


class SpectrumTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, block=16, phase=0, offset=0, delay=0, control=False,
               score=None):
        source = self.root / "test.csd"
        rate = "k" if control else "a"
        octaves = 3 if control else 4
        quality = 10 if control else 50
        if score is None:
            score = f"i 1 {offset/32768} .125 1 2048"
        delay_line = f"aSignal delay aTone,{delay/32768}" if delay else "aSignal = aTone"
        signal = "kSignal oscili .1,p5" if control else f"aTone oscili .1,p5,giSine,{phase}\n{delay_line}"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs=1
giSine ftgen 1,0,16384,10,1
instr 1
{signal}
wSpectrum spectrum {rate}Signal,.015625,{octaves},12,{quality},0,0,0
kSum specsum wSpectrum
kCount timeinstk
if kCount % (kr/64) == 0 then
 printks "TRACE %g %.12f\\n",0,p4,kSum
endif
endin
</CsInstruments>
<CsScore>
{score}
e
</CsScore>
</CsoundSynthesizer>
''')
        run = subprocess.run([str(self.executable), str(source)],
                             capture_output=True, text=True, timeout=30)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        traces = [(int(tag), float(value)) for tag,value in
                  re.findall(r"TRACE ([\d]+) ([\d.eE+-]+)", run.stdout+run.stderr)]
        self.assertTrue(traces, run.stdout+run.stderr)
        self.assertTrue(all(math.isfinite(value) for _,value in traces))
        return traces

    def assert_trace(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        self.assertLess(max(abs(a-b) for (_,a),(_,b) in zip(actual,expected)), 2e-6)

    def test_audio_block_sizes(self):
        reference = self.render(block=1)
        self.assertGreater(reference[-1][1], .01)
        for block in (16,64):
            self.assert_trace(self.render(block=block), reference)

    def test_steady_tone_phase(self):
        sine = self.render()
        cosine = self.render(phase=.25)
        self.assertLess(abs(sine[-1][1]-cosine[-1][1]), .001)

    def test_reused_note_history(self):
        traces = self.render(score="i 1 0 .125 1 2048\n"
                                   "i 1 .25 .125 2 3000\n"
                                   "i 1 .5 .125 3 2048")
        first = [x for x in traces if x[0] == 1]
        repeated = [x for x in traces if x[0] == 3]
        self.assert_trace(first, repeated)
        middle = [x for x in traces if x[0] == 2]
        self.assert_trace(middle, self.render(score="i 1 0 .125 2 3000"))

    def test_audio_start_offset(self):
        self.assert_trace(self.render(offset=3), self.render(delay=3))

    def test_control_input_partial_note(self):
        reference = self.render(control=True, score="i 1 0 .125 1 220")
        actual = self.render(control=True,
                             score="i 1 0.000091552734375 .1251 1 220")
        self.assert_trace(actual, reference)
        self.assertGreater(actual[-1][1], .01)


if __name__ == "__main__":
    unittest.main()
