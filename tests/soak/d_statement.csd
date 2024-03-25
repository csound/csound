<CsoundSynthesizer>

<CsInstruments>
instr 1
  a1  oscil 10000, 440
      out   a1
endin
instr sound
  a1  oscil 10000, 440
      out   a1
endin
</CsInstruments>

<CsScore>
f 0 10
i 1 0 -1
d 1 1 0

i "sound" 3 -1
i "-sound" 4 0
e
</CsScore>

</CsoundSynthesizer>
