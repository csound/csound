<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>

/* Csound-test
{
  "description": "test prints does not crash when given a number arguments",
  "expect": {
    "exit": 0
  }
}
*/
sr	=	48000
ksmps	=	1
;nchnls	=	2
0dbfs	=	1

instr 1
  prints 1 // should not crash ever
endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.01

</CsScore>
</CsoundSynthesizer>

