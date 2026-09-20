<CsTest>
description = "minarray and maxarray reject arrays trimmed to zero at either rate"
[expect]
exit = "nonzero"
stderr_regex = ['INIT ERROR[^\n]*minarray/maxarray: empty array', 'PERF ERROR[^\n]*minarray/maxarray: empty array', '4 errors in performance']
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iValues[] fillarray 3, 4
  trim_i iValues, 0
  iMin minarray iValues
endin
instr 2
  iValues[] fillarray 3, 4
  trim_i iValues, 0
  iMax maxarray iValues
endin
instr 3
  kValues[] fillarray 5, 6
  trim kValues, 0
  kMin minarray kValues
endin
instr 4
  kValues[] fillarray 5, 6
  trim kValues, 0
  kMax maxarray kValues
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 .01 .01
i 3 .02 .01
i 4 .03 .01
e
</CsScore>
</CsoundSynthesizer>
