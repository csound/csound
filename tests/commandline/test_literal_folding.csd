<CsTest>
description = "Literal folding keeps note inputs, branches and division warnings"
args = ["-n", "-d", "-m4", "--optimize=constant-folding"]
[expect]
exit = 0
output = ["first=15 second=30", "first=15 second=20", "Division by zero"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  ; Fold the literals, but keep the branch and the per-note value of p4.
  iFirst = (2 + 3) * (4 - 1)
  if p4 == 2 then
    iSecond = iFirst * p4
  else
    iSecond = (7 - 2) * 4
  endif
  prints "first=%g second=%g\n", iFirst, iSecond

  ; Division must still run and report its warning even if no one uses it.
  kZero init 0
  kUnused = 1 / kZero
endin
</CsInstruments>
<CsScore>
i 1 0 .001 2
i 1 .002 .001 3
e
</CsScore>
</CsoundSynthesizer>
