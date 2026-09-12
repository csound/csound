<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
giShape ftgen 1, 0, 17, -7, -1, 16, 1
instr 1
  aIn init 0
  aH, aL, aB, aR svn aIn, 64, 1, 1, p4, p5, p6
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 999 0 1
i 1 0 .015625 0 999 1
i 1 0 .015625 1 0 0
e
</CsScore>
</CsoundSynthesizer>
