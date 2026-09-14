<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "test string assignment and printing",
  "expect": {
    "exit": 0
  }
}
*/
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ShrtfL sprintf "/Users/ben/Documents/csound/hrtf/hrtf-%i-left.dat", sr


endin
</CsInstruments>
<CsScore>

i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
