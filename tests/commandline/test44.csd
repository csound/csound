<CsoundSynthesizer>

<CsInstruments>
/* Csound-test
{
  "description": "expected failure with in-arg given to in opcode",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, Unable to find opcode entry for 'in'"
    ]
  }
}
*/
sr=44100
ksmps=1
nchnls=1

instr 1
  kenv linseg  0, 0.1, 1^2, (p3+0.2), 1, 0.1, 0
  asig in 1
       out kenv*asig
endin

</CsInstruments>

<CsScore>

i 1 0 3
e

</CsScore>

</CsoundSynthesizer>
