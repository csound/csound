<CsTest>
description = "Reject invalid preset tags, channels, controller pairs, and arrays"

[expect]
exit = "nonzero"
stderr = ["invalid preset tag", "expected controller/value pairs", "channel must be in 1..16", "controller and value must be in 0..127", "expected a controller preset array", "No such preset"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  kId ctrlpreset -1, 1, 7, 0
endin
instr 2
  kId ctrlpreset 1, 1, 7
endin
instr 3
  kId ctrlpreset 1, 0, 7, 0
endin
instr 4
  kId ctrlpreset 1, 1, 128, 0
endin
instr 5
  kArray[] fillarray 1, 1, 7
  kId ctrlpreset 1, kArray
endin
instr 6
  kArray[] fillarray 1, 17, 7, 0
  kId ctrlpreset 1, kArray
endin
instr 7
  kArray[] fillarray 1, 1, 7, -1
  kId ctrlpreset 1, kArray
endin
instr 8
  kId ctrlpreset 1, 1, 7, 0
  ctrlselect 0
endin
instr 9
  kId ctrlpreset 1e20, 1, 7, 0
endin
instr 10
  ctrlselect -1
endin
instr 11
  ctrlselect 9
endin
instr 12
  kArray[] init 0
  kId ctrlpreset 1, kArray
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
i 4 0 .01
i 5 0 .01
i 6 0 .01
i 7 0 .01
i 8 0 .01
i 9 0 .01
i 10 0 .01
i 11 0 .01
i 12 0 .01
</CsScore>
</CsoundSynthesizer>
