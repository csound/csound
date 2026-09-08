<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  ; In a float build, these products round to 40 and 2 or 20 samples.
  ; Promoting the operands before multiplying can lose a sample.
  aout granule 1, 1, 1, p4, 0, 1, 1, 0.005, 0, p5, 0, 0, 0.02, 0, 0, 0, 0.5, 1, 1, 1, 1, 0
  ksample downsamp aout
  kcount init 0
  kexpected = 40 + ((p4 * kcount % p6) + p6) % p6
  if !(abs(ksample - kexpected) <= 0.001) then
    printks "grain4 region mismatch: mode %.0f, length %.0f, sample %.0f, expected %.0f, got %.6f\n", 0, p4, p6, kcount, kexpected, ksample
    exitnowk(-1)
  endif
  kcount += 1
  if kcount >= 2 * p6 + 1 then
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 64 -7 0 64 64
i 1 0 0.006  1 0.00025 2
i 1 0 0.006 -1 0.00025 2
i 1 0 0.006  1 0.0025 20
i 1 0 0.006 -1 0.0025 20
</CsScore>
</CsoundSynthesizer>
