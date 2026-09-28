<CsTest>
description = "reused spatial-send instruments cannot retain an ended source"
[expect]
exit = "nonzero"
stderr = [
  "locsend: no previous locsig in this instrument instance",
  "spsend: no previous space in this instrument instance",
  "2 errors in performance"
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 2
0dbfs = 1
instr Loc
  aSignal = .25
  if p4 == 1 then
    aD1, aD2 locsig aSignal, 0, 1, 1
  endif
  aL1, aL2 locsend
endin
instr Sp
  aSignal = .25
  if p4 == 1 then
    aD1, aD2, aD3, aD4 space aSignal, 0, 0, 1, 0, 0
  endif
  aL1, aL2, aL3, aL4 spsend
endin
</CsInstruments>
<CsScore>
i "Loc" 0 .01 1
i "Loc" .02 .01 0
i "Sp" .04 .01 1
i "Sp" .06 .01 0
e
</CsScore>
</CsoundSynthesizer>
