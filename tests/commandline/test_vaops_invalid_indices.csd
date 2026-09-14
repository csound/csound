<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  aBuffer = 0
  kIndex init p4
  kRead vaget kIndex, aBuffer
endin
instr 2
  aBuffer = 0
  kIndex init p4
  vaset .5, kIndex, aBuffer
endin
instr 3
  aBuffer = 0
  kIndex init p4
  kRead = aBuffer[kIndex]
endin
instr 4
  aBuffer = 0
  kIndex init p4
  aBuffer[kIndex] = .5
endin
</CsInstruments>
<CsScore>
; Each form must reject these five indices before integer conversion.
i 1 0 .01 -1e-10
i 1 0 .01 -1
i 1 0 .01 32
i 1 0 .01 1e30
i 1 0 .01 -1e30
i 2 0 .01 -1e-10
i 2 0 .01 -1
i 2 0 .01 32
i 2 0 .01 1e30
i 2 0 .01 -1e30
i 3 0 .01 -1e-10
i 3 0 .01 -1
i 3 0 .01 32
i 3 0 .01 1e30
i 3 0 .01 -1e30
i 4 0 .01 -1e-10
i 4 0 .01 -1
i 4 0 .01 32
i 4 0 .01 1e30
i 4 0 .01 -1e30
e
</CsScore>
</CsoundSynthesizer>
