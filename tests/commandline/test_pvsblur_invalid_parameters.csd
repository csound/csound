<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject invalid pvsblur parameters",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "pvsblur: invalid maximum delay",
      "pvsblur does not work sliding yet",
      "pvsblur: invalid frame size",
      "pvsblur: format must be amp-freq or amp-phase"
    ]
  }
}
*/
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  fInput pvsosc .5, 512, 4, 128, 32
  fOutput pvsblur fInput, 0, p4
endin
instr 2
  aInput = .5
  fInput pvsanal aInput, 64, 1, 64, 1
  fOutput pvsblur fInput, .01, .1
endin
instr 3
  kInput[] init 65
  fInput tab2pvs kInput, 32
  fOutput pvsblur fInput, .01, .1
endin
instr 4
  fInput pvsinit 128, 32, 128, 1, 2
  fOutput pvsblur fInput, .01, .1
endin
</CsInstruments>
<CsScore>
; Five init errors: negative/oversized delay, sliding, odd size, bad format.
i 1 0 .01 -1
i 1 0 .01 1e30
i 2 0 .01
i 3 0 .01
i 4 0 .01
e
</CsScore>
</CsoundSynthesizer>
