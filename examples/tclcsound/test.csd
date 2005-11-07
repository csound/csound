<CsoundSynthesizer>
<CsOptions>
-d
</CsOptions>
<CsInstruments>
sr=44100
ksmps=10
nchnls=1

gifn ftgen 1, 0, 16384, 10, 1
gk1 chnexport "freq_offset", 3
instr 1
asig oscili p4,p5+gk1,1;
     out asig
endin

</CsInstruments>
<CsScore>

i1 0 2 10000 440
i1 + 2 10000 660
i1 + 2 10000 550
i1 + 2 10000 330
i1 + 2 10000 220
</CsScore>

</CsoundSynthesizer>
