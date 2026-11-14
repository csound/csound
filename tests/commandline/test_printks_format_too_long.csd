<CsTest>
description = "printks refuses a format too long for its buffer, given at init or grown during performance"

[expect]
exit = "nonzero"
stderr_regex = [
  'INIT ERROR in instr 1 .*format too long in formatted printing',
  'INIT ERROR in instr 3 .*format too long in formatted printing',
  'PERF ERROR in instr 4 .*format too long in formatted printing',
]
output = ["short 1|"]
output_excludes = ["AddressSanitizer", "runtime error:", "TOOLONG"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

gSformat init "short %d|\n"

opcode Xs(count:i):S
  text:S = "x"
  while strlen(text) < count do
    text = strcat(text, text)
  od
  xout strsub(text, 0, count)
endop

instr 1
  ; 8192 characters, one more than the format's 8192 bytes hold.
  printks strcat("TOOLONG", Xs(8185)), 0
endin

instr 2
  schedule 3, 0, .1, strcat("TOOLONG", Xs(20000))
endin

instr 3
  ; printks with a string p-field
  printks p4, 0
endin

instr 4
  kCycle timeinstk
  printks gSformat, 0, kCycle
endin

instr 5
  gSformat = strcat("TOOLONG", Xs(20000))
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
i 4 .2 .3
i 5 .3 .1
e
</CsScore>
</CsoundSynthesizer>
