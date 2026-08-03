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
  iFirst = table:i(0, 1)
  iLast = table:i(3, 1)
  if iFirst != 1.0 || iLast != 3.0 then
    prints "GEN44 matrix values do not match\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 0 44 "gen44_matrix_valid.txt"
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
