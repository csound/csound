<CsTest>
description = "pvscale applies sliding gain to DC and clears inactive samples"

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
gfScaled pvsinit 64, 16, 64, 1

instr 1
 ; Include DC, which needs the gain even though its frequency stays zero.
 aTone oscili .25, 1024
 aInput = .1+aTone
 fInput pvsanal aInput, 64, 1, 64, 1
 if p5 == 0 then
  fUnit pvscale fInput, 2, 0, 1
  fActual pvscale fInput, 2, 0, p4
 else
  aRamp phasor 128
  aRatio = .5+aRamp
  fUnit pvscale fInput, aRatio, 0, 1
  fActual pvscale fInput, aRatio, 0, p4
 endif
 fExpected pvsgain fUnit, p4
 aExpected pvsynth fExpected
 aActual pvsynth fActual
 kError max_k abs(aExpected-aActual), 1, 1
 if !(kError < .00001) then
  printks "pvscale sliding gain %g, audio control %g: error %g\n", 0, p4, p5, kError
  exitnowk(-1)
 endif
 kCycle init 0
 kCycle += 1
 if kCycle == 40 then
  gkChecks += 1
  turnoff
 endif
endin

; Keep the input running before and after the scaling note.
instr 10
 aInput oscili .25, 1024
 gfSource pvsanal aInput, 64, 1, 64, 1
endin

instr 11
 gfScaled pvscale gfSource, 2
endin

instr 12
 ; Read a sample outside instrument 11's active range in the same block.
 kCentroid pvscent gfScaled
 if kCentroid != 0 then
  printks "pvscale left a spectrum outside the active samples: %g\n", 0, kCentroid
  exitnowk(-1)
 endif
 gkChecks += 1
 turnoff
endin

instr 99
 if i(gkChecks) != 8 then
  prints "pvscale sliding checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Gain, use an audio-rate pitch ratio.
i 1 0 .125 0 0
i 1 0 .125 .5 0
i 1 0 .125 2 0
i 1 0 .125 0 1
i 1 0 .125 .5 1
i 1 0 .125 2 1
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
