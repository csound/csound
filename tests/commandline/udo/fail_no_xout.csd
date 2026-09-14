<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

/* Csound-test
{
  "description": "fail due to no xout",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, invalid xout statement for UDO: defined 'i', found '(null)'",
      "syntax error, testUDO UDO"
    ]
  }
}
*/
sr	=	44100
ksmps	=	1
;nchnls	=	2
0dbfs	=	1

opcode testUDO, i, i
ival xin
endop

instr 1	
ival testUDO 1
turnoff
endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.1


</CsScore>
</CsoundSynthesizer>

