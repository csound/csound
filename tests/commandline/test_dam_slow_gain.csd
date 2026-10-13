<CsTest>
description = "dam preserves slow gain rises, falls, and direction changes"
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

instr FixedTarget
  ; A zero threshold makes p4 the target gain. p5 is seconds per gain unit.
  aInput init .25
  aOutput dam aInput, 0, p4, 1, p5, p5
  kSamples init 0
  kSamples += ksmps
  if kSamples == sr then
    iDirection = p4 > 1 ? 1 : -1
    kExpected = .25*(1+iDirection/p5)
    kActual vaget ksmps-1, aOutput
    if !(abs(kActual-kExpected) < .000001) then
      printks "Target %g, time %g: expected %g after one second, got %g\n", 0, p4, p5, kExpected, kActual
      exitnowk -1
    endif
    gkChecks += 1
  endif
endin

instr ReverseDirection
  ; Rise for one second, then raise the threshold so gain must fall.
  kSamples init 0
  kThreshold = kSamples < sr ? 0 : 1
  aInput init .25
  aOutput dam aInput, kThreshold, 2, .5, 512, 1024
  kSamples += ksmps
  if kSamples == 2*sr then
    kExpected = .25*(1+1/512-1/1024)
    kActual vaget ksmps-1, aOutput
    if !(abs(kActual-kExpected) < .000001) then
      printks "After a rise and fall: expected %g, got %g\n", 0, kExpected, kActual
      exitnowk -1
    endif
    gkChecks += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) != 3 then
    prints "Not all slow gain checks completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "FixedTarget" 0 1.01 2 512
i "FixedTarget" 0 1.01 .5 1024
i "ReverseDirection" 0 2.01
i "CheckCompletion" 2.02 .01
e
</CsScore>
</CsoundSynthesizer>
