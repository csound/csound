<CsoundSynthesizer>

<CsInstruments>

/* Csound-test
{
  "description": "tests that empty branches do not cause compiler issues",
  "expect": {
    "exit": 0
  }
}
*/
sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

k1 init 0

if (k1 == 1) then
elseif (k1 == 2) then
else
endif

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
