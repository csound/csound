<CsTest>
description = "fail due to invalid xout"

[expect]
exit = "nonzero"
stderr = ["syntax error, invalid xout statement for UDO: defined 'i', found 'ic'", "syntax error, testUDO UDO"]
</CsTest>
<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
;nchnls	=	2
0dbfs	=	1

opcode testUDO, i, i
ival xin
xout ival, 45
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

