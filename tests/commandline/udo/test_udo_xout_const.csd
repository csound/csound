<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
;nchnls	=	2
0dbfs	=	1

opcode test_udo, k,0
 xout 0
endop

instr 1	
  kval = test_udo()

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>

