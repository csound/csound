<CsTest>
description = "ntom and ntof reject empty and malformed names at init and performance time"
[expect]
exit = "nonzero"
stderr = [
  'ntom: invalid note name "": note name is empty',
  'ntof: invalid note name "4": expected a note letter after the octave',
  'ntom: invalid note name "xA": octave must be a digit from 0 to 9',
  'ntof: invalid note name "4C?": expected # or b for an accidental, or + or - for cents',
  'ntof: invalid note name "4C+xx": cents must contain only decimal digits',
  'ntom: invalid note name "4C+100": cents must have at most two digits',
  'ntof: invalid note name "4H": expected an uppercase note letter from A to G',
  'ntom: invalid note name "4C#+100": note name must have at most six characters',
  'ntof: invalid note name "4c": expected an uppercase note letter from A to G',
  'ntom: invalid note name "4Cb#": expected + or - after the accidental',
  'ntof: invalid note name "4C#12": expected + or - after the accidental',
]
stderr_regex = [
  'INIT ERROR[^\n]*ntom: invalid note name',
  'INIT ERROR[^\n]*ntof: invalid note name',
  'PERF ERROR[^\n]*ntom: invalid note name',
  'PERF ERROR[^\n]*ntof: invalid note name',
  '12 errors in performance',
]
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
  ; Let the opcode initialize with a valid name, then reject Sbad at k-rate.
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
; Initialization errors include the rejected name and its parsing problem.
i 1 0 .01 ""
i 2 .01 .01 "4"
i 1 .02 .01 "xA"
i 2 .03 .01 "4C?"

; These notes initialize with 4C before switching to the invalid name.
i 3 .04 .01 ""
i 4 .05 .01 "4C+xx"
i 3 .06 .01 "4C+100"
i 4 .07 .01 "4H"

; Long names, lowercase letters and malformed accidentals remain invalid.
i 1 .08 .01 "4C#+100"
i 2 .09 .01 "4c"
i 3 .10 .01 "4Cb#"
i 4 .11 .01 "4C#12"
e
</CsScore>
</CsoundSynthesizer>
