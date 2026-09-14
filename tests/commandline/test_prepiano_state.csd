<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1
gkChecks init 0

; Two preparations in opposite orders must describe the same instrument.
giRattle ftgen 1, 0, 16, -2, 2, .6, 10, 100, .001, .4, 5, 120, .002
giRattleReverse ftgen 2, 0, 16, -2, 2, .4, 5, 120, .002, .6, 10, 100, .001
giRubber ftgen 3, 0, 16, -2, 2, .7, 50, 500, 1000, .4, 30, 300, 500
giRubberReverse ftgen 4, 0, 16, -2, 2, .4, 30, 300, 500, .7, 50, 500, 1000

instr 1
  iStrings = p4
  iScan = p5
  iSpread = p6
  ; A single string must ignore detuning, including a nonzero value.
  iDetune = (iStrings == 1 ? 0 : 10)
  aLeft, aRight prepiano 60, iStrings, 10, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, iScan, iSpread, p7, p8
  aRefLeft prepiano 60, iStrings, iDetune, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, iScan - iSpread/2, 0, p7, p8
  aRefRight prepiano 60, iStrings, iDetune, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, iScan + iSpread/2, 0, p7, p8
  kIndex = 0
  kPeak init 0
  while kIndex < ksmps do
    kLeft vaget kIndex, aLeft
    kRight vaget kIndex, aRight
    kRefLeft vaget kIndex, aRefLeft
    kRefRight vaget kIndex, aRefRight
    kError = abs(kLeft - kRefLeft) + abs(kRight - kRefRight)
    if !(kError <= .00002) then
      printks "prepiano stereo mismatch: strings=%g scan=%g spread=%g error=%g\n", 0, iStrings, iScan, iSpread, kError
      exitnowk(-1)
    endif
    kPeak = max(kPeak, abs(kLeft), abs(kRight))
    kIndex += 1
  od
  kTime timeinsts
  kChecked init 0
  if kTime > .06 && kChecked == 0 then
    if !(kPeak > .001) then
      printks "prepiano produced no tone\n", 0
      exitnowk(-1)
    endif
    gkChecks += 1
    kChecked = 1
  endif
endin

instr 2
  iRattle = (p4 == 1 ? giRattle : 0)
  iRattleReverse = (p4 == 1 ? giRattleReverse : 0)
  iRubber = (p4 == 2 ? giRubber : 0)
  iRubberReverse = (p4 == 2 ? giRubberReverse : 0)
  aFirst prepiano 60, 3, 10, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, 0, 0, iRattle, iRubber
  aReverse prepiano 60, 3, 10, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, 0, 0, iRattleReverse, iRubberReverse
  aBare prepiano 60, 3, 10, 1, 3, .002, 2, 2, 1, 5000, -.01, .09, 20, 0, 0
  kIndex = 0
  kEffect init 0
  while kIndex < ksmps do
    kFirst vaget kIndex, aFirst
    kReverse vaget kIndex, aReverse
    kBare vaget kIndex, aBare
    if !(abs(kFirst - kReverse) <= .00002) then
      printks "prepiano depends on preparation order: kind=%g first=%g reverse=%g\n", 0, p4, kFirst, kReverse
      exitnowk(-1)
    endif
    kEffect = max(kEffect, abs(kFirst - kBare))
    kIndex += 1
  od
  kTime timeinsts
  kChecked init 0
  if kTime > .06 && kChecked == 0 then
    if !(kEffect > .00001) then
      printks "prepiano ignored its preparation table: kind=%g\n", 0, p4
      exitnowk(-1)
    endif
    gkChecks += 1
    kChecked = 1
  endif
endin

instr 99
  if i(gkChecks) != 8 then
    prints "prepiano checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Reuse the instrument with stationary, equal and distinct scan frequencies.
i 1 0 .08 3 0 0 0 0
i 1 .1 .08 3 3 0 0 0
i 1 .2001 .0801 3 3 2 0 0
i 1 .3001 .0801 1 3 2 0 0
i 1 .4001 .0801 3 3 2 1 0
i 1 .5001 .0801 3 3 2 0 3
i 2 .6001 .0801 1
i 2 .7001 .0801 2
i 99 .8 .01
e
</CsScore>
</CsoundSynthesizer>
