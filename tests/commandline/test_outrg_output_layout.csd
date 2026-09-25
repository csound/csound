<CsTest>
description = "outrg preserves channel layout and active samples with local ksmps"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 4
0dbfs = 4
gkChecks init 0

instr WriteRange
  setksmps p5
  aFirst = 1
  aSecond = 2
  outrg p4, aFirst, aSecond
endin

opcode LocalRange, 0, 0
  setksmps 2
  aFirst = 1
  aSecond = 2
  outrg 3.75, aFirst, aSecond
endop

instr WriteUDO
  LocalRange
endin

instr CheckOutput
  ; Each check covers one full global block.
  ; p4/p5 mark the active sample interval; p6 is the first output channel.
  ; p7 is the number of identical notes contributing to the output.
  aMix[] monitor
  kChannel = 0
  while kChannel < nchnls do
    kSample = 0
    while kSample < ksmps do
      kExpected = 0
      if kSample >= p4 && kSample < p5 then
        if kChannel == p6-1 then
          kExpected = p7
        elseif kChannel == p6 then
          kExpected = 2*p7
        endif
      endif
      kActual vaget kSample, aMix[kChannel]
      if kActual != kExpected then
        printks "outrg start=%g channel=%g sample=%g: expected %g, got %g\n", \
          0, p2, kChannel+1, kSample, kExpected, kActual
        exitnowk -1
      endif
      kSample += 1
    od
    kChannel += 1
  od
  gkChecks += 1
  turnoff
endin

instr CheckResults
  if i(gkChecks) != 5 then
    prints "outrg output checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Ordinary output, then local output to the last pair (fractional start truncates).
i "WriteRange" 0 .25 1 8
i "CheckOutput" 0 .25 0 8 1 1
i "WriteRange" .25 .25 3.75 2
i "CheckOutput" .25 .25 0 8 3 1
; A local note starts and ends inside a global block.
i "WriteRange" .53125 .15625 1 2
i "CheckOutput" .5 .25 1 6 1 1
; A local-ksmps UDO must use the same channel layout.
i "WriteUDO" .75 .25
i "CheckOutput" .75 .25 0 8 3 1
; Output from overlapping notes must add, not replace earlier samples.
i "WriteRange" 1 .25 1 8
i "WriteRange" 1 .25 1 2
i "CheckOutput" 1 .25 0 8 1 2
i "CheckResults" 1.25 .25
e
</CsScore>
</CsoundSynthesizer>
