<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  ifn = p4
  ibas = 1
  asig loscil 1, 1, ifn, ibas
  outs asig, asig
endin

instr 2
  isnd  = p4
  aread line   sr*p3, p3, 0
  asig  tablei aread, isnd
  outs   asig, asig
endin

</CsInstruments>
<CsScore>
f 1 0 131072 1 "beats.wav" 0 0 0
f 2 0    0   1 "flute.aiff" 0 0 0

i 1 0 1 1
i 1 + 1 2
i 2 4 2 1
e
</CsScore>
</CsoundSynthesizer>
