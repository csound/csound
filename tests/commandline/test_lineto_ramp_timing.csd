<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kCycle init 0
  kCycle += 1
  kInput = kCycle == 1 ? 0 : (kCycle < 12 ? 1 : .25)
  kTrigger = kCycle == 2 ? 1 : (kCycle == 12 ? -1 : 0)
  kLine lineto kInput, p4/kr
  kTriggered tlineto kInput, p4/kr, kTrigger
  if p4 <= 0 then
    kExpected = kInput
  elseif kCycle < 12 then
    kExpected = min(max(kCycle-2, 0)/ceil(p4), 1)
  else
    kExpected = 1 - .75*min((kCycle-12)/ceil(p4), 1)
  endif
  kError = abs(kLine-kExpected) + abs(kTriggered-kExpected)
  if !(kError <= .000001) then
    printks "lineto/tlineto duration %g failed at cycle %d: %g\n", 0, p4, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 20 then
    gkChecks += 1
  endif
endin

instr 2
  kCycle init 0
  kCycle += 1
  kInput = kCycle == 1 ? 0 : (kCycle < 4 ? 1 : .6)
  kTrigger = kCycle == 2 || kCycle == 4 ? 1 : 0
  kOutput tlineto kInput, 4/kr, kTrigger
  if kCycle < 4 then
    kExpected = max(kCycle-2, 0)/4
  else
    kExpected = .25 + .35*min((kCycle-4)/4, 1)
  endif
  if !(abs(kOutput-kExpected) <= .000001) then
    printks "tlineto retrigger failed at cycle %d: expected %g, got %g\n", 0, kCycle, kExpected, kOutput
    exitnowk(-1)
  endif
  if kCycle == 12 then
    gkChecks += 1
  endif
endin

instr 3
  kCycle init 0
  kCycle += 1
  kInput = kCycle == 1 ? 0 : (kCycle == 2 ? 1 : 2)
  kTime = kCycle < 3 ? 4/kr : 2/kr
  kOutput lineto kInput, kTime
  if kCycle < 6 then
    kExpected = max(kCycle-2, 0)/4
  else
    kExpected = 1 + min((kCycle-6)/2, 1)
  endif
  if !(abs(kOutput-kExpected) <= .000001) then
    printks "lineto changed an active ramp at cycle %d: expected %g, got %g\n", 0, kCycle, kExpected, kOutput
    exitnowk(-1)
  endif
  if kCycle == 12 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 8 then
    prints "lineto/tlineto checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02 -1
i 1 0 .02 0
i 1 0 .02 .5
i 1 0 .02 1
i 1 0 .02 4
i 1 0 .02 4.5
i 2 .03 .012
i 3 .05 .012
i 99 .07 .001
</CsScore>
</CsoundSynthesizer>
