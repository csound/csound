<CsTest>
description = "score y seeds retain exact integer values in every build precision"
[expect]
exit = 0
output = ["score seed spellings agree", "y: seed 1e39 out of range, ignored"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
giValue[] init 64
giReference[] init 64
giCount init 0
instr Capture
  giValue[p5] = p4
  giReference[p5] = p6
  giCount += 1
endin
instr Check
  if giCount != 36 then
    prints "FAIL score seed cases did not complete\n"
    exitnow 1
  endif
  iN = 0
  while iN < giCount do
    iReference = giReference[iN]
    if giValue[iN] != giValue[iReference] then
      prints "FAIL score seed case %g: %.12g expected %.12g\n", iN, giValue[iN], giValue[iReference]
      exitnow 1
    endif
    iN += 1
  od
  if giValue[1] == giValue[2] then
    prints "FAIL adjacent score seeds gave the same number\n"
    exitnow 1
  endif
  prints "score seed spellings agree\n"
endin
</CsInstruments>
<CsScore>
; The first five cases are reference seeds; each later case names its reference.
y 123
i "Capture" 0 0 [~] 0 0
y 2
i "Capture" 0 0 [~] 1 1
y 3
i "Capture" 0 0 [~] 2 2
y 3000000001
i "Capture" 0 0 [~] 3 3
y 1
i "Capture" 0 0 [~] 4 4
y 4294967294
i "Capture" 0 0 [~] 5 1
y 4294967295
i "Capture" 0 0 [~] 6 2
y +0004294967295
i "Capture" 0 0 [~] 7 2
y 4294967295.999999999999999999999
i "Capture" 0 0 [~] 8 2
y 3000000001.75
i "Capture" 0 0 [~] 9 3
y 3.000000001e9
i "Capture" 0 0 [~] 10 3
y 30000000010000000000000000000000000000000e-31
i "Capture" 0 0 [~] 11 3
y .000000003000000001e18
i "Capture" 0 0 [~] 12 3
y 42949672950e-1
i "Capture" 0 0 [~] 13 2
y 0xffffffff
i "Capture" 0 0 [~] 14 2
y 0Xffffffff.f
i "Capture" 0 0 [~] 15 2
y 0x1ffffffffp-1
i "Capture" 0 0 [~] 16 2
y 0x1.fffffffeP31
i "Capture" 0 0 [~] 17 2
y 0x.ffffffffp32
i "Capture" 0 0 [~] 18 2
y 0xfffffffe
i "Capture" 0 0 [~] 19 1
y 0
i "Capture" 0 0 [~] 20 4
y -0
i "Capture" 0 0 [~] 21 4
y -.999999999999999999999
i "Capture" 0 0 [~] 22 4
y -0x.8
i "Capture" 0 0 [~] 23 4
y 0e999999999999999999999999999999999
i "Capture" 0 0 [~] 24 4
y 1e-999999999999999999999999999999999
i "Capture" 0 0 [~] 25 4
y 3e+
i "Capture" 0 0 [~] 26 2
y 3junk
i "Capture" 0 0 [~] 27 2
y 123
y -1
i "Capture" 0 0 [~] 28 0
y 123
y -0x.8p1
i "Capture" 0 0 [~] 29 0
y 123
y 4294967296
i "Capture" 0 0 [~] 30 0
y 123
y 0x100000000
i "Capture" 0 0 [~] 31 0
y 123
y 1e999999999999999999999999999999999
i "Capture" 0 0 [~] 32 0
y 123
y +INF
i "Capture" 0 0 [~] 33 0
y 123
y +NaN
i "Capture" 0 0 [~] 34 0
; A rejected value beyond float range keeps its spelling and the prior seed.
y 123
y 1e39
i "Capture" 0 0 [~] 35 0
i "Check" .1 0
e
</CsScore>
</CsoundSynthesizer>
