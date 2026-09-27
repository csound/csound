<CsTest>
description = "Exponential segments retain slow changes through boundaries and continuation"
[expect]
exit = 0
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
gkChecks init 0

instr CheckCurves
  ; The two segments last 32 seconds each. Breakpoint times are absolute.
  kDuration expseg p4, 32, p5, 32, p6
  kBreakpoint expsegb p4, 32, p5, 64, p6
  aDuration expseg p4, 32, p5, 32, p6
  aBreakpoint expsegb p4, 32, p5, 64, p6
  aSampleDuration expsega p4, 32, p5, 32, p6
  aSampleBreakpoint expsegba p4, 32, p5, 64, p6
  kAudioDuration downsamp aDuration
  kAudioBreakpoint downsamp aBreakpoint
  kAudioSampleDuration downsamp aSampleDuration
  kAudioSampleBreakpoint downsamp aSampleBreakpoint
  kStep init 0
  ; Check midpoints, the segment boundary, and motion past the final point.
  if kStep == 16*kr || kStep == 32*kr || kStep == 48*kr || kStep == 64*kr || kStep == 80*kr then
    kTime = kStep/kr
    if kTime < 32 then
      kExpected = p4 * (p5/p4)^(kTime/32)
    else
      kExpected = p5 * (p6/p5)^((kTime-32)/32)
    endif
    if !(abs(kDuration-kExpected) < .00001 && abs(kBreakpoint-kExpected) < .00001) then
      printks "At %g seconds, expected %.9f: expseg control=%.9f, expsegb control=%.9f\n", 0, kTime, kExpected, kDuration, kBreakpoint
      exitnowk -1
    endif
    if !(abs(kAudioDuration-kExpected) < .00001 && abs(kAudioBreakpoint-kExpected) < .00001 && \
         abs(kAudioSampleDuration-kExpected) < .00001 && abs(kAudioSampleBreakpoint-kExpected) < .00001) then
      printks "At %g seconds, expected %.9f: expseg audio=%.9f, expsegb audio=%.9f, expsega=%.9f, expsegba=%.9f\n", 0, kTime, kExpected, kAudioDuration, kAudioBreakpoint, kAudioSampleDuration, kAudioSampleBreakpoint
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kStep += 1
endin

instr CheckCompletion
  if i(gkChecks) != 20 then
    prints "All four curves must reach all five check points\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Rising and falling curves, with positive and negative endpoints.
i "CheckCurves" 0 80.01 1 1.05 1.1
i "CheckCurves" 0 80.01 1.1 1.05 1
i "CheckCurves" 0 80.01 -1 -1.05 -1.1
i "CheckCurves" 0 80.01 -1.1 -1.05 -1
i "CheckCompletion" 81 .01
e
</CsScore>
</CsoundSynthesizer>
