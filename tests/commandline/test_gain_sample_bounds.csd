<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  aWave oscili .25, 321
  aInput = .5 + aWave
  aReference = .6 - .3*aWave
  kTarget rms aReference, p4
  aGain gain aInput, kTarget, p4
  aInPlace = aInput
  aInPlace gain aInPlace, kTarget, p4
  ; Matching RMS filters make rms + gain equivalent to balance.
  aExpected balance aInput, aReference, p4
  aError = abs(aGain-aExpected) + abs(aInPlace-aExpected)
  kError max_k aError, 1, 1
  if !(kError <= .00001) then
    printks "gain differs from balance for note at %g: error %g\n", 0, p2, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 6 then
    prints "gain checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .006 10
i 1 .01 .00575 10
i 1 .02025 .0055 10
i 1 .030625 .005125 1000
i 1 .04 .00175 1000
i 1 .05025 .0015 1000
i 99 .06 .002
</CsScore>
</CsoundSynthesizer>
