<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>

<CsInstruments>
/* Csound-test
{
  "description": "Test ftgen gen01 file input",
  "expect": {
    "exit": 0
  }
}
*/
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

itab ftgen 2, 0, 0, 1, "./pianoc2.wav", 0, 0, 0

instr 1
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>