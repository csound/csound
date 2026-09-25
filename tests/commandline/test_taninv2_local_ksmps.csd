<CsTest>
description = "taninv2 uses each audio array element's stride inside a local-ksmps UDO"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode LocalAngles, aa, a[]a[]
  setksmps 4
  aY[], aX[] xin
  aAngles[] = taninv2(aY, aX)
  xout aAngles[0], aAngles[1]
endop

instr CheckLocalStride
  aY[] init 2
  aX[] init 2
  aY[0] = 1
  aX[0] = 1
  aY[1] = -1
  aX[1] = 1
  aPositive, aNegative LocalAngles aY, aX
  kSample = 0
  while kSample < ksmps do
    kPositive vaget kSample, aPositive
    kNegative vaget kSample, aNegative
    if !(abs(kPositive-taninv2(1, 1)) < .000001 && \
         abs(kNegative-taninv2(-1, 1)) < .000001) then
      printks "Local taninv2 sample %g: expected (+pi/4,-pi/4), got (%g,%g)\n", \
        0, kSample, kPositive, kNegative
      exitnowk -1
    endif
    kSample += 1
  od
  gkChecks += 1
  turnoff
endin

instr CheckResults
  if i(gkChecks) != 1 then
    prints "Local taninv2 check did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckLocalStride" 0 .001
i "CheckResults" .002 .001
e
</CsScore>
</CsoundSynthesizer>
