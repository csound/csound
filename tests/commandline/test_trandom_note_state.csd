<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kBlock init 0
  kTrigger = 0
  kMin = .25
  kMax = .25
  if p4 == 1 then
    if kBlock == 0 then
      kTrigger = 1
    elseif kBlock == 2 then
      kTrigger = -1
      kMin = .75
      kMax = .75
    endif
    kExpected = (kBlock < 2 ? .25 : .75)
  else
    kExpected = 0
  endif
  kActual trandom kTrigger, kMin, kMax
  if kActual != kExpected then
    printks "FAIL trandom note=%g block=%g: %g expected %g\n", \
        0, p4, kBlock, kActual, kExpected
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 2
  kBlock init 0
  kTrigger = (kBlock == 0 ? 1 : 0)
  if kBlock == 2 then
    reinit RESET
  endif
RESET:
  kActual trandom kTrigger, .5, .5
  rireturn
  kExpected = (kBlock < 2 ? .5 : 0)
  if kActual != kExpected then
    printks "FAIL trandom reinit block=%g: %g expected %g\n", \
        0, kBlock, kActual, kExpected
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 3
  ; Resetting held state must not consume a random number.
  seed 1234
  iRandom[] fillarray 0.06207801683902648, 0.5032507099692014, 0.19473178274684202
  kBlock init 0
  kActual trandom 1, 0, 1
  if !(abs(kActual - iRandom[kBlock]) < .000001) then
    printks "FAIL trandom changed the seeded sequence at block %g: %g\n", 0, kBlock, kActual
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 3 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 6 then
    prints "FAIL trandom checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Reuse the same instrument instance after it has held a nonzero value.
i 1 0 .0625 0
i 1 .125 .0625 1
i 1 .25 .0625 0
i 1 .375 .0625 1
i 2 .5 .0625
i 3 .625 .046875
i 99 .75 .015625
</CsScore>
</CsoundSynthesizer>
