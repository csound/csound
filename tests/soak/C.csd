<CsoundSynthesizer>

<CsInstruments>
0dbfs = 1

instr 1
ii pcount
print ii
  a1  oscil 0.5,A4
  outs a1,a1
  endin
</CsInstruments>

<CsScore>


i1   0    .5        100
i .  +
i .  .    .         !
i

s
i1   0    .5        100
i .  +    .
C    0
i .  .    .
i .  .
i .


e
</CsScore>

</CsoundSynthesizer>
