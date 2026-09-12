"""Compare convolve with a direct FIR sum across block and impulse lengths."""

import cmath
import ctypes as ct
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


def write_cv(path, length, channels, sample_type, text_format=False):
    """Write the native CV layout with a three-tap impulse per channel."""
    class Header(ct.Structure):
        _fields_ = [(name, ct.c_int32) for name in
                    ("magic", "head_bytes", "data_bytes", "data_format")]
        _fields_ += [("rate", sample_type)]
        _fields_ += [(name, ct.c_int32) for name in
                     ("channels", "channel", "length", "format")]
        _fields_ += [("info", ct.c_char * 4)]

    fft_size = 1 << (2 * length - 2).bit_length()
    data = []
    for channel in range(channels):
        for bin_number in range(fft_size // 2 + 1):
            value = sum(gain * cmath.exp(-2j * math.pi * bin_number * tap / fft_size)
                        for tap, gain in ((0, .5), (1, -.125), (length - 1, .25)))
            value /= 2 ** channel
            data.extend((value.real, value.imag))
    samples = (sample_type * len(data))(*data)
    header = Header(666, ct.sizeof(Header), ct.sizeof(samples), 36,
                    1024, channels, 32767, length, 1)
    if text_format:
        path.write_text(f"CVANAL\n{header.head_bytes} {header.data_bytes} 36 "
                        f"1024 {channels} 32767 {length} 1\n" +
                        "\n".join(float(value).hex() for value in data) + "\n")
    else:
        path.write_bytes(bytes(header) + bytes(samples))


class ConvolveSampleTests(unittest.TestCase):
    def test_block_boundaries_and_input_reuse(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = os.environ.get("CSOUND_TEST_EXECUTABLE", str(default))
        if not Path(executable).is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for length in (3, 8, 17, 25, 32):
                for size, sample_type in ((4, ct.c_float), (8, ct.c_double)):
                    write_cv(root / f"impulse-{length}-{size}.cv", length, 4, sample_type)
                    write_cv(root / f"text-{length}-{size}.cv", length, 4,
                             sample_type, text_format=True)
            events = []
            for case, (length, mode, offset) in enumerate(
                    (length, mode, offset) for length in (3, 8, 17, 25, 32)
                    for mode in (0, 1, 2, 3, 4) for offset in (0, 3)):
                events.append(f"i1 {case / 2 + offset / 1024:.10f} .1875 {length} {mode}")
            csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gkChecks init 0
instr 1
  iSize floatsize
  SFile sprintf "impulse-%d-%d.cv", p4, iSize
  if p5==3 then
    SFile sprintf "text-%d-%d.cv", p4, iSize
  endif
  iOffset=int(p2*sr+.5)%ksmps
  iTotal=int(p3*sr+.5)
  iDelay=(p4>=ksmps ? ceil(p4/ksmps)*ksmps : ceil(ksmps/p4)*p4)
  kCount init 0
  kStart=(kCount==0 ? iOffset : 0)
  kEnd=min(ksmps,kStart+iTotal-kCount)
  aInput init 0
  kN=0
  while kN<ksmps do
    kIndex=kCount+kN-kStart
    ; Nonzero inactive input makes incorrect sample masking visible.
    kValue=(kN>=kStart && kN<kEnd ? (kIndex%19-9)/16 : 10)
    vaset kValue,kN,aInput
    kN+=1
  od
  if p5==0 || p5==3 then
    a1,a2,a3,a4 convolve aInput,SFile
  elseif p5==1 then
    a1,a2,aInput,a4 convolve aInput,SFile
    a3=aInput
  elseif p5==2 then
    a1 convolve aInput,SFile,3
  else
    strset 77,SFile
    a1 convolve aInput,77,3
  endif
  kN=0
  while kN<ksmps do
    kIndex=kCount+kN-kStart-iDelay
    kExpected=0
    if kN>=kStart && kN<kEnd then
      if kIndex>=0 then
        kExpected=.5*(kIndex%19-9)/16
      endif
      if kIndex>=1 then
        kExpected-=.125*((kIndex-1)%19-9)/16
      endif
      if kIndex>=p4-1 then
        kExpected+=.25*((kIndex-p4+1)%19-9)/16
      endif
    endif
    kActual vaget kN,a1
    kFirst=(p5==2 || p5==4 ? kExpected/4 : kExpected)
    if !(abs(kActual-kFirst)<.00001) then
      printks "FAIL convolve length=%g mode=%g index=%g: %g expected %g\\n",0,p4,p5,kIndex,kActual,kFirst
      exitnowk -1
    endif
    if p5!=2 && p5!=4 then
      k2 vaget kN,a2
      k3 vaget kN,a3
      k4 vaget kN,a4
      if !(abs(k2-kExpected/2)<.00001) || !(abs(k3-kExpected/4)<.00001) || !(abs(k4-kExpected/8)<.00001) then
        printks "FAIL convolve channel separation\\n",0
        exitnowk -1
      endif
    endif
    kN+=1
  od
  kCount+=kEnd-kStart
  if kCount==iTotal then
    gkChecks+=1
  endif
endin
instr 2
  iSize floatsize
  kCycle init 0
  kCycle+=1
  if kCycle==13 || kCycle==25 || kCycle==37 then
    reinit LOAD
  endif
LOAD:
  iLength=(i(kCycle)<13 ? 25 : i(kCycle)<25 ? 8 : i(kCycle)<37 ? 32 : 3)
  iLast=iLength-1
  SFile sprintf "impulse-%d-%d.cv",iLength,iSize
  iDelay=(iLength>=ksmps ? ceil(iLength/ksmps)*ksmps : ceil(ksmps/iLength)*iLength)
  aInput=1
  a1,a2,a3,a4 convolve aInput,SFile
  rireturn
  kN=0
  while kN<ksmps do
    kIndex=((kCycle-1)%12)*ksmps+kN-iDelay
    kExpected=(kIndex<0 ? 0 : kIndex==0 ? .5 : kIndex<iLast ? .375 : .625)
    k1 vaget kN,a1
    k4 vaget kN,a4
    if !(abs(k1-kExpected)<.00001) || !(abs(k4-kExpected/8)<.00001) then
      printks "FAIL convolve reinit length=%g sample=%g: %g expected %g\\n",0,iLength,kIndex,k1,kExpected
      exitnowk -1
    endif
    kN+=1
  od
endin
instr 99
  if i(gkChecks)!=50 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
""" + "\n".join(events) + "\ni2 25 .75\ni99 26 .015625\ne\n</CsScore>\n</CsoundSynthesizer>\n"
            path = root / "check.csd"
            path.write_text(csd)
            result = subprocess.run([executable, path.name], cwd=root,
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
