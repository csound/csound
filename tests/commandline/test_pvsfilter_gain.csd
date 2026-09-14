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
gfFiltered pvsinit 64, 16, 64, 1

; Applying the optional gain must equal a separate pvsgain after filtering.
instr 1, 2
  kCycle init 0
  aIn oscili .25, 1024
  aFilter oscili .5, 1024
  fIn pvsanal aIn, 64, p6, 64, 1
  fFilter pvsanal aFilter, 64, p6, 64, 1
  if p1 == 1 then
    fUnit pvsfilter fIn, fFilter, p4, 1
    fActual pvsfilter fIn, fFilter, p4, p5
  else
    aRamp phasor 128
    aDepth = p4+aRamp
    fUnit pvsfilter fIn, fFilter, aDepth, 1
    fActual pvsfilter fIn, fFilter, aDepth, p5
  endif
  fExpected pvsgain fUnit, p5
  aExpected pvsynth fExpected
  aActual pvsynth fActual
  aError = abs(aExpected-aActual)
  kError max_k aError, 1, 1
  if !(kError < .00001) then
    printks "pvsfilter depth %g gain %g hop %g: error %g\n", 0, p4, p5, p6, kError
    exitnowk -1
  endif
  kCycle += 1
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

; A shared input remains active beyond the filtering note's sample range.
instr 10
  aIn oscili .25, 1024
  gfSource pvsanal aIn, 64, 1, 64, 1
endin

instr 11
  gfFiltered pvsfilter gfSource, gfSource, 0, 1
endin

instr 12
  kCentroid pvscent gfFiltered
  if kCentroid != 0 then
    printks "pvsfilter left a spectrum outside the active samples: %g\n", 0, kCentroid
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 99
  if i(gkChecks) != 21 then
    prints "pvsfilter checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Depth, gain, hop. Hop 1 selects sliding DFT; hop 16 uses FFT frames.
i 1 0 .125 1 2 1
i 1 0 .125 .25 2 1
i 1 0 .125 1 .5 1
i 1 0 .125 1 0 1
i 1 0 .125 1 -1 1
i 1 0 .125 1 1 1
i 1 0 .125 0 2 1
i 1 0 .125 -1 2 1
i 1 0 .125 2 2 1
i 1 0 .125 1 2 16
i 1 0 .125 .25 2 16
i 1 0 .125 1 .5 16
i 1 0 .125 1 0 16
i 2 .25 .125 0 2 1
i 2 .25 .125 -.5 .5 1
i 2 .25 .125 .5 2 1
i 2 .25 .125 0 0 1
; Reuse note instances with different gains.
i 1 .5 .125 1 .5 1
i 2 .5 .125 0 .5 1
i 10 .75 .125
; Start at sample 5, finish at sample 11 of a later block.
i 11 .7818603515625 .004638671875
i 12 .7816162109375 .0001220703125
i 12 .7864990234375 .0001220703125
i 99 1 .01
e
</CsScore>
</CsoundSynthesizer>
