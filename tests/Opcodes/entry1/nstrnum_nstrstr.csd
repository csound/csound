<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

sr	= 44100
ksmps = 32
nchnls	= 2
0dbfs	= 1

instr nothing
endin

instr john
 prints "instrument name = "
 puts nstrstr(p1),1
 prints "instrument number = %d\n", nstrnum("john")
endin

instr test
endin

</CsInstruments>
<CsScore>
i "john" 0 0
</CsScore>
</CsoundSynthesizer>


