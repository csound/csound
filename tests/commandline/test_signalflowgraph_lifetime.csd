<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

connect "Source", "value", "Sink", "value"

instr Source
  iTable ftgenonce 0, 0, 16, 10, 1
  kValue table 0, iTable
  outletk "value", kValue
endin

instr Sink
  kValue inletk "value"
endin
</CsInstruments>
<CsScore>
i "Sink" 0 0.15
i "Source" 0 0.03
i "Source" 0.05 0.03
i "Source" 0.10 0.03
e
</CsScore>
</CsoundSynthesizer>
