<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "fail when struct init provides only some members",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      ":Point; init c"
    ]
  }
}
*/
sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

struct Point x:i, y:i

instr 1
  point:Point init 7
endin

</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
