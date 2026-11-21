<CsTest>
description = "phasorbnk retains a small step through cancellation"

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

instr 1
  kSample init 0
  kSteps[] fillarray -2^(-53), 2^(-27), -2^(-27), -.125, 2^(-54), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  kPhase phasorbnk kSteps[min(kSample,15)]*kr, 0, 2, .125
  if kSample == 5 then
    if !(kPhase == 0) then
      printks "phasorbnk lost a step before wrapping\n", 0
      exitnowk -1
    endif
    turnoff
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
