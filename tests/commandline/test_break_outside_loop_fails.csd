<CsoundSynthesizer>
<CsInstruments>

/* Csound-test
{
  "description": "testing break outside loop gives parser error",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "found break statement outside of loop."
    ]
  }
}
*/
instr 1
    a:i = 1
    b:i = 2
    c:i = 3
    break

    while (c>0) do
        c = c - 1
    od
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
