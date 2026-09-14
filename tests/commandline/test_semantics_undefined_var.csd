<CsoundSynthesizer>
<CsInstruments>
/* Csound-test
{
  "description": "test undefined var",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, get_arg_type2: Variable 'k1' used before defined",
      "syntax error, Variable type for k1 could not be determined"
    ]
  }
}
*/
sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

a1 poscil k1
out a1

endin


</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
