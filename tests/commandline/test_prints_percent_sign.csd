<CsTest>
description = "prints, printks and printks2 print one percent sign for a % before a blank, and for %c of 37, up to the end of the buffer"

[expect]
exit = "nonzero"
stderr_regex = [
  'INIT ERROR in instr 2 .*a format too long',
  'INIT ERROR in instr 3 .*a format too long',
]
output = ["50 % done 1|", "[%]|", "tab %\t3|", "k 50 % done 2|", "k [%]|", "k2 [%]|"]
output_regex = [' {8188}1% \|fits']
output_excludes = ["%%", "|over"]
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

instr 1
  prints "50 % done %d|\n", 1
  prints "[%c]|\n", 37
  prints "tab %\t%d|\n", 3
  printks "k 50 % done %d|\n", 0, 2
  printks "k [%c]|\n", 0, 37
  printks2 "k2 [%c]|\n", 37
  ; 8189 characters and a percent sign and a blank: 8191, which fit.
  prints "%8189.0f% ", 1
  prints "|fits\n"
  turnoff
endin

instr 2
  ; One more does not.
  prints "%8190.0f% ", 1
  prints "|over\n"
endin

instr 3
  ; A wide conversion, then more percent signs and blanks than fit after it.
  Sformat = "%8000.0f"
  iCount = 0
  while iCount < 200 do
    Sformat strcat Sformat, "% "
    iCount += 1
  od
  prints Sformat, 1
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
i 3 .2 .1
e
</CsScore>
</CsoundSynthesizer>
