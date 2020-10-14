<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>

sr	=	48000
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	

prints(
  sprintf(
  "Test: %d\n", 
  1))

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.1
e

</CsScore>
</CsoundSynthesizer>

