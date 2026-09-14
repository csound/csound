<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "expected failure: unary + parse error",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, unexpected NEWLINE"
    ]
  }
}
*/
; Test: unary '+' missing operand.
; Expected: parse failure without any crash.

instr 1
  ival init 0
  ival = +
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
