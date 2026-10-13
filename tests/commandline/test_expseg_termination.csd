<CsTest>
description = "Exponential envelopes continue the last valid curve after a nonpositive later duration"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckContinuation
  setksmps p4
  ; Rise from 1 to 2 in eight samples, then decay to 1 in four.
  ; The nonpositive duration must stop initialization before its zero endpoint.
  aCurve expsega 1, .25, 2, .125, 1, p5, 0, .25, 8
  aActive = 1
  kElapsed init 0
  kSample = 0
  while kSample < ksmps do
    kActive vaget kSample, aActive
    kActual vaget kSample, aCurve
    kExpected = 0
    if kActive != 0 then
      kExpected = (kElapsed < 8 ? 2^(kElapsed/8) : 2*2^(-(kElapsed-8)/4))
      kElapsed += 1
    endif
    if !(abs(kActual-kExpected) < .00001) then
      printks "expsega duration=%g sample=%g: expected %g, got %g\n", \
        0, p5, kElapsed, kExpected, kActual
      exitnowk -1
    endif
    kSample += 1
  od
  gkChecks += 1
endin

instr CheckBothRates
  setksmps 1
  ; At ksmps=1, expseg at both rates must follow the same curve.
  kCurve expseg 1, .25, 2, p4, 0, .25, 8
  aCurve expseg 1, .25, 2, p4, 0, .25, 8
  kAudio downsamp aCurve
  kSample init 0
  kExpected = 2^(kSample/8)
  if !(abs(kCurve-kExpected) < .00001 && abs(kAudio-kExpected) < .00001) then
    printks "expseg duration=%g sample=%g: expected %g, got control=%g audio=%g\n", \
      0, p4, kSample, kExpected, kCurve, kAudio
    exitnowk -1
  endif
  kSample += 1
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 88 then
    prints "Envelope termination checks: expected 88, got %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, local blocks, and partial first/last blocks.
i "CheckContinuation" 0 .5 8 0
i "CheckContinuation" 0 .5 8 -1
i "CheckContinuation" .5 .5 2 0
i "CheckContinuation" .5 .5 2 -1
i "CheckContinuation" [1+1/32] [14/32] 8 0
i "CheckContinuation" [1+1/32] [14/32] 8 -1
i "CheckBothRates" 1.5 1 0
i "CheckBothRates" 1.5 1 -1
i "CheckResults" 2.5 .25
e
</CsScore>
</CsoundSynthesizer>
