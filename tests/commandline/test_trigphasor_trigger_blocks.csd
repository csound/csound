<CsTest>
description = "trigphasor audio reset timing across blocks and changing rates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  ; The trigger crosses zero at p4 samples after the note starts.
  aTrigger line -p4, p3, 64-p4
  aRate line .25*p5, p3, 2.25*p5
  kBlock timeinstk
  kRate = (kBlock == 1 ? .25 : .75)*p5
  aAK trigphasor aTrigger, kRate, 0, 16, 4
  aAA trigphasor aTrigger, aRate, 0, 16, 4
  aGate = 1
  kElapsed init 0
  kExpectedAK init 0
  kExpectedAA init 0
  kPreviousAK init 0
  kPreviousAA init 0
  kN = 0
  while kN < ksmps do
    kGate vaget kN, aGate
    kAK vaget kN, aAK
    kAA vaget kN, aAA
    if kGate != 0 then
      if kElapsed == 0 && p4 < 0 then
        ; A note-start trigger has no earlier interval to advance through.
        kExpectedAK = 4
        kExpectedAA = 4
      elseif kElapsed > p4 && kElapsed-1 <= p4 then
        kExpectedAK = 4 + (kElapsed-p4)*kPreviousAK
        kExpectedAA = 4 + (kElapsed-p4)*kPreviousAA
      endif
      kExpectedAK -= 16*floor(kExpectedAK/16)
      kExpectedAA -= 16*floor(kExpectedAA/16)
      if abs(kAK-kExpectedAK) + abs(kAA-kExpectedAA) > .00001 then
        printks "trigphasor crossing=%g sample=%g expected=%g/%g actual=%g/%g\n", 0, p4, kElapsed, kExpectedAK, kExpectedAA, kAK, kAA
        exitnowk(-1)
      endif
      kPreviousAK = kRate
      kPreviousAA = (.25+kElapsed/32)*p5
      kExpectedAK += kPreviousAK
      kExpectedAA += kPreviousAA
      kElapsed += 1
    elseif kAK != 0 || kAA != 0 then
      printks "trigphasor inactive output must be zero\n", 0
      exitnowk(-1)
    endif
    kN += 1
  od
  if kElapsed == 64 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 10 then
    prints "trigphasor block checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Crossings within and between blocks, forwards and backwards.
i 1 0 .0078125 2.5 1
i 1 .015625 .0078125 15.5 1
i 1 .03125 .0078125 16.5 1
i 1 .046875 .0078125 15.5 -1
; Start three samples into a block; elapsed sample 13 begins the next block.
i 1 .0628662109375 .0078125 12.5 1
i 1 .0784912109375 .0078125 12.5 -1
; A positive trigger on the first active sample and an exact-zero crossing.
i 1 .09375 .0078125 -.5 1
i 1 .1097412109375 .0078125 -.5 -1
i 1 .125 .0078125 15 1
i 1 .1409912109375 .0078125 12 1
i 99 .15625 .002
e
</CsScore>
</CsoundSynthesizer>
