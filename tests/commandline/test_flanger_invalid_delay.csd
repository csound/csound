<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject non-finite flanger delay",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "flanger: delay is outside imaxd"
    ]
  }
}
*/
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  asignal = .25
  kdelay = sqrt(-1)
  adelay = kdelay
  aout flanger asignal, adelay, 0, .01
endin

instr 2
  asignal = .25
  adelay = .02
  aout flanger asignal, adelay, 0, .01
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
</CsScore>
</CsoundSynthesizer>
