<CsoundSynthesizer>

<CsInstruments>
sr=44100
ksmps=1
nchnls=1

instr 1
  kenv linseg  0, 0.1, 1^2, (p3+0.2), 1, 0.1, 0
  asig in
       out kenv*asig
endin

</CsInstruments>

<CsScore>

i 1 0 3
e

</CsScore>

</CsoundSynthesizer>