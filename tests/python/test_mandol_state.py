"""Check mandol control changes, interpolation, and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class MandolTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, frequency="p4", detune=".99", pluck=".4", rate=1,
               block=16, offset=0, fullscale=1, minimum=40, body=None,
               gain=".995", score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 220"
        if body is None:
            body = "1024,-7,0,8,1,24,-.5,96,0,896,0"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs={fullscale}
giBody ftgen 1,0,{body}
instr 1
kTime timeinsts
kFrequency init p4
kFrequency = {frequency}
kDetune = {detune}
kPluck = {pluck}
kGain = {gain}
aOut mandol .1*0dbfs,kFrequency,kPluck,kDetune,kGain,{rate},giBody,{minimum}
out aOut
endin
</CsInstruments>
<CsScore>
{score}
e
</CsScore>
</CsoundSynthesizer>
''')
        run = subprocess.run([str(self.executable), "-W", "-f", "-o",
                              str(wave), str(source)], capture_output=True,
                             text=True, cwd=self.root, timeout=30)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        raw = wave.read_bytes()
        position = 12
        while position + 8 <= len(raw):
            tag, size = struct.unpack_from("<4sI", raw, position)
            if tag == b"data":
                samples = struct.unpack_from(f"<{size//4}f", raw, position+8)
                self.assertTrue(all(map(math.isfinite, samples)))
                return samples
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    def assert_signal(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        self.assertLess(max(abs(a-b) for a, b in zip(actual, expected)), 2e-6)

    def test_detune_changes_at_fixed_pitch(self):
        fixed = self.render()
        changed = self.render(detune="(kTime < .0625 ? .99 : .95)")
        self.assert_signal(changed[:2000], fixed[:2000])
        self.assertGreater(max(abs(a-b) for a,b in
                               zip(changed[2200:4096], fixed[2200:4096])), .001)
        # A negligible pitch change must not be needed to apply detuning.
        nudged = self.render(detune="(kTime < .0625 ? .99 : .95)",
                             frequency="(kTime < .0625 ? p4 : p4+1e-7)")
        self.assert_signal(changed[:4096], nudged[:4096])

    def test_pluck_changes_during_excitation(self):
        body = "8192,-7,0,2048,1,2048,-1,4096,0"
        fixed = self.render(body=body)
        changed = self.render(body=body, pluck="(kTime < .0625 ? .4 : .8)")
        self.assert_signal(changed[:2000], fixed[:2000])
        self.assertGreater(max(abs(a-b) for a,b in
                               zip(changed[2200:4096], fixed[2200:4096])), .001)

    def test_fractional_body_interpolation(self):
        # Every position in the constant part of the table has the same value.
        body = "8192,-7,.3,8192,.3"
        reference = self.render(body=body, rate=1)
        for rate in (0, .5, 1.5):
            with self.subTest(rate=rate):
                actual = self.render(body=body, rate=rate)
                self.assert_signal(actual[:4096], reference[:4096])

    def test_first_pitch_and_note_reuse(self):
        fifty = self.render(score="i 1 0 .125 50")
        nearby = self.render(score="i 1 0 .125 50.0000001")
        self.assert_signal(fifty[:4096], nearby[:4096])
        actual = self.render(score="i 1 0 .125 50\ni 1 .25 .125 220\ni 1 .5 .125 50")
        self.assert_signal(actual[:4096], actual[16384:20480])
        fresh = self.render()
        self.assert_signal(actual[8192:12288], fresh[:4096])
        # The default minimum must leave room for detuning both strings.
        for detune in (".9", ".99", "1"):
            with self.subTest(detune=detune):
                self.assert_signal(self.render(minimum=0, detune=detune)[:4096],
                                   self.render(detune=detune)[:4096])

    def test_delay_updates_do_not_depend_on_buffer_capacity(self):
        frequency = "440+100*sin(kTime*10)"
        detune = ".95+.04*sin(kTime*15)"
        reference = self.render(frequency=frequency, detune=detune, minimum=40)
        actual = self.render(frequency=frequency, detune=detune, minimum=200)
        self.assert_signal(actual, reference)

    def test_release_keeps_feedback_damped(self):
        actual = self.render()
        expected = self.render(score="i 1 0 .25 220",
                               gain="(kTime <= .125 ? .995 : .45-220*.000005)")
        self.assert_signal(actual[:7000], expected[:7000])
        self.assertLess(max(map(abs, actual[6500:7000])), 1e-5)

    def test_partial_blocks_and_scale(self):
        reference = self.render()
        actual = self.render(offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        # Release begins on a control boundary, which shifts with the offset.
        self.assert_signal(actual[3:4003], reference[:4000])
        self.assert_signal(self.render(block=1)[:4000], reference[:4000])
        self.assert_signal(self.render(fullscale=32768)[:4096], reference[:4096])


if __name__ == "__main__":
    unittest.main()
