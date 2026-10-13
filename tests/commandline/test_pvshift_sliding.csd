<CsTest>
description = "pvshift applies sliding gain below the cutoff and clears inactive samples"

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
gfSource pvsinit 64, 16, 64, 1
gfShifted pvsinit 64, 16, 64, 1

instr 1
 ; DC and the 1024 Hz tone are below the cutoff, but still need the gain.
 aTone oscili .25, 1024
 aInput = .1+aTone
 fInput pvsanal aInput, 64, 1, 64, 1
 fUnit pvshift fInput, 128, 2048, 0, 1
 fActual pvshift fInput, 128, 2048, 0, p4
 fExpected pvsgain fUnit, p4
 aExpected pvsynth fExpected
 aActual pvsynth fActual
 kError max_k abs(aExpected-aActual), 1, 1
 if !(kError < .00001) then
  printks "pvshift sliding gain %g: error %g\n", 0, p4, kError
  exitnowk(-1)
 endif
 kCycle init 0
 kCycle += 1
 if kCycle == 40 then
  gkChecks += 1
  turnoff
 endif
endin

; Keep the input running before and after the shifting note.
instr 10
 aInput oscili .25, 1024
 gfSource pvsanal aInput, 64, 1, 64, 1
endin

instr 11
 gfShifted pvshift gfSource, 128, 0
endin

instr 12
 ; Read a sample outside instrument 11's active range in the same block.
 kCentroid pvscent gfShifted
 if kCentroid != 0 then
  printks "pvshift left a spectrum outside the active samples: %g\n", 0, kCentroid
  exitnowk(-1)
 endif
 gkChecks += 1
 turnoff
endin

instr 99
 if i(gkChecks) != 5 then
  prints "pvshift sliding checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Gains: mute, attenuate, amplify.
i 1 0 .125 0
i 1 0 .125 .5
i 1 0 .125 2
i 10 .75 .125
; Start at sample 5 and finish at sample 11 of a later block.
i 11 .7818603515625 .004638671875
; Inspect samples 3 and 11 of the first and last blocks, respectively.
i 12 .7816162109375 .0001220703125
i 12 .7864990234375 .0001220703125
i 99 1 .01
e
</CsScore>
</CsoundSynthesizer>
