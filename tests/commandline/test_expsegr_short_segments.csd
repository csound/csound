<CsTest>
description = "expsegr applies zero-step jumps before writing the current output"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

opcode Check, 0, kkS
  kActual, kExpected, SName xin
  if !(abs(kActual-kExpected) < .00001) then
    printks "%s: expected %g, got %g\n", 0, SName, kExpected, kActual
    exitnowk -1
  endif
endop

instr MiddleJumps
  ; Both rates have the same step length. At step 4, two short
  ; segments must jump from 16 through 4 to 2 in the same step.
  iShort = p4/sr
  iSign = p5
  kLine expsegr iSign, 4/sr, 16*iSign, iShort, 4*iSign, iShort, 2*iSign, 4/sr, 8*iSign, 4/sr, iSign
  aLine expsegr iSign, 4/sr, 16*iSign, iShort, 4*iSign, iShort, 2*iSign, 4/sr, 8*iSign, 4/sr, iSign
  kAudio downsamp aLine
  kStep timeinstk
  kStep -= 1
  if kStep < 4 then
    kExpected = 2^kStep
  elseif kStep < 8 then
    kExpected = 2*4^((kStep-4)/4)
  elseif kStep < 12 then
    kExpected = 8
  else
    kExpected = 8*(1/8)^(min(4, kStep-12)/4)
  endif
  Check kLine, iSign*kExpected, "middle jumps, control"
  Check kAudio, iSign*kExpected, "middle jumps, audio"
endin

instr FirstJump
  ; A zero or short positive first duration jumps immediately.
  kLine expsegr 1, p4/sr, 16, 4/sr, 1, 4/sr, .25
  aLine expsegr 1, p4/sr, 16, 4/sr, 1, 4/sr, .25
  kAudio downsamp aLine
  kStep timeinstk
  kStep -= 1
  if kStep < 4 then
    kExpected = 16*(1/16)^(kStep/4)
  elseif kStep < 8 then
    kExpected = 1
  else
    kExpected = .25^(min(4, kStep-8)/4)
  endif
  Check kLine, kExpected, "first jump, control"
  Check kAudio, kExpected, "first jump, audio"
endin
</CsInstruments>
<CsScore>
i "MiddleJumps" 0 [12/1024] 0 1
i "MiddleJumps" .1 [12/1024] .125 1
i "MiddleJumps" .2 [12/1024] .125 -1
i "FirstJump" .3 [8/1024] 0
i "FirstJump" .4 [8/1024] .125
e
</CsScore>
</CsoundSynthesizer>
