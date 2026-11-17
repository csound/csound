<CsTest>
description = "i() in functional form refuses an expression, whatever the variable it writes is called"

[expect]
exit = "nonzero"
stderr = ["Use of i() with expression not permitted"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  value:k init 3
  ; an expression of k values has no value at init, so this would be 0
  written:i init 0
  written = i(value + 1)
  print written
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
