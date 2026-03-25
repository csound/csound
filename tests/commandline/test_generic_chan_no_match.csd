<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


instr 1
 cvar:Complex init 1,1
 chnset cvar, "Test1"
 kvar chnget "Test1"
endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
