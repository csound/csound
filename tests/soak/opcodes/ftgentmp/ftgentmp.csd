<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1

instr 1
  ifno  ftgentmp 0, 0, 512, 10, 1
  asig = poscil:a(0.1, 220, ifno)
  outs asig, asig
endin

</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
