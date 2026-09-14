<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject a GEN44 header without >",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "GEN44: Invalid matrix size"
    ]
  }
}
*/
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giMatrix ftgen 1, 0, 0, -44, "gen44_matrix_missing_close.txt"

instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
