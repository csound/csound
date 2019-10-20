<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
0dbfs  = 1
nchnls = 2

stack 100000

instr 1

  a1	oscils 0.7, 220, 0
  k1	line 0, p3, 1
  push "blah", 123.45, a1, k1
  push rnd(k1)
  k_rnd	pop
  S01, i01, a01, k01 pop
  ktrig	metro 5.0
  outs a01, a01

endin
</CsInstruments>
<CsScore>

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>
