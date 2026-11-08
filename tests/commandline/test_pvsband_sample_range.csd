<CsTest>
description = "pvsband filters process Nyquist and clear inactive sliding samples"

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
gfRejected pvsinit 64, 16, 64, 1
gfPassed pvsinit 64, 16, 64, 1
gfReference pvsinit 64, 16, 64, 1

instr 10
 ; Shared inputs keep running outside the filtering note's active samples.
 aFirst oscili .25, 1024
 aNyquist oscili .125, sr/2, -1, .25
 gfFirst pvsanal aFirst+aNyquist+.2, 64, 1, 64, 1
endin

instr 11
 ; Keep the whole analysis, including near-silent bins whose estimated
 ; frequencies can lie just beyond Nyquist.
 gfPassed pvsbandp gfFirst, 0, 0, sr, sr
 gfRejected pvsbandr gfFirst, 2*sr, 2*sr, 3*sr, 3*sr
 gfReference pvsgain gfFirst, 1
endin

instr 12
 ; Observe complete blocks, including samples outside instrument 11's note.
 kBin = 0
 while kBin <= 32 do
  aReferenceAmp, aReferenceFreq pvsbin gfReference, kBin
  aRejectAmp, aRejectFreq pvsbin gfRejected, kBin
  aPassAmp, aPassFreq pvsbin gfPassed, kBin
  kSample = 0
  while kSample < ksmps do
   kReferenceAmp vaget kSample, aReferenceAmp
   kReferenceFreq vaget kSample, aReferenceFreq
   kRejectAmp vaget kSample, aRejectAmp
   kRejectFreq vaget kSample, aRejectFreq
   kPassAmp vaget kSample, aPassAmp
   kPassFreq vaget kSample, aPassFreq
   ; Both filters retain the source inside the note and clear it outside.
   if kPassAmp != kReferenceAmp || kPassFreq != kReferenceFreq || kRejectAmp != kReferenceAmp || kRejectFreq != kReferenceFreq then
    printks "pvsband bin %g sample %g: pass=%g reject=%g expected=%g; frequencies %g/%g expected=%g\n", 0, kBin, kSample, kPassAmp, kRejectAmp, kReferenceAmp, kPassFreq, kRejectFreq, kReferenceFreq
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
  prints "pvsband sample range checks did not finish\n"
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
; Full blocks must still filter every sample.
i 11 .09375 .00390625
i 12 .09375 .00390625
i 99 .15 .01
e
</CsScore>
</CsoundSynthesizer>
