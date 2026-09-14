<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "test struct member access",
  "expect": {
    "exit": 0
  }
}
*/
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

struct Point x:i, y:i

instr 1
  ; Initialize struct
  point1:Point init 10, 20
  
  ; Try to access struct member (not assignment, just access)
  ix = point1.x
  iy = point1.y
  
  print ix, iy
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
