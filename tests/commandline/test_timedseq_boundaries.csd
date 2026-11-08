<CsTest>
description = "Timed sequence crossings, wrapping, and stopped pointers"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
gkDone init 0
giRows ftgen 0, 0, -16, -2, 11, .25, 1, 0, 22, .5, 2, 0, 33, .75, 3, 0, -1, 1, 0, 0
giShort ftgen 0, 0, -6, -2, 44, .0625, 55, .9375, -1, 1
giZero ftgen 0, 0, -4, -2, 66, 0, -1, 1

instr 1
  kPhase[] fillarray 0, .125, .25, .25, .375, .5, .625, .75, .875, 0, .125, .3125, .5625, .8125, .625, .5, .375, .25, .125, 0
  kWant[] fillarray 0, 0, 11, 0, 0, 22, 0, 33, 0, 0, 0, 11, 22, 33, 33, 22, 0, 11, 0, 0
  kStep init 0
  kEvent init -99
  kTime init -99
  kDuration init -99
  kValue init -99
  kLast init -99
  kTrig timedseq kPhase[kStep], giRows, kEvent, kTime, kDuration, kValue
  if (kWant[kStep] != 0 && kTrig != 1) || (kWant[kStep] == 0 && kTrig != 0) then
    printks "wrong trigger at step %d: %f\n", 0, kStep, kTrig
    exitnowk -1
  endif
  if kTrig == 1 then
    kLast = kWant[kStep]
    if kEvent != kLast || kTime != kLast / 44 || kDuration != kLast / 11 || kValue != 0 then
      exitnowk -1
    endif
  elseif kEvent != kLast then
    exitnowk -1
  endif
  kStep += 1
  if kStep == lenarray(kPhase) then
    gkDone += 1
    turnoff
  endif
endin

instr 2
  ; Two pfields suffice. Cross events on both sides of a loop boundary.
  kPhase[] fillarray .875, .125, .875, .875, 1.125, -.125, -.125
  kWant[] fillarray 0, 44, 55, 0, 44, 55, 0
  kStep init 0
  kEvent init -99
  kTime init -99
  kTrig timedseq kPhase[kStep], giShort, kEvent, kTime
  if (kWant[kStep] != 0 && (kTrig != 1 || kEvent != kWant[kStep])) || (kWant[kStep] == 0 && kTrig != 0) then
    exitnowk -1
  endif
  kStep += 1
  if kStep == lenarray(kPhase) then
    gkDone += 1
    turnoff
  endif
endin

instr 3
  ; A single event at zero needs no duplicate at the end of the loop.
  ; Run this instrument twice to check initialization on note reuse.
  kPhase[] fillarray 0, 0, .25, .5, .75, 0, 0
  kWant[] fillarray 1, 0, 0, 0, 0, 1, 0
  kStep init 0
  kEvent init 0
  kTime init -1
  kTrig timedseq kPhase[kStep], giZero, kEvent, kTime
  if kTrig != kWant[kStep] || kEvent != 66 || kTime != 0 then
    exitnowk -1
  endif
  kStep += 1
  if kStep == lenarray(kPhase) then
    gkDone += 1
    turnoff
  endif
endin

instr 4
  if i(gkDone) != 4 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03
i 2 0 .01
i 3 0 .01
i 3 .02 .01
i 4 .04 .001
</CsScore>
</CsoundSynthesizer>
