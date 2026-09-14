<CsoundSynthesizer>

<CsInstruments>
/* Csound-test
{
  "description": "tests that empty UDOs do not cause compiler issues",
  "expect": {
    "exit": 0
  }
}
*/
sr=44100
ksmps=1
nchnls=1

opcode test, 0, 0
endop

	instr 1	;untitled
test
	endin


</CsInstruments>

<CsScore>
i1	0.0	.1	
e

</CsScore>

</CsoundSynthesizer>
