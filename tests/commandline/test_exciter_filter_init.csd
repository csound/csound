<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 aIn oscili .25, 1000
 aZero exciter aIn, 0, 10000, 5, 0
 ; A tiny positive cutoff approximates the zero-cutoff response.
 aNear exciter aIn, .000001, 10000, 5, 0
 aInPlace = aIn
 aInPlace exciter aInPlace, 0, 10000, 5, 0
 kIndex = 0
 while kIndex < ksmps do
  kZero vaget kIndex, aZero
  kNear vaget kIndex, aNear
  kInPlace vaget kIndex, aInPlace
  if !(abs(kZero-kNear)+abs(kInPlace-kZero) < .00001) then
   printks "exciter zero cutoff differs: actual=%g near-zero=%g\n", 0, kZero, kNear
   exitnowk(-1)
  endif
  kIndex += 1
 od
 kFirst init 1
 if kFirst == 1 then
  gkChecks += 1
  kFirst = 0
 endif
endin

instr 2
 kCycle init 0
 kLow = (kCycle < 32 ? 500 : p4)
 kHigh = (kCycle < 32 ? 10000 : p5)
 aIn oscili .25, 1000
 if kCycle == 32 then
  reinit FILTER
 endif
FILTER:
 aReset exciter aIn, kLow, kHigh, 5, 0
 rireturn
 if kCycle >= 32 then
  ; This instance first runs after reinit, with the same input and empty state.
  aFresh exciter aIn, p4, p5, 5, 0
  kIndex = 0
  while kIndex < ksmps do
   kReset vaget kIndex, aReset
   kFresh vaget kIndex, aFresh
   if !(abs(kReset-kFresh) < .000001) then
    printks "exciter reinit low=%g high=%g differs: reset=%g fresh=%g\n", 0, p4, p5, kReset, kFresh
    exitnowk(-1)
   endif
   if p5 == 0 && kReset != 0 then
    printks "exciter retained coefficients at zero upper cutoff\n", 0
    exitnowk(-1)
   endif
   kIndex += 1
  od
 endif
 if kCycle == 63 then
  gkChecks += 1
 endif
 kCycle += 1
endin

instr 99
 if i(gkChecks) != 5 then
  prints "exciter checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
i 1 .031375 .0198125
i 2 .06 .04266666666666667 0 10000
i 2 .12 .04266666666666667 1000 0
i 2 .18 .04266666666666667 0 0
i 99 .24 .001
e
</CsScore>
</CsoundSynthesizer>
