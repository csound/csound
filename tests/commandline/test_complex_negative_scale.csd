<CsTest>
description = "Polar complex magnitude, phase and logarithm agree with rectangular values after negative scaling"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Check(kValue:Complex, kExpectedReal:k, kExpectedImag:k, SCase:S):void
  kExpectedMagnitude = sqrt(kExpectedReal*kExpectedReal + kExpectedImag*kExpectedImag)
  kReal = real(kValue)
  kImag = imag(kValue)
  kMagnitude = abs(kValue)
  if !(abs(kReal-kExpectedReal) < .00001 && abs(kImag-kExpectedImag) < .00001 && abs(kMagnitude-kExpectedMagnitude) < .00001) then
    printks "%s: expected (%g,%g), magnitude %g; got (%g,%g), magnitude %g\n", \
      0, SCase, kExpectedReal, kExpectedImag, kExpectedMagnitude, kReal, kImag, kMagnitude
    exitnowk -1
  endif
  ; A zero value has no defined phase or finite logarithm.
  if kExpectedMagnitude > 0 then
    kExpectedPhase = taninv2(kExpectedImag, kExpectedReal)
    kLog:Complex = log(kValue)
    if !(abs(arg(kValue)-kExpectedPhase) < .00001 && abs(real(kLog)-log(kExpectedMagnitude)) < .00001 && abs(imag(kLog)-kExpectedPhase) < .00001) then
      printks "%s: phase or logarithm disagrees with rectangular form\n", 0, SCase
      exitnowk -1
    endif
  endif
endop

instr CheckScalar
  kSource:Complex = polar(complex(3, 4))
  kCycle timeinstk
  ; Exercise positive, negative and zero multipliers on successive cycles.
  kScale = (kCycle == 1 ? 2 : (kCycle == 2 ? -2 : 0))
  kLeft:Complex = kScale*kSource
  kRight:Complex = kSource*kScale
  Check(kLeft, 3*kScale, 4*kScale, "scalar * complex")
  Check(kRight, 3*kScale, 4*kScale, "complex * scalar")
  if kScale != 0 then
    kQuotient:Complex = kSource/kScale
    kReciprocal:Complex = kScale/kSource
    Check(kQuotient, 3/kScale, 4/kScale, "complex / scalar")
    Check(kReciprocal, 3*kScale/25, -4*kScale/25, "scalar / complex")
  endif

  ; An explicit signed radius represents the same value as negative scaling.
  kSigned:Complex = complex(-5, taninv2(4, 3), 1)
  kCanonical:Complex = polar(kSigned)
  Check(kSigned, -3, -4, "explicit negative radius")
  Check(kCanonical, -3, -4, "polar conversion of negative radius")

  gkChecks += 1
  if kCycle == 3 then
    turnoff
  endif
endin

instr CheckArrays
  ; Both forms of each operand must give the same result.
  kSource:Complex[] = [complex(3,4), polar(complex(3,4)), complex(-3,4), polar(complex(-3,4))]
  kScale[] = [-2, -2, 2, 2]
  kProduct:Complex[] = kSource*kScale
  kReverseProduct:Complex[] = kScale*kSource
  kQuotient:Complex[] = kSource/kScale
  kReciprocal:Complex[] = kScale/kSource
  kUniformProduct:Complex[] = -2*kSource
  kUniformQuotient:Complex[] = kSource/-2
  kInPlaceProduct:Complex[] = kSource
  kInPlaceProduct *= kScale
  kInPlaceQuotient:Complex[] = kSource
  kInPlaceQuotient /= kScale
  kCanonical:Complex[] = polar(kProduct)
  kMagnitudes[] = abs(kProduct)
  kPhases[] = arg(kProduct)
  kLogs:Complex[] = log(kProduct)
  aMagnitudes = abs(kProduct)
  aPhases = arg(kProduct)

  kIndex = 0
  while kIndex < 4 do
    kReal = (kIndex < 2 ? 3 : -3)
    kImag = 4
    kFactor = kScale[kIndex]
    Check(kProduct[kIndex], kReal*kFactor, kImag*kFactor, "array product")
    Check(kReverseProduct[kIndex], kReal*kFactor, kImag*kFactor, "real array * complex array")
    Check(kQuotient[kIndex], kReal/kFactor, kImag/kFactor, "array quotient")
    Check(kReciprocal[kIndex], kReal*kFactor/25, -kImag*kFactor/25, "real array / complex array")
    Check(kUniformProduct[kIndex], -2*kReal, -2*kImag, "scalar * complex array")
    Check(kUniformQuotient[kIndex], kReal/-2, kImag/-2, "complex array / scalar")
    Check(kInPlaceProduct[kIndex], kReal*kFactor, kImag*kFactor, "array *= real array")
    Check(kInPlaceQuotient[kIndex], kReal/kFactor, kImag/kFactor, "array /= real array")
    Check(kCanonical[kIndex], kReal*kFactor, kImag*kFactor, "array polar conversion")

    kExpectedPhase = taninv2(kImag*kFactor, kReal*kFactor)
    kAudioMagnitude vaget kIndex, aMagnitudes
    kAudioPhase vaget kIndex, aPhases
    if !(abs(kMagnitudes[kIndex]-10) < .00001 && abs(kAudioMagnitude-10) < .00001 && abs(kPhases[kIndex]-kExpectedPhase) < .00001 && abs(kAudioPhase-kExpectedPhase) < .00001) then
      printks "Array magnitude/phase conversion failed at element %g\n", 0, kIndex
      exitnowk -1
    endif
    Check(kLogs[kIndex], log(10), kExpectedPhase, "array logarithm")
    kIndex += 1
  od
  gkChecks += 1
  kCycle timeinstk
  if kCycle == 3 then
    turnoff
  endif
endin

instr CheckSubtraction
  ; Subtracting a complex value negates its imaginary part in either form.
  kSource:Complex[] = [complex(3,4), polar(complex(3,4))]
  kSeparate:Complex[] = 10-kSource
  kReused:Complex[] = kSource
  kReused = 10-kReused
  Check(kSeparate[0], 7, -4, "scalar - rectangular array")
  Check(kSeparate[1], 7, -4, "scalar - polar array")
  Check(kReused[0], 7, -4, "scalar - reused rectangular array")
  Check(kReused[1], 7, -4, "scalar - reused polar array")
  gkChecks += 1
  kCycle timeinstk
  if kCycle == 3 then
    turnoff
  endif
endin

instr CheckResults
  if i(gkChecks) != 9 then
    prints "Complex scaling checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckScalar" 0 .001
i "CheckArrays" 0 .001
i "CheckSubtraction" 0 .001
i "CheckResults" .002 .001
e
</CsScore>
</CsoundSynthesizer>
