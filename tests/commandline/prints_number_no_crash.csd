<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>

sr	=	48000
ksmps	=	1
;nchnls	=	2
0dbfs	=	1

instr 1
  prints 1 ;; should give an error and not crash!
endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.01

</CsScore>
</CsoundSynthesizer>

