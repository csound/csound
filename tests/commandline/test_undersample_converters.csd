<CsTest>
description = "undersample passes audio and control signals with each converter mode"

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

opcode LowerRate, ak, ak
  undersample p4, p5
  aInput, kInput xin
  xout aInput * 2, kInput + 1
endop

instr Check
  aInput = 0.25
  kInput = timeinstk()
  aOutput, kOutput LowerRate aInput, kInput
  ; Control rate is unchanged, including with the sinc converter modes.
  if kOutput != kInput + 1 then
    printks "mode=%g: control input=%g, output=%g\n", 0, p5, kInput, kOutput
    exitnowk -1
  endif
  ; Allow the sinc filters to settle, then check every returned sample.
  if timeinsts() > 0.1 then
    kSample = 0
    while kSample < ksmps do
      kActual vaget kSample, aOutput
      if abs(kActual - 0.5) > 0.0001 then
        printks "factor=%g, mode=%g: expected 0.5, got %g at sample %g\n", \
                0, p4, p5, kActual, kSample
        exitnowk -1
      endif
      kSample += 1
    od
  endif
endin
</CsInstruments>
<CsScore>
; Each mode with exact and rounded local block sizes.
i "Check" 0 0.2 2 0
i "Check" + . 3 0
i "Check" + . 2 1
i "Check" + . 3 1
i "Check" + . 2 2
i "Check" + . 3 2
i "Check" + . 2 3
i "Check" + . 3 3
i "Check" + . 2 4
i "Check" + . 3 4
e
</CsScore>
</CsoundSynthesizer>
