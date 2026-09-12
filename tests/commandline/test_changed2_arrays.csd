<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
gkChecks init 0

opcode Check, 0, kk
  kActual, kExpected xin
  if kActual != kExpected then
    printks "changed detector: expected=%d actual=%d\n", 0, kExpected, kActual
    exitnowk(-1)
  endif
endop

instr 1
  kArray[][] init p4,p5
  kCycle timeinstk
  if kCycle == 2 then
    kArray[p4-1][p5-1] = 1
  elseif kCycle == 4 then
    kArray[0][0] = 2
  endif
  kTrigger changed2 kArray
  Check kTrigger, (kCycle == 2 || kCycle == 4 ? 1 : 0)
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  aArray[][][] init 2,2,3
  kCycle timeinstk
  aValue = kCycle >= 2 ? 1 : 0
  aArray[1][1][2] = aValue
  kTrigger changed2 aArray
  Check kTrigger, (kCycle == 2 ? 1 : 0)
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  kArray[] init 6
  trim_i kArray, 2
  kCycle timeinstk
  kSize = kCycle < 2 ? 2 : (kCycle < 5 ? 6 : (kCycle == 5 ? 1 : (kCycle < 8 ? 0 : 3)))
  trim kArray, kSize
  if kCycle == 3 then
    kArray[5] = 1
  elseif kCycle == 9 then
    kArray[2] = 1
  endif
  kTrigger changed2 kArray
  Check kTrigger, (kCycle == 2 || kCycle == 3 || kCycle == 5 || kCycle == 6 || kCycle == 8 || kCycle == 9 ? 1 : 0)
  if kCycle == 10 then
    gkChecks += 1
    turnoff
  endif
endin

instr 4
  kEmpty[] init 0
  kArray[] init 1
  kCycle timeinstk
  kArray[0] = kCycle
  if kCycle == 3 then
    reinit WATCH
  endif
WATCH:
  kTrigger changed2 kArray
  rireturn
  kEmptyTrigger changed2 kEmpty
  Check kEmptyTrigger, 0
  Check kTrigger, (kCycle == 1 || kCycle == 3 ? 0 : 1)
  kValue = kCycle < 3 ? 5 : 7
  kLegacy changed kValue
  kNew changed2 kValue
  Check kLegacy, (kCycle == 1 || kCycle == 3 ? 1 : 0)
  Check kNew, (kCycle == 3 ? 1 : 0)
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if gkChecks != 7 then
    printks "missing changed2 cases: %d\n", 0, gkChecks
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .125 2 3
i 1 .125 .125 3 2
i 1 .25 .125 1 3
i 1 .375 .125 3 1
i 2 .5 .125
i 3 .625 .25
i 4 .875 .125
i 99 1 .01
</CsScore>
</CsoundSynthesizer>
