<CsTest>
description = "Spectrum conversions preserve endpoints, input reuse and current lengths"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
gkChecks init 0

opcode Check, 0, kk
  kActual, kExpected xin
  if !(abs(kActual-kExpected) < 1e-10) then
    printks "spectrum conversion: expected %.12g, got %.12g\n", 0, kExpected, kActual
    exitnowk -1
  endif
endop

opcode CheckRoundTrip, 0, kk
  kActual, kExpected xin
  ; Magnitude and phase rounding limits the reconstructed interior bins.
  iFloat = (16777217 == 16777216 ? 1 : 0)
  kTolerance = (iFloat == 1 ? 1e-6*max(1, abs(kExpected)) : 1e-10)
  if !(abs(kActual-kExpected) < kTolerance) then
    printks "spectrum round trip: expected %.12g, got %.12g\n", 0, kExpected, kActual
    exitnowk -1
  endif
endop

instr 1
  kInput[] init 8
  kReuse[] init 8
  kMags[] init 5
  kPhases[] init 5
  kMagReuse[] init 5
  kPhaseReuse[] init 5
  kBoth[] init 5
  kCycle timeinstk
  kN = (kCycle == 2 ? 4 : (kCycle == 3 ? 2 : 8))
  kBins = kN/2+1
  trim kInput, kN
  trim kReuse, kN
  ; Refill full inputs before each conversion that reuses their storage.
  trim kMags, kBins
  trim kPhases, kBins
  trim kMagReuse, kBins
  trim kPhaseReuse, kBins
  trim kBoth, kBins
  kJ = 0
  while kJ < kN do
    kInput[kJ] = (kCycle % 2 == 0 ? 1 : -1)*(kJ+1)
    kReuse[kJ] = kInput[kJ]
    kJ += 1
  od
  kPolar[] rect2pol kInput
  kReuse rect2pol kReuse
  Check kPolar[0], kInput[0]
  Check kPolar[1], kInput[1]
  kJ = 0
  while kJ < kN do
    Check kReuse[kJ], kPolar[kJ]
    kJ += 1
  od
  kRect[] pol2rect kPolar
  kReuse pol2rect kReuse
  kLength lenarray kRect
  Check kLength, kN
  kJ = 0
  while kJ < kN do
    if kJ < 2 then
      Check kRect[kJ], kInput[kJ]
    else
      CheckRoundTrip kRect[kJ], kInput[kJ]
    endif
    ; Reusing input storage must produce the same result as a separate output.
    Check kReuse[kJ], kRect[kJ]
    kJ += 1
  od

  kJ = 0
  while kJ < kBins do
    kMags[kJ] = kJ+1
    kPhases[kJ] = kJ*.25+kCycle
    kMagReuse[kJ] = kMags[kJ]
    kPhaseReuse[kJ] = kPhases[kJ]
    kBoth[kJ] = kMags[kJ]
    kJ += 1
  od
  kCombined[] pol2rect kMags, kPhases
  kMagReuse pol2rect kMagReuse, kPhases
  kPhaseReuse pol2rect kMags, kPhaseReuse
  kBoth pol2rect kBoth, kBoth
  kCombinedLength lenarray kCombined
  kMagLength lenarray kMagReuse
  kPhaseLength lenarray kPhaseReuse
  kBothLength lenarray kBoth
  Check kCombinedLength, kN
  Check kMagLength, kN
  Check kPhaseLength, kN
  Check kBothLength, kN
  kJ = 0
  while kJ < kN do
    if kJ < 2 then
      kBin = (kJ == 0 ? 0 : kBins-1)
      kExpected = kMags[kBin]*cos(kPhases[kBin])
      kBothExpected = kMags[kBin]*cos(kMags[kBin])
    else
      kBin = int(kJ/2)
      kExpected = kMags[kBin]*(kJ % 2 == 0 ? cos(kPhases[kBin]) : sin(kPhases[kBin]))
      kBothExpected = kMags[kBin]*(kJ % 2 == 0 ? cos(kMags[kBin]) : sin(kMags[kBin]))
    endif
    Check kCombined[kJ], kExpected
    Check kMagReuse[kJ], kExpected
    Check kPhaseReuse[kJ], kExpected
    Check kBoth[kJ], kBothExpected
    kJ += 1
  od
  gkChecks += 1
  if kCycle == 4 then
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 4 then
    prints "spectrum conversion checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i99 .02 .001
</CsScore>
</CsoundSynthesizer>
