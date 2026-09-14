<CsoundSynthesizer>
<CsInstruments>

/* Csound-test
{
  "description": "test expected failure with negative dimension size and array",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "Error: sizes must be >= 0 for array initialization"
    ]
  }
}
*/
sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

gkArr[] init 5, -1

instr 1	

kArr[] init 5, -1

endin

</CsInstruments>
<CsScore>



</CsScore>
</CsoundSynthesizer>

