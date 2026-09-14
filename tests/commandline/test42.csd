<CsoundSynthesizer>

<CsInstruments>
/* Csound-test
{
  "description": "extended string",
  "expect": {
    "exit": 0
  }
}
*/
;
sr=44100
ksmps=1
nchnls=2

	instr 1	;untitled

kk = 100
        printks {{ string
and next %d line
}}, 100, kk

	endin


</CsInstruments>

<CsScore>
i1	0	2
e

</CsScore>

</CsoundSynthesizer>
