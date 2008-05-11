<CsoundSynthesizer>
<CsInstruments>
sr=48000
ksmps=1
nchnls=2

instr 1
imc   la_i_mc_create 5, 5, p4, p5
      la_i_print_mc imc
endin

</CsInstruments>
<CsScore>
i 1 1 1 0 0
i 1 2 1 -1 1
e
</CsScore>
</CsoundSynthesizer>
