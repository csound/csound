<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1

instr 1
ifno  ftgentmp  0, 0, 512, 10, 1
print ifno
endin

instr 2
print ftlen(p4)
endin

</CsInstruments>
<CsScore>
i 1 0 10
i 2 2 1 101
i 1 5 10
i 2 7 1 102
i 2 12 1 101 
i 2 17 1 102 
e
</CsScore>
</CsoundSynthesizer>
