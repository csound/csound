<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 8
nchnls = 1
0dbfs = 1

giCarrier ftgen 1, 0, 4, -7, 1, 4, 1
; The two tables describe the same triangle, including the guard point.
giPowerTwo ftgen 2, 0, 8, -7, 0, 4, 1, 4, 0
giOther ftgen 3, 0, -12, -7, 0, 6, 1, 6, 0
; Set the initial phase to 1/8, where the triangle's value is 1/4.
giParams ftgen 4, 0, 4, -2, .125, 0, 0, 0
gkChecks init 0

instr 1
  iMode = p4
  iFrequency = p5
  aReference oscbnk 0,1,0,0,1,1234,iFrequency,iFrequency,iFrequency,iFrequency,iMode,0,0,0,0,0,0,-1,1,2,2,0,0,0,4,0
  aLFO1 oscbnk 0,1,0,0,1,1234,iFrequency,iFrequency,iFrequency,iFrequency,iMode,0,0,0,0,0,0,-1,1,3,2,0,0,0,4,0
  aLFO2 oscbnk 0,1,0,0,1,1234,iFrequency,iFrequency,iFrequency,iFrequency,iMode,0,0,0,0,0,0,-1,1,2,3,0,0,0,4,0
  kCycle timeinstk
  kIndex = 0
  while kIndex < ksmps do
    kReference vaget kIndex, aReference
    kLFO1 vaget kIndex, aLFO1
    kLFO2 vaget kIndex, aLFO2
    if abs(kReference - kLFO1) > .00001 || abs(kReference - kLFO2) > .00001 then
      printks "oscbnk LFO table mismatch: mode=%d frequency=%g sample=%d reference=%g LFO1=%g LFO2=%g\n", 0, iMode, iFrequency, kIndex, kReference, kLFO1, kLFO2
      exitnowk(-1)
    endif
    ; The second block is fully active in every case.
    if iFrequency == 0 && kCycle == 2 then
      kExpected = (iMode == 68 ? .5 : .25)
      if abs(kReference - kExpected) > .00001 then
        printks "oscbnk static LFO value: expected=%g actual=%g\n", 0, kExpected, kReference
        exitnowk(-1)
      endif
    endif
    kIndex += 1
  od
  if kCycle == 8 then
    gkChecks += 1
  endif
endin

instr 2
  if gkChecks != 12 then
    printks "oscbnk missing LFO cases: %d\n", 0, gkChecks
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
; Each LFO alone and both together, with static and moving phases.
; The second group begins three samples into a block.
i 1 0 .0625 64 0
i 1 0 .0625 4 0
i 1 0 .0625 68 0
i 1 0 .0625 64 24
i 1 0 .0625 4 24
i 1 0 .0625 68 24
i 1 .1279296875 .0625 64 0
i 1 .1279296875 .0625 4 0
i 1 .1279296875 .0625 68 0
i 1 .1279296875 .0625 64 24
i 1 .1279296875 .0625 4 24
i 1 .1279296875 .0625 68 24
i 2 .25 .01
</CsScore>
</CsoundSynthesizer>
