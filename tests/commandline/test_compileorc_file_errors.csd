<CsTest>
description = "compileorc returns errors without hanging or stopping the caller"
[expect]
exit = 0
stderr = ["compileorc file errors handled"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1

instr 1
  iResult compileorc "."
  if iResult != -1 then
    exitnow 1
  endif
  iResult compileorc "compileorc_missing_file.orc"
  if iResult != -1 then
    exitnow 1
  endif
  fprints "compileorc_empty.orc", "%s", ""
  iResult compileorc "compileorc_empty.orc"
  if iResult != -1 then
    exitnow 1
  endif
  fprints "compileorc_valid.orc", "instr CompiledFromFile\nendin\n"
  iResult compileorc "compileorc_valid.orc"
  if iResult != 0 || nstrnum("CompiledFromFile") <= 0 then
    exitnow 1
  endif
  prints "compileorc file errors handled\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
