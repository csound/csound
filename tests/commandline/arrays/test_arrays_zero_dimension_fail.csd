<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "test expected failure with zero dimension in multi-dimensional array",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "Error: zero-size array initialization is only supported for 1-D arrays"
    ]
  }
}
*/
instr 1
  nums:i[][] init 0, 2
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
