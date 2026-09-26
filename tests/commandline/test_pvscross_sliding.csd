<CsTest>
description = "pvscross mixes sliding amplitudes, preserves source frequencies and clears inactive samples"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
gfLeft pvsinit 64, 1, 64, 1
gfRight pvsinit 64, 1, 64, 1
gfMixed pvsinit 64, 1, 64, 1
gfMaskedLeft pvsinit 64, 1, 64, 1
gfMaskedRight pvsinit 64, 1, 64, 1

instr ProduceSpectra
  ; Shared inputs continue outside the mixing note's active samples.
  aLeft oscili .25, 1024
  aRight oscili .5, 2048
  gfLeft pvsanal aLeft + .2, 64, 1, 64, 1
  gfRight pvsanal aRight + .1, 64, 1, 64, 1
endin

instr MixSpectra
  gfMixed pvscross gfLeft, gfRight, -.5, 2
  ; Gains use their absolute values, including values above one.
  ; These references also apply the mixing note's sample boundaries.
  gfMaskedLeft pvsgain gfLeft, .5
  gfMaskedRight pvsgain gfRight, 2
endin

instr CheckBlock
  ; Inspect every bin and sample, including DC, Nyquist and inactive ends.
  kBin = 0
  while kBin <= 32 do
    aLeftAmp, aLeftFreq pvsbin gfMaskedLeft, kBin
    aRightAmp, aRightFreq pvsbin gfMaskedRight, kBin
    aMixedAmp, aMixedFreq pvsbin gfMixed, kBin
    kSample = 0
    while kSample < ksmps do
      kLeftAmp vaget kSample, aLeftAmp
      kLeftFreq vaget kSample, aLeftFreq
      kRightAmp vaget kSample, aRightAmp
      kMixedAmp vaget kSample, aMixedAmp
      kMixedFreq vaget kSample, aMixedFreq
      kExpectedAmp = kLeftAmp + kRightAmp
      if abs(kMixedAmp - kExpectedAmp) > .000001 || kMixedFreq != kLeftFreq then
        printks "pvscross bin %g sample %g: amplitude %g expected %g; frequency %g expected %g\n", 0, kBin, kSample, kMixedAmp, kExpectedAmp, kMixedFreq, kLeftFreq
        exitnowk(-1)
      endif
      kSample += 1
    od
    kBin += 1
  od
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 6 then
    prints "pvscross sample checks did not finish\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i "ProduceSpectra" 0 .125
; Start at sample 5, finish at sample 11 two blocks later.
i "MixSpectra" .0318603515625 .004638671875
i "CheckBlock" .03125 .005859375
; Reuse the instance for a note contained within one block (samples 5..10).
i "MixSpectra" .0631103515625 .000732421875
i "CheckBlock" .0625 .001953125
; Full blocks must still mix every sample.
i "MixSpectra" .09375 .00390625
i "CheckBlock" .09375 .00390625
i "CheckCompletion" .15 .01
e
</CsScore>
</CsoundSynthesizer>
