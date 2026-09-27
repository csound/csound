<CsTest>
description = "pvsmaska clears inactive sliding samples and preserves active amplitudes and frequencies"

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
giMask ftgen 0, 0, 64, -7, .5, 64, .5
gfSource pvsinit 64, 1, 64, 1
gfMasked pvsinit 64, 1, 64, 1
gfReference pvsinit 64, 1, 64, 1

instr ProduceSpectrum
  ; The source keeps running outside the masking note's active samples.
  aInput oscili .25, 1024
  gfSource pvsanal aInput + .2, 64, 1, 64, 1
endin

instr ApplyMask
  ; A mask of .5 at depth .5 gives gain (1 - .5) + .5*.5 = .75.
  gfMasked pvsmaska gfSource, giMask, .5
  gfReference pvsgain gfSource, .75
endin

instr CheckBlock
  ; Check every bin, including DC and Nyquist, over the complete block.
  kBin = 0
  while kBin <= 32 do
    aMaskedAmp, aMaskedFreq pvsbin gfMasked, kBin
    aExpectedAmp, aExpectedFreq pvsbin gfReference, kBin
    kSample = 0
    while kSample < ksmps do
      kMaskedAmp vaget kSample, aMaskedAmp
      kMaskedFreq vaget kSample, aMaskedFreq
      kExpectedAmp vaget kSample, aExpectedAmp
      kExpectedFreq vaget kSample, aExpectedFreq
      if kMaskedAmp != kExpectedAmp || kMaskedFreq != kExpectedFreq then
        printks "pvsmaska bin %g sample %g: amplitude %g expected %g; frequency %g expected %g\n", 0, kBin, kSample, kMaskedAmp, kExpectedAmp, kMaskedFreq, kExpectedFreq
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
    prints "pvsmaska sample checks did not finish\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i "ProduceSpectrum" 0 .125
; Start at sample 5, finish at sample 11 two blocks later.
i "ApplyMask" .0318603515625 .004638671875
i "CheckBlock" .03125 .005859375
; Reuse the instance for a note contained within one block (samples 5..10).
i "ApplyMask" .0631103515625 .000732421875
i "CheckBlock" .0625 .001953125
; Full blocks must still process every sample.
i "ApplyMask" .09375 .00390625
i "CheckBlock" .09375 .00390625
i "CheckCompletion" .15 .01
e
</CsScore>
</CsoundSynthesizer>
