"""Check ATS buffered partial selection and frequency scaling."""

import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class ATSBufreadTests(unittest.TestCase):
    def test_partial_selection(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            score = []
            strings = []
            case = 0
            for kind in range(1, 5):
                for endian, suffix in (("<", "le"), (">", "be")):
                    name = f"partials-{kind}-{suffix}.ats"
                    header = [123, 8192, 4096, 8192, 4, 2, .8, 2640, 1, kind]
                    data = []
                    for frame in range(2):
                        data.append(frame*.5)
                        for partial in range(1, 5):
                            data.extend((partial*.1*(1+frame), partial*(440+220*frame)))
                            if kind in (2, 4):
                                data.append(0)  # Phase.
                        if kind in (3, 4):
                            data.extend([0]*25)  # Noise-band energies.
                    values = header + data
                    (root / name).write_bytes(struct.pack(endian + "d"*len(values), *values))
                    number = kind*2 + (suffix == "be") + 100
                    strings.append(f'strset {number}, "{name}"')
                    for count, first, step in ((4, 0, 1), (2, 0, 2), (2, 1, 2)):
                        for numeric in (False, True):
                            filename = str(number) if numeric else f'"{name}"'
                            score.append(f'i {2 if numeric else 1} {case*.03125} '
                                         f'.01171875 {filename} {count} {first} {step}')
                            case += 1
            instruments = []
            for number in (1, 2):
                filename = 'SFile strget p4\n' if number == 1 else ''
                argument = 'SFile' if number == 1 else 'p4'
                # Tap the first and last selected partial, preserving selection order.
                instruments.append(f'''instr {number}
  {filename}
  kCycle init 0
  kTime = (kCycle % 3)*.25
  kMultiplier = (kCycle < 3 ? 2 : .5)
  ATSbufread kTime, kMultiplier, {argument}, p5, p6, p7
  kFreq1, kAmp1 ATSpartialtap 1
  kFreq2, kAmp2 ATSpartialtap p5
  kFrac = kTime*2
  iFirst = p6+1
  iLast = p6+1+(p5-1)*p7
  kExpected1 = iFirst*(440+220*kFrac)*kMultiplier
  kExpected2 = iLast*(440+220*kFrac)*kMultiplier
  kError = abs(kFreq1-kExpected1) + abs(kFreq2-kExpected2)
  kError += abs(kAmp1-iFirst*.1*(1+kFrac)) + abs(kAmp2-iLast*.1*(1+kFrac))
  if !(kError < .001) then
    printks "ATS mismatch: first=%g step=%g time=%g multiplier=%g freq=%g,%g expected=%g,%g\\n", 0, p6, p7, kTime, kMultiplier, kFreq1, kFreq2, kExpected1, kExpected2
    exitnowk(-1)
  endif
  kCycle += 1
  if kCycle == 6 then
    gkChecks += 1
  endif
endin
''')
            csd = '''<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
''' + '\n'.join(strings) + '\n' + ''.join(instruments) + f'''
instr 99
  if i(gkChecks) != {case} then
    prints "ATS checks did not complete\\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
''' + '\n'.join(score) + f'''
i 99 {case*.03125} .01
e
</CsScore>
</CsoundSynthesizer>
'''
            (root / "test.csd").write_text(csd)
            result = subprocess.run([str(executable), "test.csd"], cwd=root,
                                    capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
