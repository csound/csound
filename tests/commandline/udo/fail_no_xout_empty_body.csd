<CsTest>
description = "fail due to no xout in an empty body"

[expect]
exit = "nonzero"
stderr = ["syntax error, invalid xout statement for UDO: defined 'i', found '(null)'", "syntax error, testUDO UDO"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 1

; without an xout, ival keeps what it held: 5
opcode testUDO, i, i
endop

instr 1
ival init 5
ival testUDO 1
prints "ival = %g\n", ival
turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
