<CsTest>
description = "GEN34 renders finite alternating samples at exact Nyquist"

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

giSource ftgen 1, 0, -3, -2, 2e38, 4, .25
giDest ftgen 2, 0, 8, -34, giSource, 1, 1
instr 1
 iFirst table 0, giDest
 iSecond table 1, giDest
 if !(iFirst == 2e38 && iSecond == -2e38) then
  prints "GEN34 overflowed the Nyquist oscillator\n"
  exitnow -1
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
