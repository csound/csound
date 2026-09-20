<CsTest>
description = "ntom and ntof reject empty and malformed names at init and performance time"
[expect]
exit = "nonzero"
stderr_regex = ['INIT ERROR[^\n]*ntom: invalid note name', 'INIT ERROR[^\n]*ntof: invalid note name', 'PERF ERROR[^\n]*ntom: invalid note name', 'PERF ERROR[^\n]*ntof: invalid note name', '8 errors in performance']
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  Snote strget p4
  iMidi ntom Snote
endin
instr 2
  Snote strget p4
  iFreq ntof Snote
endin
instr 3, 4
  Sbad strget p4
  Snote init "4C"
  igoto ready
  Snote strcpyk Sbad
ready:
  if p1 == 3 then
    kMidi ntom Snote
  else
    kFreq ntof Snote
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 ""
i 2 .01 .01 "4"
i 1 .02 .01 "xA"
i 2 .03 .01 "4C?"
i 3 .04 .01 ""
i 4 .05 .01 "4C+xx"
i 3 .06 .01 "4C+100"
i 4 .07 .01 "4H"
e
</CsScore>
</CsoundSynthesizer>
