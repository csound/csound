<CsTest>
description = "tvconv direct and partitioned convolution use the same amplitude scale"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
#ifndef FULL_SCALE
#define FULL_SCALE #32768#
#end
0dbfs = $FULL_SCALE
gkChecks init 0

instr CheckGain
 ; Once the buffers fill, each tap contributes .25 * .125 * 0dbfs.
 ; p6 is the actual number of taps after size rounding and swapping.
 aInput = .25 * 0dbfs
 aCoefficients = .125 * 0dbfs
 aOutput tvconv aInput, aCoefficients, 1, 1, p4, p5
 kCycle init 0
 kCycle += 1
 if kCycle == 16 then
  kActual downsamp aOutput
  iExpected = p6 * .25 * .125 * 0dbfs
  if !(abs(kActual - iExpected) < .00001 * 0dbfs) then
   printks "tvconv gain: partition=%g filter=%g expected=%g actual=%g\n", 0, p4, p5, iExpected, kActual
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 9 then
  prints "tvconv gain checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Direct convolution supports any whole filter length.
i "CheckGain" 0 .04  1   8  8
i "CheckGain" 0 .04  1   7  7
i "CheckGain" 0 .04  0   7  7
i "CheckGain" 0 .04  .5  7  7
; Partitioned convolution, including a single partition.
i "CheckGain" 0 .04  2   8  8
i "CheckGain" 0 .04  4   8  8
i "CheckGain" 0 .04  8   8  8
; Ties round up: 3 -> 4. The larger argument becomes the filter size.
i "CheckGain" 0 .04  3   7  8
i "CheckGain" 0 .04 12   3 16
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
