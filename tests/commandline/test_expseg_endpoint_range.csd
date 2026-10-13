<CsTest>
description = "Exponential segments preserve finite curves when endpoint ratios overflow or underflow"
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
    kDuration expseg p6*iStart, 1, p6*iEnd
    kBreakpoint expsegb p6*iStart, 1, p6*iEnd
    aDuration expseg p6*iStart, 1, p6*iEnd
    aBreakpoint expsegb p6*iStart, 1, p6*iEnd
    aSampleDuration expsega p6*iStart, 1, p6*iEnd
    aSampleBreakpoint expsegba p6*iStart, 1, p6*iEnd
    kAudioDuration downsamp aDuration
    kAudioBreakpoint downsamp aBreakpoint
    kAudioSampleDuration downsamp aSampleDuration
    kAudioSampleBreakpoint downsamp aSampleBreakpoint
    kStep init 0
    if kStep == 16 || kStep == 32 then
      ; Compute the expected logarithm without forming iEnd/iStart.
      kExpected = p6 * 10^(p4 + (p5-p4)*kStep/sr)
      if !(abs(kDuration/kExpected-1) < .00001 && abs(kBreakpoint/kExpected-1) < .00001) then
        printks "Powers %g to %g, step %g: expected %g, expseg control=%g, expsegb control=%g\n", 0, p4, p5, kStep, kExpected, kDuration, kBreakpoint
        exitnowk -1
      endif
      if !(abs(kAudioDuration/kExpected-1) < .00001 && abs(kAudioBreakpoint/kExpected-1) < .00001 && \
           abs(kAudioSampleDuration/kExpected-1) < .00001 && abs(kAudioSampleBreakpoint/kExpected-1) < .00001) then
        printks "Powers %g to %g, step %g: expected %g, expseg audio=%g, expsegb audio=%g, expsega=%g, expsegba=%g\n", 0, p4, p5, kStep, kExpected, kAudioDuration, kAudioBreakpoint, kAudioSampleDuration, kAudioSampleBreakpoint
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
; All endpoints and outputs fit in float precision.
i "CheckRange" 0 [33/32] -30 30 1
i "CheckRange" 0 [33/32] 30 -30 1
i "CheckRange" 0 [33/32] -30 30 -1
i "CheckRange" 0 [33/32] 30 -30 -1
; These ratios exceed the double range; the endpoints still fit.
i "CheckRange" 0 [33/32] -200 200 1
i "CheckRange" 0 [33/32] 200 -200 1
i "CheckRange" 0 [33/32] -200 200 -1
i "CheckRange" 0 [33/32] 200 -200 -1
i "CheckCompletion" 1.1 .03125
e
</CsScore>
</CsoundSynthesizer>
