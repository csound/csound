<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -j 4
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  kvalues[] init 1
  kindex init 2
  kvalue = kvalues[kindex]
  kcycle timeinstk
  printks "after-error cycle=%d value=%f\n", 0, kcycle, kvalue
endin


</CsInstruments>
<CsScore>
i 1 0 0.01
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>