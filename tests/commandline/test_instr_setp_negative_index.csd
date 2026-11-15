<CsTest>
description = "setp on an Instr instance ignores a negative or NaN index, as it ignores one past the instrument's p-fields, writes nothing before p0, and truncates a fraction as before"

[expect]
exit = 0
output = [
  "index 4: p4 1234 p5 7",
  "index 5: p4 3 p5 1234",
  "index 7: p4 3 p5 7",
  "index 1234: p4 3 p5 7",
  "index -1: p4 3 p5 7",
  "index -1e+06: p4 3 p5 7",
  "index NaN: p4 3 p5 7",
  "the next note ran",
]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

instr Voice ; p6, the index, is its last p-field
  if timeinstk() == 2 then
    if p6 == p6 then
      printks "index %g: p4 %g p5 %g\n", 0, p6, p4, p5
    else
      printks "index NaN: p4 %g p5 %g\n", 0, p4, p5
    endif
  endif
endin

; Voice is given p4 = 3, p5 = 7 and, in p6, the index set before each of
; its passes. A NaN where p4 is 0.
instr 1
  index:i = p4 == 0 ? sqrt(-1) : p4
  voice:Instr = create(Voice)
  err:i = splice(voice, this, 0)
  err1:i = init(voice, 3, 7, index)
  setp(voice, index, 1234)
  err2:k = perf(voice)
  delete(voice)
endin

instr 2
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.03 4
i 1 0.1 0.03 5
i 1 0.2 0.03 7
i 1 0.25 0.03 6.5
i 1 0.3 0.03 -1
i 1 0.4 0.03 -1000000
i 1 0.5 0.03 0
i 2 0.6 0.1
e
</CsScore>
</CsoundSynthesizer>
