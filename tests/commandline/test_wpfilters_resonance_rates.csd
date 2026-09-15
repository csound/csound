<CsTest>
description = "K35 and ZDF ladder resonance limits agree across input rates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode CheckSame, 0, aa
  aActual, aExpected xin
  kError max_k abs(aActual - aExpected), 1, 1
  if !(kError < .0000001) then
    printks "resonance clamp mismatch: %g\n", 0, kError
    exitnowk -1
  endif
endop

instr 1
  aInput oscili .01, 220
  iLimited limit p4, 1, 10
  aScalar k35lpf aInput, 1000, p4, p5
  aScalarLimit k35lpf aInput, 1000, iLimited, p5
  CheckSame aScalar, aScalarLimit

  kCycle timeinstk
  ; Exercise the coefficient cache when Q crosses both limits and returns.
  kQ = (kCycle < 10 ? p4 : (kCycle < 20 ? 5 : (kCycle < 30 ? -2 : 12)))
  kLimited limit kQ, 1, 10
  aQ = kQ
  aControl k35lpf aInput, 1000, kQ, p5
  aAudio k35lpf aInput, 1000, aQ, p5
  aLimit k35lpf aInput, 1000, kLimited, p5
  CheckSame aControl, aLimit
  CheckSame aAudio, aLimit
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  aInput oscili .01, 220
  iLimited limit p4, 1, 10
  aScalar k35hpf aInput, 1000, p4, p5
  aScalarLimit k35hpf aInput, 1000, iLimited, p5
  CheckSame aScalar, aScalarLimit

  kCycle timeinstk
  ; Exercise the coefficient cache when Q crosses both limits and returns.
  kQ = (kCycle < 10 ? p4 : (kCycle < 20 ? 5 : (kCycle < 30 ? -2 : 12)))
  kLimited limit kQ, 1, 10
  aQ = kQ
  aControl k35hpf aInput, 1000, kQ, p5
  aAudio k35hpf aInput, 1000, aQ, p5
  aLimit k35hpf aInput, 1000, kLimited, p5
  CheckSame aControl, aLimit
  CheckSame aAudio, aLimit
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  aInput oscili .01, 220
  iLimited limit p4, 0.5, 25
  aScalar zdfladder aInput, 1000, p4
  aScalarLimit zdfladder aInput, 1000, iLimited
  CheckSame aScalar, aScalarLimit

  kCycle timeinstk
  ; Exercise the coefficient cache when Q crosses both limits and returns.
  kQ = (kCycle < 10 ? p4 : (kCycle < 20 ? 5 : (kCycle < 30 ? -2 : 27)))
  kLimited limit kQ, 0.5, 25
  aQ = kQ
  aControl zdfladder aInput, 1000, kQ
  aAudio zdfladder aInput, 1000, aQ
  aLimit zdfladder aInput, 1000, kLimited
  CheckSame aControl, aLimit
  CheckSame aAudio, aLimit
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 15 then
    prints "resonance clamp checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0.00000 .03 -2 0
i 1 0.00000 .03 5 0
i 1 0.00000 .03 27 0
i 1 0.04000 .03 -2 1
i 1 0.04000 .03 5 1
i 1 0.04000 .03 27 1
i 2 0.10000 .03 -2 0
i 2 0.10000 .03 5 0
i 2 0.10000 .03 27 0
i 2 0.14000 .03 -2 1
i 2 0.14000 .03 5 1
i 2 0.14000 .03 27 1
i 3 0.20000 .03 -2 0
i 3 0.20000 .03 5 0
i 3 0.20000 .03 27 0
i 99 .3 .001
</CsScore>
</CsoundSynthesizer>
