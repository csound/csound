<CsTest>
description = "Repeated exponential breakpoints jump to the new value and repeated final points hold it"
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

instr CheckBreakpoints
  ; At sample 4 jump from 2 to 4, then decay to 2 at sample 8.
  aMiddle expsegba 1, .125, 2, .125, 4, .25, 2
  aMiddleOld expsegb 1, .125, 2, .125, 4, .25, 2
  kMiddle expsegb 1, .125, 2, .125, 4, .25, 2
  ; A repeated final breakpoint must hold its endpoint, not multiply by infinity.
  aFinal expsegba 1, .125, 2, .125, 4
  aFinalOld expsegb 1, .125, 2, .125, 4
  kFinal expsegb 1, .125, 2, .125, 4
  kAudioMiddle downsamp aMiddle
  kAudioMiddleOld downsamp aMiddleOld
  kAudioFinal downsamp aFinal
  kAudioFinalOld downsamp aFinalOld
  kSample init 0
  kExpectedMiddle = (kSample < 4 ? 2^(kSample/4) : 4*2^(-(kSample-4)/4))
  kExpectedFinal = (kSample < 4 ? 2^(kSample/4) : 4)
  if !(abs(kAudioMiddle-kExpectedMiddle) < .00001 && \
       abs(kAudioMiddleOld-kExpectedMiddle) < .00001 && \
       abs(kMiddle-kExpectedMiddle) < .00001 && \
       abs(kAudioFinal-kExpectedFinal) < .00001 && \
       abs(kAudioFinalOld-kExpectedFinal) < .00001 && \
       abs(kFinal-kExpectedFinal) < .00001) then
    printks "breakpoint sample %g: expected middle=%g final=%g; got middle=%g/%g/%g final=%g/%g/%g\n", \
      0, kSample, kExpectedMiddle, kExpectedFinal, kAudioMiddle, kAudioMiddleOld, kMiddle, kAudioFinal, kAudioFinalOld, kFinal
    exitnowk -1
  endif
  kSample += 1
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 12 then
    prints "Breakpoint checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckBreakpoints" 0 .375
i "CheckResults" .375 .03125
e
</CsScore>
</CsoundSynthesizer>
