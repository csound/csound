<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>
/* Csound-test
{
  "description": "fail on redefinition of variable by array",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, Array variable name 'karr' used before as a different type"
    ]
  }
}
*/
instr 2
karr init 1
karr[] = karr + 4
printk2  karr
endin
</CsInstruments>
; ==============================================
<CsScore>
i2 0.1 0.001
</CsScore>