<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
gkChecks init 0

; With 2 Hz at kr=16, a main/offbeat pair takes 16 control cycles.
instr 1
  kCycle init 0
  kCycle += 1
  kTick metro2 2, p4, -1, p5, 1
  iPhase = frac(p5)
  iHeld = (iPhase == 0 || iPhase == p4 ? 1 : 0)
  kPosition = iPhase*16+kCycle-iHeld
  kExpected = 0
  if kPosition % 16 == 0 then
    kExpected = 1
  endif
  if (kPosition-p4*16) % 16 == 0 then
    kExpected = -1
  endif
  if kCycle == 1 && iHeld == 1 then
    kExpected = (iPhase == 0 ? 1 : -1)
  endif
  if kTick != kExpected then
    printks "metro2 swing %g phase %g cycle %g: %g expected %g\n", 0, p4, p5, kCycle, kTick, kExpected
    exitnowk -1
  endif
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

; A fast cycle must not leave ticks queued after the frequency becomes zero.
instr 2
  kCycle init 0
  kCycle += 1
  kFrequency = (kCycle == 2 ? p4 : 0)
  kTick metro2 kFrequency, .5, -1, 0, 1
  kExpected = (kCycle == 1 ? 1 : (kCycle == 2 ? -1 : 0))
  if kTick != kExpected then
    printks "metro2 fast frequency %g cycle %g: %g expected %g\n", 0, p4, kCycle, kTick, kExpected
    exitnowk -1
  endif
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

; Changing swing must move the next offbeat, without duplicating a tick.
instr 3
  kCycle init 0
  kCycle += 1
  kSwing = (kCycle < 5 ? .5 : .75)
  kTick metro2 2, kSwing, -1, 0, 1
  kExpected = (kCycle == 1 || kCycle == 17 ? 1 : (kCycle == 13 || kCycle == 29 ? -1 : 0))
  if kTick != kExpected then
    printks "metro2 changing swing cycle %g: %g expected %g\n", 0, kCycle, kTick, kExpected
    exitnowk -1
  endif
  if kCycle == 32 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 14 then
    prints "metro2 checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 3 .5 0
i 1 0 3 .5 .75
i 1 0 3 .5 .5
i 1 0 3 .25 0
i 1 0 3 .75 .25
i 1 0 3 0 0
i 1 0 3 1 0
i 1 0 3 .5 1
i 2 0 1 64
i 2 0 1 1024
i 2 0 1 1e20
i 3 0 3
i 1 0 3 .5 1e20
; Reuse an instance with an exact initial offbeat.
i 1 3 3 .75 .75
i 99 6.5 .1
e
</CsScore>
</CsoundSynthesizer>
