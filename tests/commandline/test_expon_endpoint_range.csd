<CsTest>
description = "expon accepts same-sign endpoints across the sample type's range"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckRange
  iStart = 10^p4
  iEnd = 10^p5
  ; Skip endpoints that the sample type cannot represent.
  if iStart == 0 || iEnd == 0 || iStart == 2*iStart || iEnd == 2*iEnd then
    turnoff
  else
    kCurve expon p6*iStart, 1, p6*iEnd
    aCurve expon p6*iStart, 1, p6*iEnd
    kAudio downsamp aCurve
    kStep init 0
    if kStep == 16 || kStep == 32 then
      ; The logarithm varies linearly. Do not form iEnd/iStart here.
      kExpected = p6 * 10^(p4 + (p5-p4)*kStep/sr)
      if !(abs(kCurve/kExpected-1) < .00001 && abs(kAudio/kExpected-1) < .00001) then
        printks "expon powers %g to %g, sign %g, step %g: expected %g, got %g/%g\n", 0, p4, p5, p6, kStep, kExpected, kCurve, kAudio
        exitnowk -1
      endif
      gkChecks += 1
    endif
    kStep += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) < 8 then
    prints "The four float-range cases must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; These endpoint ratios exceed the float range; the curves remain finite.
i "CheckRange" 0 [33/32] -30 30 1
i "CheckRange" 0 [33/32] 30 -30 1
i "CheckRange" 0 [33/32] -30 30 -1
i "CheckRange" 0 [33/32] 30 -30 -1
; These endpoint ratios also exceed the double range.
i "CheckRange" 0 [33/32] -200 200 1
i "CheckRange" 0 [33/32] 200 -200 1
i "CheckRange" 0 [33/32] -200 200 -1
i "CheckRange" 0 [33/32] 200 -200 -1
; Multiplying these same-sign endpoints underflows to zero.
i "CheckRange" 0 [33/32] -200 -180 1
i "CheckRange" 0 [33/32] -200 -180 -1
i "CheckCompletion" 1.1 .03125
e
</CsScore>
</CsoundSynthesizer>
