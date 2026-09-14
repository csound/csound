<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "expected failure: opcode missing endop",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, unexpected INSTR_TOKEN"
    ]
  }
}
*/
; Test: opcode definition missing endop.
; Expected: parse failure without any crash.

opcode myop, a, k
  aout = 0

instr 1
  a1 myop 0.1
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
