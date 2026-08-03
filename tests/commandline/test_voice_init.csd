<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  aout voice 0.8, 200, 1, 0.488, 0, 1, 1, 2
  out aout
endin
</CsInstruments>
<CsScore>
f 1 0 256 10 1
f 2 0 256 10 1
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
