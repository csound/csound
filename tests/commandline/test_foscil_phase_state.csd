<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
; Explicit knots keep quarter-cycle values exact in both sample formats.
giPower ftgen 1, 0, 8, -2, 0, .5, 1, .5, 0, -.5, -1, -.5
giOther ftgen 2, 0, -12, -2, 0, .3333333333333333, .6666666666666666, \
    1, .6666666666666666, .3333333333333333, 0, -.3333333333333333, \
    -.6666666666666666, -1, -.6666666666666666, -.3333333333333333
gkChecks init 0

instr 1
  iOffset = int(p2*sr + .5) % ksmps
  ; Large test frequencies are exact whole cycles. Keep the oracle small.
  iStep = (abs(p6) > 1e10 ? 0 : p6)
  kCount init 0
  kCarrierPhase init p5 - floor(p5)
  kModPhase init p5 - floor(p5)
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  aAmp1 init 0
  aAmp2 init 0
  aCarrier init 0
  aMod = 4
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN - kStart
    vaset .25 + kSample/128, kN, aAmp1
    vaset .25 + kSample/128, kN, aAmp2
    vaset 1 + (kSample % 4)/4, kN, aCarrier
    kN += 1
  od
  if p8 == 0 then
    aPlain foscil .5, p6*sr, 1, 4, p7, p4, p5
    aInterp foscili .5, p6*sr, 1, 4, p7, p4, p5
  elseif p8 == 1 then
    aPlain foscil aAmp1, p6*sr, aCarrier, aMod, p7, p4, p5
    aInterp foscili aAmp2, p6*sr, aCarrier, aMod, p7, p4, p5
  else
    aAmp1 foscil aAmp1, p6*sr, aCarrier, aMod, p7, p4, p5
    aAmp2 foscili aAmp2, p6*sr, aCarrier, aMod, p7, p4, p5
    aPlain = aAmp1
    aInterp = aAmp2
  endif
  kN = 0
  while kN < ksmps do
    kPlain vaget kN, aPlain
    kInterp vaget kN, aInterp
    kExpected1 = 0
    kExpected2 = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kAmp = (p8 == 0 ? .5 : .25 + kSample/128)
      kCarrier = (p8 == 0 ? 1 : 1 + (kSample % 4)/4)
      kWave1 table kCarrierPhase, p4, 1
      kWave2 tablei kCarrierPhase, p4, 1
      kExpected1 = kAmp*kWave1
      kExpected2 = kAmp*kWave2
      ; FM cases sample the modulator at quarter-cycle points, where
      ; both lookup methods agree. Other cases use a zero FM index.
      kModulator tablei kModPhase, p4, 1
      kAdvance = iStep*(kCarrier + p7*4*kModulator)
      kCarrierPhase += kAdvance - floor(kAdvance)
      kCarrierPhase -= floor(kCarrierPhase)
      kModPhase += iStep*4 - floor(iStep*4)
      kModPhase -= floor(kModPhase)
    endif
    if !(abs(kPlain - kExpected1) < .000002) || \
       !(abs(kInterp - kExpected2) < .000002) then
      printks "FAIL foscil table=%g phase=%g step=%g mode=%g sample=%g: %g/%g expected %g/%g\n", \
          0, p4, p5, p6, p8, kCount + kN - kStart, kPlain, kInterp, kExpected1, kExpected2
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 2
  ; A small increment must accumulate even when it is below a float's
  ; spacing at the starting phase. The triangle is linear in this region.
  aOut foscili 1, sr/67108864, 1, 0, 0, giOther, .5
  kBase init 0
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kExpected = -4*(kBase + kN)/67108864
    if !(abs(kActual - kExpected) < .0000001) then
      printks "FAIL foscili lost small phase increments: %g expected %g\n", 0, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBase += ksmps
  if kBase == 64 then
    gkChecks += 1
  endif
endin

instr 3
  aRef1 foscil .5, 64, 1, 4, .5, p4, .25
  aRef2 foscili .5, 64, 1, 4, .5, p4, .25
  kBlock init 0
  kPhase init .25
  if kBlock == 2 then
    kPhase = -1
    reinit OSC
  endif
OSC:
  aOut1 foscil .5, 64, 1, 4, .5, p4, i(kPhase)
  aOut2 foscili .5, 64, 1, 4, .5, p4, i(kPhase)
  rireturn
  kError1 max_k abs(aOut1 - aRef1), 1, 1
  kError2 max_k abs(aOut2 - aRef2), 1, 1
  if !(kError1 < .000001) || !(kError2 < .000001) then
    printks "FAIL foscil negative initial phase reset the oscillator\n", 0
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 18 then
    prints "FAIL foscil checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Quarter phase, whole initial cycles, and interpolation between entries.
i 1 0 .0625 1 .25 0 0 0
i 1 .1279296875 .0625 2 .25 0 0 0
i 1 .25 .0625 1 1.25 0 0 0
i 1 .3779296875 .0625 2 1.25 0 0 1
i 1 .5 .0625 2 .125 0 0 0
; FM and changing audio parameters, including input/output reuse.
i 1 .6279296875 .0625 1 .25 .0625 .5 0
i 1 .7529296875 .0625 2 .25 .0625 .5 0
i 1 .8779296875 .0625 1 .25 .0625 .5 1
i 1 1.0029296875 .0625 2 .25 .0625 .5 1
i 1 1.1279296875 .0625 1 .25 .0625 .5 2
i 1 1.2529296875 .0625 2 .25 .0625 .5 2
; Reverse playback and whole-cycle increments retain a fractional phase.
i 1 1.375 .0625 2 .25 -.25 0 0
i 1 1.5 .0625 2 .25 -1 0 1
i 1 1.625 .0625 2 .25 1099511627776 0 0
i 1 1.75 .0625 2 .25 -1099511627776 0 1
i 2 1.875 .0625
i 3 2 .0625 1
i 3 2.125 .0625 2
i 99 2.25 .015625
</CsScore>
</CsoundSynthesizer>
