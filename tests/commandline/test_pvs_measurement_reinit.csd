<CsTest>
description = "pvscent and pvsbandwidth follow a restarted spectral source"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckRestart
  kCycle init 0
  kCycle += 1
  kMagnitudes[] init 33
  kFrequencies[] init 33

  ; With a 64-point FFT, adjacent bins are 128 Hz apart. Two equal
  ; magnitudes give a centroid at the midpoint and a bandwidth equal
  ; to half their frequency difference.
  kLow = (kCycle < 10 || kCycle >= 30 ? 1 : 0)
  kHigh = (kCycle >= 10 && kCycle < 20 ? 1 : 0)
  kMagnitudes[1] = kLow
  kMagnitudes[2] = kLow
  kMagnitudes[8] = kHigh
  kMagnitudes[10] = kHigh
  kBin = 0
  while kBin < 33 do
    kFrequencies[kBin] = 128*kBin
    kBin += 1
  od

  ; Restart only the producer: low bins -> high bins -> silence -> low bins.
  ; Keep all three measurement opcodes running across each restart.
  if kCycle == 10 || kCycle == 20 || kCycle == 30 then
    reinit SOURCE
  endif
SOURCE:
  fSource pvsfromarray kMagnitudes, kFrequencies, p4
  rireturn

  kCentroid pvscent fSource
  aCentroid pvscent fSource
  kBandwidth pvsbandwidth fSource
  kExpectedCentroid = 192*kLow + 1152*kHigh
  kExpectedBandwidth = 64*kLow + 128*kHigh
  if kCentroid != kExpectedCentroid || kBandwidth != kExpectedBandwidth then
    printks "hop %g cycle %g: centroid %g (expected %g), bandwidth %g (expected %g)\n", 0, p4, kCycle, kCentroid, kExpectedCentroid, kBandwidth, kExpectedBandwidth
    exitnowk(-1)
  endif
  kSample = 0
  while kSample < ksmps do
    kAudioCentroid vaget kSample, aCentroid
    if kAudioCentroid != kExpectedCentroid then
      printks "hop %g cycle %g sample %g: audio centroid %g (expected %g)\n", 0, p4, kCycle, kSample, kAudioCentroid, kExpectedCentroid
      exitnowk(-1)
    endif
    kSample += 1
  od
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 120 then
    prints "Not all spectral measurement checks finished\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; One-block, two-block and non-block-aligned hops. Each note runs 40 blocks.
i "CheckRestart" 0 .078125 16
i "CheckRestart" 0 .078125 32
i "CheckRestart" 0 .078125 40
i "CheckCompletion" .1 .01
e
</CsScore>
</CsoundSynthesizer>
