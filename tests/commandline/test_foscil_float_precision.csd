<CsTest>
description = "foscil retains fractional phase detail in float builds"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giShape ftgen 0, 0, -12, -2, 0, 1/3, 2/3, 1, 2/3, 1/3, 0, -1/3, -2/3, -1, -2/3, -1/3
gkChecks init 0

instr SlowCarrier
  ; Check both directions at a zero crossing and at cycle wraparound.
  ; p6 selects scalar, audio-rate, or in-place audio-rate parameters.
  aAmp = 1
  aCar init 1
  aMod init 0
  if p6 == 0 then
    aOut foscili 1, p5*sr/67108864, 1, 0, 0, giShape, p4
  elseif p6 == 1 then
    aOut foscili aAmp, p5*sr/67108864, aCar, aMod, 0, giShape, p4
  else
    aAmp foscili aAmp, p5*sr/67108864, aCar, aMod, 0, giShape, p4
    aOut = aAmp
  endif
  kBase init 0
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kExpected = (p4 == .5 ? -4 : 4)*p5*(kBase+kN)/67108864
    if !(abs(kActual-kExpected) < .00000001) then
      printks "Slow carrier phase=%g direction=%g mode=%g sample=%g: %g expected %g\n", 0, p4, p5, p6, kBase+kN, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBase += ksmps
  if kBase == 64 then
    gkChecks += 1
  endif
endin

instr SlowModulator
  ; A large FM index makes tiny modulator phase changes observable.
  aAmp init 1
  aCar init 0
  aMod init 1
  if p4 == 0 then
    aOut foscili 1, sr/67108864, 0, 1, 16777216, giShape, .5
  else
    aOut foscili aAmp, sr/67108864, aCar, aMod, 16777216, giShape, .5
  endif
  kBase init 0
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kSample = kBase+kN
    ; The carrier integrates -n/2^26 cycles per sample.
    kExpected = 2*kSample*(kSample-1)/67108864
    if !(abs(kActual-kExpected) < .00000001) then
      printks "Slow modulator mode=%g sample=%g: %g expected %g\n", 0, p4, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBase += ksmps
  if kBase == 64 then
    gkChecks += 1
  endif
endin

instr Boundary
  ; The initial phase is exactly four increments below the middle entry.
  ; Check the discontinuous lookup against a sample count, not a float phase.
  aAmp init 1
  aCar init 1
  aMod init 0
  if p4 == 0 then
    aOut foscil 1, sr/67108864, 1, 0, 0, giShape, .5-1/16777216
  else
    aOut foscil aAmp, sr/67108864, aCar, aMod, 0, giShape, .5-1/16777216
  endif
  kBase init 0
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kExpected = (kBase+kN < 4 ? 1/3 : 0)
    if !(abs(kActual-kExpected) < .0000001) then
      printks "Table boundary mode=%g sample=%g: %g expected %g\n", 0, p4, kBase+kN, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBase += ksmps
  if kBase == 64 then
    gkChecks += 1
  endif
endin

instr Reinit
  ; Reinit must preserve a nonzero rounding remainder with a negative phase,
  ; and clear it when a new initial phase is supplied.
  kBlock init 0
  kPhase init .5
  if kBlock == 2 then
    kPhase = -1
    reinit OSC
  elseif kBlock == 3 then
    kPhase = .5
    reinit OSC
  endif
OSC:
  aOut foscili 1, sr/1073741824, 1, 0, 0, giShape, i(kPhase)
  rireturn
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kSample = (kBlock == 3 ? kN : kBlock*ksmps+kN)
    kExpected = -4*kSample/1073741824
    if !(abs(kActual-kExpected) < .0000000001) then
      printks "Reinit block=%g sample=%g: %g expected %g\n", 0, kBlock, kN, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) != 17 then
    prints "Not all float phase checks completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "SlowCarrier" 0 .0625 .5 1 0
i "SlowCarrier" 0 .0625 .5 -1 0
i "SlowCarrier" 0 .0625 0 1 0
i "SlowCarrier" 0 .0625 0 -1 0
i "SlowCarrier" 0 .0625 .5 1 1
i "SlowCarrier" 0 .0625 .5 -1 1
i "SlowCarrier" 0 .0625 0 1 1
i "SlowCarrier" 0 .0625 0 -1 1
i "SlowCarrier" 0 .0625 .5 1 2
i "SlowCarrier" 0 .0625 .5 -1 2
i "SlowCarrier" 0 .0625 0 1 2
i "SlowCarrier" 0 .0625 0 -1 2
i "SlowModulator" 0 .0625 0
i "SlowModulator" 0 .0625 1
i "Boundary" 0 .0625 0
i "Boundary" 0 .0625 1
i "Reinit" 0 .0625
i "CheckCompletion" .125 .015625
e
</CsScore>
</CsoundSynthesizer>
