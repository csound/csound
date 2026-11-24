<CsTest>
description = "phaser2 preserves allpass gain near DC and Nyquist in float builds"
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
  if p9 != 0 then
    aInput oscili 1, sr/2, -1, .25
  else
    aInput = 1
  endif
  ; For a constant input, each allpass stage has gain 1. Feedback changes
  ; the DC output to 1/(1-feedback), and the Nyquist output to 1/(1+feedback).
  ; High-Q feedback cases need longer to settle.
  aOutput phaser2 aInput, p7, p8, iOrder, iMode, 2, iFeedback
  kBlock init 0
  iSettling = (p8 > 1000 ? 48 : 16)
  if kBlock == iSettling*kr then
    iGain = 1/(1-(p9 != 0 ? -iFeedback : iFeedback))
    kN = 0
    while kN < ksmps do
      kOutput vaget kN, aOutput
      kInput vaget kN, aInput
      kExpected = iGain*kInput
      if !(abs(kOutput-kExpected) < .00001) then
        printks "phaser2 stages=%g mode=%g feedback=%g: expected %.12g, got %.12g\n", 0, iOrder, iMode, iFeedback, kExpected, kOutput
        exitnowk -1
      endif
      kN += 1
    od
    gkChecks += 1
    turnoff
  endif
  kBlock += 1
endin

instr CheckCompletion
  if i(gkChecks) != 14 then
    prints "All phaser2 gain checks must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Stage count, spacing mode, feedback, frequency, Q, alternating input.
i "CheckConstantInput" 0 16.1 1 1 0 1 .5 0
i "CheckConstantInput" 0 16.1 4 1 0 1 .5 0
i "CheckConstantInput" 0 16.1 4 2 0 1 .5 0
i "CheckConstantInput" 0 16.1 1 1 .5 1 .5 0
i "CheckConstantInput" 0 16.1 4 1 -.5 1 .5 0
i "CheckConstantInput" 0 16.1 4 2 .5 1 .5 0
i "CheckConstantInput" 0 48.1 1 1 0 23999 48000 1
i "CheckConstantInput" 0 48.1 4 1 0 23999 48000 1
i "CheckConstantInput" 0 48.1 4 2 0 23999 48000 1
i "CheckConstantInput" 0 48.1 1 1 .5 23999 48000 1
i "CheckConstantInput" 0 48.1 4 1 -.5 23999 48000 1
i "CheckConstantInput" 0 48.1 4 2 .5 23999 48000 1
i "CheckConstantInput" 0 16.1 1 1 .5 0 .5 0
i "CheckConstantInput" 0 16.1 4 2 .5 0 .5 1
i "CheckCompletion" 48.2 .001
e
</CsScore>
</CsoundSynthesizer>
