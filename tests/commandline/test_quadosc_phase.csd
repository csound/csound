<CsTest>
description = "quadosc preserves complex values and tracks k-rate and audio-rate frequency changes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Check, 0, kkkS
  kReal, kImag, kPhase, SMode xin
  kExpectedReal = cos(kPhase)
  kExpectedImag = sin(kPhase)
  if !(abs(kReal-kExpectedReal) < .00001 && abs(kImag-kExpectedImag) < .00001) then
    printks "%s: expected (%g,%g), got (%g,%g)\n", \
      0, SMode, kExpectedReal, kExpectedImag, kReal, kImag
    exitnowk -1
  endif
endop

instr CheckControlFrequency
  kCycle timeinstk
  ; Change frequency twice, stop, then restart. Cross several full cycles.
  kFrequency init sr/16
  if kCycle == 2 then
    kFrequency = sr/8
  elseif kCycle == 3 then
    kFrequency = 0
  elseif kCycle == 4 then
    kFrequency = sr/32
  endif
  aFrequency = kFrequency
  kRect:Complex[] quadosc kFrequency, 0
  kPolar:Complex[] quadosc kFrequency, 1
  kAudioRect:Complex[] quadosc aFrequency, 0
  kAudioPolar:Complex[] quadosc aFrequency, 1
  kCycles init 0
  kIndex = 0
  while kIndex < ksmps do
    ; quadosc advances before emitting its first sample.
    kCycles += kFrequency/sr
    kCycles -= floor(kCycles)
    kPhase = 2*$M_PI*kCycles
    Check real(kRect[kIndex]), imag(kRect[kIndex]), kPhase, "k-rate, rectangular"
    Check real(kPolar[kIndex]), imag(kPolar[kIndex]), kPhase, "k-rate, polar"
    Check real(kAudioRect[kIndex]), imag(kAudioRect[kIndex]), kPhase, "audio-rate, rectangular"
    Check real(kAudioPolar[kIndex]), imag(kAudioPolar[kIndex]), kPhase, "audio-rate, polar"
    kIndex += 1
  od
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr CheckAudioFrequency
  ; The frequency changes within each block, not just at its boundary.
  aFrequency linseg sr/32, 64/sr, sr/8
  kRect:Complex[] quadosc aFrequency, 0
  kPolar:Complex[] quadosc aFrequency, 1
  kCycles init 0
  kIndex = 0
  while kIndex < ksmps do
    kFrequency vaget kIndex, aFrequency
    kCycles += kFrequency/sr
    kCycles -= floor(kCycles)
    kPhase = 2*$M_PI*kCycles
    Check real(kRect[kIndex]), imag(kRect[kIndex]), kPhase, "varying audio frequency, rectangular"
    Check real(kPolar[kIndex]), imag(kPolar[kIndex]), kPhase, "varying audio frequency, polar"
    kIndex += 1
  od
  kCycle timeinstk
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr CheckPolarWrapping
  ; Reverse rotation, then an increment larger than one whole cycle.
  kFrequency init -sr/16
  kCycle timeinstk
  if kCycle == 2 then
    kFrequency = sr*1.25
  elseif kCycle == 3 then
    kFrequency = 0
  elseif kCycle == 4 then
    kFrequency = sr/32
  endif
  aFrequency = kFrequency
  kPolar:Complex[] quadosc kFrequency, 1
  kAudioPolar:Complex[] quadosc aFrequency, 1
  kCycles init 0
  kIndex = 0
  while kIndex < ksmps do
    kCycles += kFrequency/sr
    kCycles -= floor(kCycles)
    kPhase = 2*$M_PI*kCycles
    Check real(kPolar[kIndex]), imag(kPolar[kIndex]), kPhase, "polar phase wrapping, k-rate"
    Check real(kAudioPolar[kIndex]), imag(kAudioPolar[kIndex]), kPhase, "polar phase wrapping, audio-rate"
    kIndex += 1
  od
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr CheckResults
  if i(gkChecks) != 3 then
    prints "quadosc phase checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckControlFrequency" 0 [64/32768]
i "CheckAudioFrequency" 0 [64/32768]
i "CheckPolarWrapping" 0 [64/32768]
i "CheckResults" [80/32768] [16/32768]
e
</CsScore>
</CsoundSynthesizer>
