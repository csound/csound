<CsTest>
description = "Spectrum extraction preserves reused inputs, endpoint signs and current lengths"

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
gkChecks init 0

opcode Check, 0, kk
  kActual, kExpected xin
  if abs(kActual - kExpected) > 1e-10 then
    printks "spectrum extraction: expected %g, got %g\n", 0, kExpected, kActual
    exitnowk -1
  endif
endop

instr 1
  kInput[] init 8
  kMagReuse[] init 8
  kPowReuse[] init 8
  kPhsReuse[] init 8
  kCycle timeinstk
  kN = (kCycle == 2 ? 4 : (kCycle == 3 ? 2 : 8))
  kSign = (kCycle % 2 == 0 ? 1 : -1)
  ; Restore the full packed spectrum before each in-place extraction.
  trim kInput, kN
  trim kMagReuse, kN
  trim kPowReuse, kN
  trim kPhsReuse, kN
  kJ = 0
  while kJ < kN do
    kValue = (kJ == 0 ? 3*kSign : (kJ == 1 ? -2*kSign : kJ-3))
    kInput[kJ] = kValue
    kMagReuse[kJ] = kValue
    kPowReuse[kJ] = kValue
    kPhsReuse[kJ] = kValue
    kJ += 1
  od

  kMag[] mags kInput
  kPow[] pows kInput
  kPhase[] phs kInput
  kMagReuse mags kMagReuse
  kPowReuse pows kPowReuse
  kPhsReuse phs kPhsReuse
  kMagSize lenarray kMag
  kPowSize lenarray kPow
  kPhaseSize lenarray kPhase
  kMagReuseSize lenarray kMagReuse
  kPowReuseSize lenarray kPowReuse
  kPhsReuseSize lenarray kPhsReuse
  Check kMagSize, kN/2+1
  Check kPowSize, kMagSize
  Check kPhaseSize, kMagSize
  Check kMagReuseSize, kMagSize
  Check kPowReuseSize, kMagSize
  Check kPhsReuseSize, kMagSize
  kJ = 0
  while kJ < kMagSize do
    if kJ == 0 then
      kReal = kInput[0]
      kImag = 0
    elseif kJ == kMagSize-1 then
      kReal = kInput[1]
      kImag = 0
    else
      kReal = kInput[2*kJ]
      kImag = kInput[2*kJ+1]
    endif
    kPower = kReal*kReal + kImag*kImag
    Check kPow[kJ], kPower
    Check kMag[kJ], sqrt(kPower)
    Check kMagReuse[kJ], kMag[kJ]
    Check kPowReuse[kJ], kPow[kJ]
    Check kPhsReuse[kJ], kPhase[kJ]
    ; Magnitude and phase must reconstruct both components, including DC/Nyquist.
    Check kMag[kJ]*cos(kPhase[kJ]), kReal
    Check kMag[kJ]*sin(kPhase[kJ]), kImag
    kJ += 1
  od
  gkChecks += 1
  if kCycle == 4 then
    turnoff
  endif
endin

instr 2
  if i(gkChecks) != 4 then
    prints "spectrum extraction checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .1
i2 .2 .01
</CsScore>
</CsoundSynthesizer>
