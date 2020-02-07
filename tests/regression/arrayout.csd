<CsoundSynthesizer>

<CsInstruments>
nchnls = 1
instr 1
  asig[] init 2
  asig[0] oscil 0dbfs/2, A4
  asig[1] oscil 0dbfs/2, A4/2
          out   1.5*asig
endin
</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>

</CsoundSynthesizer>
