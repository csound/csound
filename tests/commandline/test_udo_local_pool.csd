<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "test udos for separate local var pool",
  "expect": {
    "exit": 0
  }
}
*/
0dbfs = 1

Sresult = "anything"

opcode test1, S[], 0
       Sresult[] fillarray "t1", "t2"
       xout Sresult
endop

instr 1
       Sresult[] test1
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


