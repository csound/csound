<CsTest>
description = "readfi starts at the first line after reinit"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
strset 9177, "readf_lines.txt"

instr 1
  cycle:k timeinstk
  if (cycle == 1) then
    reinit read
  endif
read:
  for attempt in [1 ... 5] do
    line:S, lineNumber:i readfi "readf_lines.txt"
    numberedLine:S, numberedLineNumber:i readfi 9177
    expectedNumber:i = (attempt <= 2 ? attempt : -1)
    if (lineNumber != expectedNumber || numberedLineNumber != expectedNumber) then
      prints "Read %d: expected line %d, got %d and %d\n", \
             attempt, expectedNumber, lineNumber, numberedLineNumber
      exitnow(1)
    endif
    if (strcmp(line, numberedLine) != 0) then
      prints "String and numeric file arguments returned different text\n"
      exitnow(1)
    endif
  od
  rireturn
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
