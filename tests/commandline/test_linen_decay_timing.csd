<CsTest>
description = "linen preserves its historical nonzero endpoint at both signal rates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkFailures init 0
gkChecks init 0

instr 1
  ; Exact cycle counts avoid any ambiguity about rounding the duration.
  kEnvelope linen 1, 0, p4/kr, p4/kr
  kCycle init 0
  iResidual = .5/(p4 + .5)
  if kCycle == p4 then
    printks "linen control endpoint: length=%g expected=%.9f actual=%.9f\n", 0, p4, iResidual, kEnvelope
    if abs(kEnvelope-iResidual) > .000001 then
      gkFailures += 1
    endif
    gkChecks += 1
  endif
  kCycle += 1
endin

instr 2
  aEnvelope linen 1, 0, p4/sr, p4/sr
  iResidual = .5/(p4 + .5)
  kFirst init 1
  if kFirst == 1 then
    kEndpoint vaget p4, aEnvelope
    printks "linen audio endpoint: length=%g expected=%.9f actual=%.9f\n", 0, p4, iResidual, kEndpoint
    if abs(kEndpoint-iResidual) > .000001 then
      gkFailures += 1
    endif
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkFailures) != 0 || i(gkChecks) != 4 then
    prints "linen decay failures=%g completed=%g\n", i(gkFailures), i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02 4
i 1 .03 .02 8
i 2 .06 .002 4
i 2 .07 .002 8
i 99 .08 .002
e
</CsScore>
</CsoundSynthesizer>
