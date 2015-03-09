<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	

a1, a2 = pan2(vco2(0.5,440), 0.0)

outs a1, a2

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.1


</CsScore>
</CsoundSynthesizer>

