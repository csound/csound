<CsTest>
description = "distort preserves stored centre knots and quiet detail on both sides"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
; The initial distortion scale is exactly 1000 with this 0dbfs.
0dbfs = 32768
giSmall ftgen 1, 0, 8, -2, 0, 0, 0, 1, pow(2,-26), 2, 0, 0
giZero ftgen 2, 0, 8, -2, 0, 0, 0, 1, 0, 2, 0, 0
giOdd ftgen 3, 0, -9, -2, 0, 0, 0, 0, 1, -1, 0, 0, 0
gkChecks init 0
instr Check
  aInput init 1000*p5
  aOutput distort aInput, 1, p4, 0
  kOutput downsamp aOutput
  if p6 == 0 then
    if kOutput != 0 then
      exitnowk -1
    endif
  elseif !(abs(kOutput/(1000*p6)-1) < .00001) then
    printks "Centre-knot interpolation: expected %g, got %g\n", 0, 1000*p6, kOutput
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin
instr Done
  if i(gkChecks) != 8 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Check" 0 [1/8000] 1 [-1/(2^30)] [5/(2^28)]
i "Check" 0 [1/8000] 1 0 [1/(2^26)]
i "Check" 0 [1/8000] 1 [1/(2^30)] [6/(2^28)]
i "Check" 0 [1/8000] 2 [-1/(2^30)] [1/(2^28)]
i "Check" 0 [1/8000] 2 0 0
i "Check" 0 [1/8000] 2 [1/(2^30)] [2/(2^28)]
; An odd table has its centre between knots.
i "Check" 0 [1/8000] 3 [-1/(2^30)] [9/(2^30)]
i "Check" 0 [1/8000] 3 [1/(2^30)] [-9/(2^30)]
i "Done" [2/8000] [1/8000]
e
</CsScore>
</CsoundSynthesizer>
