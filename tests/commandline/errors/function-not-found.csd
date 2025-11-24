<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2

instr 1
  ; use a function name that does not exist in an expression
  a1 oscili 0.1, 440
  kx = not_a_function(a1)
  outs a1, a1
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
