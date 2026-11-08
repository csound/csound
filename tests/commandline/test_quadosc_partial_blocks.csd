<CsTest>
description = "quadosc advances only through active samples and clears inactive complex outputs"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckActiveSamples
  iStart = round(p2*sr)
  iEnd = iStart + round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  kFrequency init sr/16
  aFrequency = kFrequency
  kRect:Complex[] quadosc kFrequency, 0
  kPolar:Complex[] quadosc kFrequency, 1
  kAudioRect:Complex[] quadosc aFrequency, 0
  kAudioPolar:Complex[] quadosc aFrequency, 1

  kIndex = 0
  while kIndex < ksmps do
    kSample = kBlockStart+kIndex
    if kSample >= iStart && kSample < iEnd then
      ; Only active samples advance the oscillator, starting with one step.
      kPhase = 2*$M_PI*(kSample-iStart+1)/16
      kExpectedReal = cos(kPhase)
      kExpectedImag = sin(kPhase)
    else
      kExpectedReal = 0
      kExpectedImag = 0
    endif
    kReal[] = [real(kRect[kIndex]), real(kPolar[kIndex]), real(kAudioRect[kIndex]), real(kAudioPolar[kIndex])]
    kImag[] = [imag(kRect[kIndex]), imag(kPolar[kIndex]), imag(kAudioRect[kIndex]), imag(kAudioPolar[kIndex])]
    kMode = 0
    while kMode < 4 do
      if !(abs(kReal[kMode]-kExpectedReal) < .00001 && abs(kImag[kMode]-kExpectedImag) < .00001) then
        printks "quadosc start=%g sample=%g mode=%g expected=(%g,%g) actual=(%g,%g)\n", \
          0, iStart, kSample, kMode, kExpectedReal, kExpectedImag, kReal[kMode], kImag[kMode]
        exitnowk -1
      endif
      kMode += 1
    od
    kIndex += 1
  od
  if kBlockStart < iEnd && kBlockStart+ksmps >= iEnd then
    gkChecks += 1
  endif
  kBlockStart += ksmps
endin

instr CheckResults
  if i(gkChecks) != 3 then
    prints "quadosc partial-block checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Ordinary blocks, then a delayed start and early end, then a short note.
i "CheckActiveSamples" 0 [64/32768]
i "CheckActiveSamples" [83/32768] [60/32768]
i "CheckActiveSamples" [163/32768] [5/32768]
i "CheckResults" [192/32768] [16/32768]
e
</CsScore>
</CsoundSynthesizer>
