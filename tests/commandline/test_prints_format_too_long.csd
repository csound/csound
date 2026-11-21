<CsTest>
description = "prints refuses a format too long for its buffer, and prints the longest that fits"

[expect]
exit = "nonzero"
stderr_regex = [
  'INIT ERROR in instr 2 .*format too long in formatted printing',
  'INIT ERROR in instr 3 .*format too long in formatted printing',
  'INIT ERROR in instr 5 .*format too long in formatted printing',
]
output_regex = ['\[x{8189}\]\|']
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

opcode Xs(count:i):S
  text:S = "x"
  while strlen(text) < count do
    text = strcat(text, text)
  od
  xout strsub(text, 0, count)
endop

instr 1
  ; 8191 characters, the most the format's 8192 bytes hold.
  prints strcat(strcat("[", Xs(8189)), "]"), 0
  prints "|\n"
endin

instr 2
  ; One more.
  prints strcat("TOOLONG", Xs(8185)), 0
endin

instr 3
  ; A single tilde is two bytes: 8190 and two do not fit.
  prints strcat(strcat("TOOLONG", Xs(8183)), "~"), 0
endin

instr 4
  schedule 5, 0, .1, strcat("TOOLONG", Xs(20000))
endin

instr 5
  ; prints with a string p-field
  prints p4, 0
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
i 3 .2 .1
i 4 .3 .1
e
</CsScore>
</CsoundSynthesizer>
