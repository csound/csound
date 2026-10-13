<CsTest>
description = "phaser2 preserves allpass gain at low frequencies in float builds"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckConstantInput
  iOrder = p4
  iMode = p5
  iFeedback = p6
  aInput = 1
  ; For a constant input, each allpass stage has gain 1. Feedback changes
  ; the steady output to 1/(1-feedback). Sixteen seconds lets the 1 Hz
  ; filter settle, including the cases with several stages and feedback.
  aOutput phaser2 aInput, 1, .5, iOrder, iMode, 2, iFeedback
  kBlock init 0
  if kBlock == 16*kr then
    kOutput downsamp aOutput
    iExpected = 1/(1-iFeedback)
    if !(abs(kOutput-iExpected) < .00001) then
      printks "phaser2 stages=%g mode=%g feedback=%g: expected %.12g, got %.12g\n", 0, iOrder, iMode, iFeedback, iExpected, kOutput
      exitnowk -1
    endif
    gkChecks += 1
    turnoff
  endif
  kBlock += 1
endin

instr CheckCompletion
  if i(gkChecks) != 6 then
    prints "All phaser2 gain checks must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Stage count, spacing mode, feedback.
i "CheckConstantInput" 0 16.1 1 1 0
i "CheckConstantInput" 0 16.1 4 1 0
i "CheckConstantInput" 0 16.1 4 2 0
i "CheckConstantInput" 0 16.1 1 1 .5
i "CheckConstantInput" 0 16.1 4 1 -.5
i "CheckConstantInput" 0 16.1 4 2 .5
i "CheckCompletion" 16.2 .001
e
</CsScore>
</CsoundSynthesizer>
