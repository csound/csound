<CsTest>
description = "outz preserves channel layout and active samples with local ksmps"

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
zakinit 8, 1
0dbfs = 4
gkChecks init 0

instr WriteRange
  setksmps p5
  aFirst = 1
  aSecond = 2
  ; Store a four-channel frame, including two silent channels.
  aSilent = 0
  zaw aFirst, p4
  zaw aSecond, p4+1
  zaw aSilent, p4+2
  zaw aSilent, p4+3
  outz p4
endin

opcode LocalRange, 0, 0
  setksmps 2
  aFirst = 1
  aSecond = 2
  aSilent = 0
  zaw aFirst, 5
  zaw aSecond, 6
  zaw aSilent, 7
  zaw aSilent, 8
  outz 5.75
endop

instr WriteUDO
  LocalRange
endin

instr CheckOutput
  ; Each check covers one full global block.
  ; p4/p5 mark the active sample interval.
  ; p6 is the number of identical notes contributing to the output.
  aMix[] monitor
  kChannel = 0
  while kChannel < nchnls do
    kSample = 0
    while kSample < ksmps do
      kExpected = 0
      if kSample >= p4 && kSample < p5 then
        if kChannel == 0 then
          kExpected = p6
        elseif kChannel == 1 then
          kExpected = 2*p6
        endif
      endif
      kActual vaget kSample, aMix[kChannel]
      if kActual != kExpected then
        printks "outz start=%g channel=%g sample=%g: expected %g, got %g\n", \
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
    prints "outz output checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Ordinary output, then local output from the last four ZAK slots.
; A fractional starting index still truncates.
i "WriteRange" 0 .25 0 8
i "CheckOutput" 0 .25 0 8 1
i "WriteRange" .25 .25 5.75 2
i "CheckOutput" .25 .25 0 8 1
; A local note starts and ends inside a global block.
i "WriteRange" .53125 .15625 0 2
i "CheckOutput" .5 .25 1 6 1
; A local-ksmps UDO must use the same channel layout.
i "WriteUDO" .75 .25
i "CheckOutput" .75 .25 0 8 1
; Output from overlapping notes must add, not replace earlier samples.
i "WriteRange" 1 .25 0 8
i "WriteRange" 1 .25 0 2
i "CheckOutput" 1 .25 0 8 2
i "CheckResults" 1.25 .25
e
</CsScore>
</CsoundSynthesizer>
