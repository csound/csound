<CsTest>
description = "rms retains contributions smaller than the normalized energy range"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --preserve-subnormals
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0
instr Detector
  kSample init 0
  kFirst init 0
  aInput = (kSample == 0 ? 1e15 : 6e14)
  ; This cutoff gives the smallest positive coefficient in a float build.
  kLevel rms aInput, 1e-41
  if kSample == 0 then
    kFirst = kLevel
  endif
  if kSample == 100 then
    ; Decay is negligible: the pulse contributes 1, each later sample .36.
    kExpected = sqrt(1 + 100*.36)
    if !(kFirst > 0 && abs(kLevel/kFirst/kExpected-1) < .00001) then
      printks "Tiny RMS weight: first %g, final %g, expected ratio %g\n", 0, kFirst, kLevel, kExpected
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr Done
  if i(gkChecks) != 1 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Detector" 0 [102/48000]
i "Done" [103/48000] [1/48000]
e
</CsScore>
</CsoundSynthesizer>
