<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  kArr[][] init 2, 2
  kArr fillarray 1, 2, 3, 4
  kIndex init -1000
  kRow[] getrow kArr, kIndex
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
