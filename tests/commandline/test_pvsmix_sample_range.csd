<CsTest>
description = "pvsmix selects each sliding bin over the note's active samples and clears both inactive ends"

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
gfFirst pvsinit 64, 16, 64, 1
gfSecond pvsinit 64, 16, 64, 1
gfMixed pvsinit 64, 16, 64, 1
gfMaskedFirst pvsinit 64, 16, 64, 1
gfMaskedSecond pvsinit 64, 16, 64, 1

instr 10
 ; Shared inputs keep running outside the mixing note's active samples.
 aFirst oscili .25, 1024
 aSecond oscili .5, 2048
 gfFirst pvsanal aFirst+.2, 64, 1, 64, 1
 gfSecond pvsanal aSecond+.1, 64, 1, 64, 1
endin

instr 11
 gfMixed pvsmix gfFirst, gfSecond
 ; Independent references apply the same note boundaries to each input.
 gfMaskedFirst pvsgain gfFirst, 1
 gfMaskedSecond pvsgain gfSecond, 1
endin

instr 12
 ; Observe complete blocks, including samples outside instrument 11's note.
 kBin = 0
 while kBin <= 32 do
  aFirstAmp, aFirstFreq pvsbin gfMaskedFirst, kBin
  aSecondAmp, aSecondFreq pvsbin gfMaskedSecond, kBin
  aMixedAmp, aMixedFreq pvsbin gfMixed, kBin
  kSample = 0
  while kSample < ksmps do
   kFirstAmp vaget kSample, aFirstAmp
   kFirstFreq vaget kSample, aFirstFreq
   kSecondAmp vaget kSample, aSecondAmp
   kSecondFreq vaget kSample, aSecondFreq
   kMixedAmp vaget kSample, aMixedAmp
   kMixedFreq vaget kSample, aMixedFreq
   ; The louder input supplies both amplitude and frequency for this bin.
   ; Outside the active range both reference spectra are zero.
   kExpectedAmp = kFirstAmp >= kSecondAmp ? kFirstAmp : kSecondAmp
   kExpectedFreq = kFirstAmp >= kSecondAmp ? kFirstFreq : kSecondFreq
   if kMixedAmp != kExpectedAmp || kMixedFreq != kExpectedFreq then
    printks "pvsmix bin %g sample %g: amplitude %g expected %g; frequency %g expected %g\n", 0, kBin, kSample, kMixedAmp, kExpectedAmp, kMixedFreq, kExpectedFreq
    exitnowk(-1)
   endif
   kSample += 1
  od
  kBin += 1
 od
 gkChecks += 1
endin

instr 99
 if i(gkChecks) != 6 then
  prints "pvsmix sample range checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 10 0 .125
; Start at sample 5, finish at sample 11 two blocks later.
i 11 .0318603515625 .004638671875
i 12 .03125 .005859375
; Reuse the instance for a note contained within one block (samples 5..10).
i 11 .0631103515625 .000732421875
i 12 .0625 .001953125
; Full blocks must still mix every sample.
i 11 .09375 .00390625
i 12 .09375 .00390625
i 99 .15 .01
e
</CsScore>
</CsoundSynthesizer>
