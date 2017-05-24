Test that using xout in a UDO with a constant works.


<CsoundSynthesizer>
<CsInstruments>

sr	=	48000
ksmps	=	1
nchnls	=	2
0dbfs	=	1

opcode testXoutConst, k, 0
  xout 0.5 
endop

instr 1	
outc(vco2:a(testXoutConst(), 440))
endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5


</CsScore>
</CsoundSynthesizer>

