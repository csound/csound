<CsTest>
description = "undersample converts complete audio blocks in both directions"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

opcode CheckInputAndMakeRamp, ia, a
  undersample p4, 4
  aInput xin

  ; The caller sends 0, 1, 2, ... at 32 Hz. Linear conversion retains
  ; one input sample of history, initially zero.
  kSample = 0
  while kSample < ksmps do
    kExpected = max(0, (timeinstk() - 1) * 8 + kSample * 8 / ksmps - 1)
    kActual vaget kSample, aInput
    if abs(kActual - kExpected) > 0.00001 then
      printks "input: factor=%g, cycle=%g, sample=%g, expected=%g, got=%g\n", \
              0, p4, timeinstk(), kSample, kExpected, kActual
      exitnowk -1
    endif
    kSample += 1
  od

  ; Generate an independent ramp at the local sample rate to check xout.
  aPhase phasor 1
  xout ksmps, aPhase * sr
endop

instr Check
  aPhase phasor 1
  iLocalBlock, aOutput CheckInputAndMakeRamp aPhase * sr
  if iLocalBlock != round(ksmps / p4) then
    exitnow -1
  endif
  kSample = 0
  while kSample < ksmps do
    kExpected = max(0, (timeinstk() - 1) * iLocalBlock + \
                      kSample * iLocalBlock / ksmps - 1)
    kActual vaget kSample, aOutput
    if abs(kActual - kExpected) > 0.00001 then
      printks "output: factor=%g, cycle=%g, sample=%g, expected=%g, got=%g\n", \
              0, p4, timeinstk(), kSample, kExpected, kActual
      exitnowk -1
    endif
    kSample += 1
  od
endin
</CsInstruments>
<CsScore>
; Exact division, rounded block size (8 / 3 -> 3), and one local sample.
i "Check" 0 0.5 2
i "Check" 0.5 0.5 3
i "Check" 1 0.5 8
e
</CsScore>
</CsoundSynthesizer>
