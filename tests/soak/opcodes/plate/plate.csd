<CsoundSynthesizer>
<CsInstruments>
; Note: this example is heavy on CPU
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

  ab    diskin2  "beats.wav", 1
  al,ar platerev 1, 2, p4, 0.73, 1.0, 5.0, 0.001, ab,ab
      outs      al*.25,ar*.25

endin
</CsInstruments>
<CsScore>
f1 0 8 -2   0.3   0.3875    0.39274  0.32   0.85714 0.78548
f2 0 8 -2   0.2   0.666667  1.57097  0.24   0.75    0.78548
i1 0 1 1
i1 + 1 2
e

</CsScore>
</CsoundSynthesizer>
