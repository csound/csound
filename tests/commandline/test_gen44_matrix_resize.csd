<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  iLength = ftlen(1)
  iFirst = table:i(0, 1)
  iLast = table:i(3, 1)
  if iLength != 4 || iFirst != 1.0 || iLast != 3.0 then
    prints "GEN44 resized table does not match\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 1 44 "gen44_matrix_valid.txt"
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
