<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "expected failure: udo missing inargs",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, unexpected NEWLINE, expecting UDO_IDENT"
    ]
  }
}
*/
; Test: UDO signature missing in-arg list after comma.
; Expected: parse failure without any crash.

opcode myop, a,
  aout = 0
endop

</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
