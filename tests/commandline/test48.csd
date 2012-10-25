<CsoundSynthesizer>
<CsInstruments>
nchnls = 1

instr 1

acar oscil amod+.5, 1, 1
amod oscil .5, 220, 1
out acar

endin

</CsInstruments>

<CsScore>
f1 0 8192 10 1

i1 0 .1
</CsScore>
</CsoundSynthesizer>
