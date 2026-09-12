"""Compare pitchamdf with AMDF and RMS computed from a resampled sequence."""

import math
import os
from pathlib import Path
import statistics
import subprocess
import tempfile
import unittest


def reference(block, offset, count, factor, requested, median):
    up = -factor if factor < 0 else 1
    down = factor if factor > 0 else 1
    rate = 1024 * up / down
    if rate / requested < 1:
        requested = 64
    interval = max(requested, math.ceil(block / down) * up)
    raw = [(n * 7 % 31 - 15) / 32 for n in range(count)]
    samples = []
    if factor < 0:
        previous = 0
        for value in raw:
            samples.extend(previous + (value-previous) * j / up
                           for j in range(1, up+1))
            previous = value
    else:
        samples = raw[::down]
    width = median * 2 + 1
    periods = [0] * width
    medians = [0] * width
    rms_values = [0] * width
    rms_medians = [0] * width
    position = rms_position = 0
    pitch = 34
    next_analysis = 64 + interval
    consumed = 0
    result = []
    while consumed < count:
        consumed += min(block-offset if consumed == 0 else block, count-consumed)
        available = consumed * up if factor < 0 else (consumed+down-1) // down
        while next_analysis <= available:
            start = next_analysis - (64 + interval)
            scores = [sum(abs(samples[start+j+lag] - samples[start+j])
                          for j in range(interval)) for lag in range(4, 65)]
            pitch = 4 + scores.index(min(scores))
            if median:
                periods[position] = pitch
                medians[position] = statistics.median(periods)
                pitch = medians[(position+median+1) % width]
                position = (position+1) % width
            next_analysis += interval
        rms = math.sqrt(sum(value*value for value in samples[max(0, available-pitch):available])
                        / pitch) if pitch else 0
        if median:
            rms_values[rms_position] = rms
            rms_medians[rms_position] = statistics.median(rms_values)
            rms = rms_medians[(rms_position+median+1) % width]
            rms_position = (rms_position+1) % width
        result.append((rate/pitch if pitch else 0, rms))
    return result


class PitchamdfHistoryTests(unittest.TestCase):
    def test_invalid_frequency_ranges(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = os.environ.get("CSOUND_TEST_EXECUTABLE", str(default))
        if not Path(executable).is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
instr 1
  aInput=0
  kPitch,kRms pitchamdf aInput,p4,p5,p6,0,p7
endin
</CsInstruments>
<CsScore>
i1 0 .015625 0 128 0 1
i1 0 .015625 64 128 1000 1
i1 0 .015625 64 128 0 32
e
</CsScore>
</CsoundSynthesizer>
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid.csd"
            path.write_text(csd)
            result = subprocess.run([executable, str(path)], capture_output=True,
                                    text=True, timeout=60)
            self.assertEqual(result.returncode, 3, result.stdout + result.stderr)

    def test_resampling_windows_and_partial_notes(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = os.environ.get("CSOUND_TEST_EXECUTABLE", str(default))
        if not Path(executable).is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for block in (1, 8, 16):
                tables, events = [], []
                start = 0
                cases = [(factor, interval, median)
                         for factor in (1, 3, 32, -2, -4)
                         for interval, median in ((16, 0), (64, 1), (128, 0))]
                for case, (factor, interval, median) in enumerate(cases):
                    offset = 3 if block > 1 and case % 2 else 0
                    count = max(256, max(1, factor) * 192)
                    rate = 1024 / factor if factor > 0 else 1024 * -factor
                    expected = reference(block, offset, count, factor, interval, median)
                    for column in (0, 1):
                        number = 2*case+column+1
                        filename = f"expected-{number}.txt"
                        (root / filename).write_text("\n".join(
                            format(row[column], ".17g") for row in expected))
                        tables.append(f'giT{number} ftgen {number},0,-{len(expected)},-23,"{filename}"')
                    events.append(f"i1 {start+offset/1024:.10f} {count/1024:.10f} "
                                  f"{factor} {rate:.17g} {interval} {median} {2*case+1} {len(expected)}")
                    start += math.ceil(count/1024) + 1
                csd = f"""<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps={block}
nchnls=1
0dbfs=1
""" + "\n".join(tables) + """
gkChecks init 0
instr 1
  iOffset=int(p2*sr+.5)%ksmps
  iCount=int(p3*sr+.5)
  kCount init 0
  kCycle init 0
  kStart=(kCount==0 ? iOffset : 0)
  kEnd=min(ksmps,kStart+iCount-kCount)
  aInput init 0
  kN=0
  while kN<ksmps do
    kIndex=kCount+kN-kStart
    kValue=(kN>=kStart && kN<kEnd ? (kIndex*7%31-15)/32 : 99)
    vaset kValue,kN,aInput
    kN+=1
  od
  kPitch,kRms pitchamdf aInput,p5/64,p5/4,0,p7,p4,p5/p6,p7
  kExpectedPitch table kCycle,p8
  kExpectedRms table kCycle,p8+1
  if !(abs(kPitch-kExpectedPitch)<.0001) || !(abs(kRms-kExpectedRms)<.00001) then
    printks "FAIL pitchamdf factor=%g interval=%g median=%g cycle=%g: pitch %g expected %g, rms %g expected %g\\n",0,p4,p6,p7,kCycle,kPitch,kExpectedPitch,kRms,kExpectedRms
    exitnowk -1
  endif
  kCycle+=1
  kCount+=kEnd-kStart
  if kCount==iCount && kCycle==p9 then
    gkChecks+=1
  endif
endin
instr 99
  if i(gkChecks)!=15 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
""" + "\n".join(events) + f"\ni99 {start} .015625\ne\n</CsScore>\n</CsoundSynthesizer>\n"
                path = root / "check.csd"
                path.write_text(csd)
                with self.subTest(block=block):
                    result = subprocess.run([executable, path.name], cwd=root,
                                            capture_output=True, text=True, timeout=60)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
