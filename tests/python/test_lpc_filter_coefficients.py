"""LPC filters must not alter the analysis shared by other consumers."""

import ctypes as ct
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


def analysis(order):
    poles = []
    polynomial = [1.0]
    for radius, angle in ((.5, .5), (.6, .9))[:order // 2]:
        poles.extend((radius, -angle, radius, angle))
        pair = [1, -2 * radius * math.cos(angle), radius * radius]
        product = [0.0] * (len(polynomial) + 2)
        for i, coefficient in enumerate(polynomial):
            for j, value in enumerate(pair):
                product[i+j] += coefficient * value
        polynomial = product
    return poles, [-value for value in polynomial[:0:-1]]


def write_lpc(path, values, order, pole_format, sample_type):
    class Header(ct.Structure):
        _fields_ = [(name, ct.c_uint32) for name in
                    ("size", "magic", "order", "values")]
        _fields_ += [(name, sample_type) for name in ("rate", "sr", "duration")]
        _fields_ += [("text", ct.c_char * 4)]

    frame = [1, 1, 0, 100] + values
    header = Header(ct.sizeof(Header), 2399 if pole_format else 999,
                    order, len(frame), 64, 1024, .03125)
    samples = (sample_type * (len(frame) * 2))(*(frame + frame))
    path.write_bytes(bytes(header) + bytes(samples))


class LpcFilterTests(unittest.TestCase):
    def test_shared_analysis_and_filter_output(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = os.environ.get("CSOUND_TEST_EXECUTABLE", str(default))
        if not Path(executable).is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            tables = []
            for order in (2, 4):
                poles, coefficients = analysis(order)
                for size, sample_type in ((4, ct.c_float), (8, ct.c_double)):
                    for mode, values in ((0, coefficients), (1, poles)):
                        write_lpc(root / f"analysis-{order}-{mode}-{size}.lpc",
                                  values, order, mode, sample_type)
                # Direct all-pole recurrence, independent of Csound's circular buffer.
                output = []
                for n in range(128):
                    value = .1 if n == 0 else .05 if n == 10 else 0
                    value += sum(coefficient * output[n-order+i]
                                 for i, coefficient in enumerate(coefficients)
                                 if n-order+i >= 0)
                    output.append(value)
                tables.append(f"giRef{order} ftgen {order},0,-128,-2," +
                              ",".join(format(value, ".17g") for value in output))
                step = []
                for n in range(64):
                    step.append(.1 + sum(coefficient * step[n-order+i]
                                         for i, coefficient in enumerate(coefficients)
                                         if n-order+i >= 0))
                tables.append(f"giStep{order} ftgen {order+10},0,-64,-2," +
                              ",".join(format(value, ".17g") for value in step))
            events = []
            for case, (order, mode, offset) in enumerate(
                    (order, mode, offset) for order in (2, 4)
                    for mode in (0, 1) for offset in (0, 3)):
                events.append(f"i1 {case / 2 + offset / 1024:.10f} .125 {order} {mode}")
            csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
""" + "\n".join(tables) + """
gkChecks init 0
instr 1
  iSize floatsize
  SFile sprintf "analysis-%d-%d-%d.lpc",p4,p5,iSize
  iOffset=int(p2*sr+.5)%ksmps
  kCount init 0
  kStart=(kCount==0 ? iOffset : 0)
  kEnd=min(ksmps,kStart+128-kCount)
  aInput init 0
  kN=0
  while kN<ksmps do
    kIndex=kCount+kN-kStart
    kValue=(kIndex==0 ? .1 : kIndex==10 ? .05 : 0)
    vaset kValue,kN,aInput
    kN+=1
  od
  kRatio=(kCount<32 ? 2 : kCount<64 ? .5 : kCount<96 ? 1 : 2)
  lpslot 0
  kR,kO,kE,kC lpread 0,SFile
  if p5==0 then
    aShift1 lpfreson aInput,kRatio
    aShift2=aInput
    aShift2 lpfreson aShift2,kRatio
  else
    kCfBefore,kBwBefore lpform 1
  endif
  aFirst lpreson aInput
  aSecond=aInput
  aSecond lpreson aSecond
  if p5==1 then
    kCfAfter,kBwAfter lpform 1
    if kCfBefore!=kCfAfter || kBwBefore!=kBwAfter then
      printks "FAIL lpreson changed the shared poles\\n",0
      exitnowk -1
    endif
  endif
  ; An isolated reader provides a reference for the shifted filter.
  lpslot 1
  kR2,kO2,kE2,kC2 lpread 0,SFile
  if p5==0 then
    aShiftRef lpfreson aInput,kRatio
  endif
  kN=0
  while kN<ksmps do
    kExpected=0
    if kN>=kStart && kN<kEnd then
      kExpected table kCount+kN-kStart,p4
    endif
    kFirst vaget kN,aFirst
    kSecond vaget kN,aSecond
    if !(abs(kFirst-kExpected)<.00001) || !(abs(kSecond-kExpected)<.00001) then
      printks "FAIL lpreson order=%g mode=%g sample=%g: %g %g expected %g\\n",0,p4,p5,kCount+kN-kStart,kFirst,kSecond,kExpected
      exitnowk -1
    endif
    if p5==0 then
      kShift1 vaget kN,aShift1
      kShift2 vaget kN,aShift2
      kShiftRef vaget kN,aShiftRef
      if !(abs(kShift1-kShiftRef)<.00001) || !(abs(kShift2-kShiftRef)<.00001) then
        printks "FAIL lpfreson changed the shared coefficients\\n",0
        exitnowk -1
      endif
    endif
    kN+=1
  od
  kCount+=kEnd-kStart
  if kCount==128 then
    gkChecks+=1
  endif
endin
instr 2
  kCycle timeinstk
  if kCycle==5 || kCycle==9 || kCycle==13 then
    reinit RELOAD
  endif
RELOAD:
  iSize floatsize
  iOrder=(i(kCycle)<5 || (i(kCycle)>=9 && i(kCycle)<13) ? 4 : 2)
  iMode=(i(kCycle)<9 ? 1 : 0)
  iTable=iOrder+10
  SFile sprintf "analysis-%d-%d-%d.lpc",iOrder,iMode,iSize
  lpslot 0
  kR,kO,kE,kC lpread 0,SFile
  aInput=.1
  aFirst lpreson aInput
  aSecond lpreson aInput
  rireturn
  kTable=iTable
  kN=0
  while kN<ksmps do
    kExpected tablekt ((kCycle-1)%4)*ksmps+kN,kTable
    kFirst vaget kN,aFirst
    kSecond vaget kN,aSecond
    if !(abs(kFirst-kExpected)<.00001) || !(abs(kSecond-kExpected)<.00001) then
      printks "FAIL lpreson reinit cycle=%g order=%g mode=%g sample=%g: %g %g expected %g\\n",0,kCycle,iOrder,iMode,kN,kFirst,kSecond,kExpected
      exitnowk -1
    endif
    kN+=1
  od
endin
instr 99
  if i(gkChecks)!=8 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
""" + "\n".join(events) + "\ni2 4 .25\ni99 4.5 .015625\ne\n</CsScore>\n</CsoundSynthesizer>\n"
            path = root / "check.csd"
            path.write_text(csd)
            result = subprocess.run([executable, path.name], cwd=root,
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
