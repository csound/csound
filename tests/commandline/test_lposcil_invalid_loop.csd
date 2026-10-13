<CsTest>
description = "looping oscillators reject a loop whose start reaches its end"
[expect]
exit = 3
stderr = ["lposcil: loop start must precede end"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
giWave ftgen 1, 0, 8, -2, 1
instr 1
  kEnd init 6
  kCycle timeinstk
  if kCycle == 2 then
    kEnd = 2
  endif
  if p4 == 0 then
    aOut lposcil 1, 1, 2, kEnd, giWave
  elseif p4 == 1 then
    aOut lposcil3 1, 1, 2, kEnd, giWave
  else
    aAmp = 1
    aOut lposcila aAmp, 1, 2, kEnd, giWave
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001 0
i 1 0 .001 1
i 1 0 .001 2
e
</CsScore>
</CsoundSynthesizer>
