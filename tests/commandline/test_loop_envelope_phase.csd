<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kCycle init 0
 kPhase init p4
 kTrig = (p6 == 1 || (p6 == 2 && kCycle%3 == 0) ? 1 : 0)
 if kTrig != 0 then
  kPhase = p4
 endif
 kPhase = kPhase-floor(kPhase)
 if kPhase == 1 then
  kPhase = 0
 endif
 kFrequency = p5*kr
 ; The initial zero-duration segment must be skipped at the loop start.
 kLinear loopseg kFrequency, kTrig, p4, 99, 0, 10, 1, 20, 2, 30, 1
 kCurve loopxseg kFrequency, kTrig, p4, 99, 0, 10, 1, 20, 2, 30, 1
 kHeld lpshold kFrequency, kTrig, p4, 99, 0, 10, 1, 20, 2, 30, 1
 kTyped looptseg kFrequency, kTrig, p4, 99, 0, 0, 10, 0, 1, 20, 0, 2, 30, 0, 1
 kDriven loopsegp kPhase+p7, 99, 0, 10, 1, 20, 2, 30, 1
 kDrivenHeld lpsholdp kPhase+p7, 99, 0, 10, 1, 20, 2, 30, 1
 if kPhase < .25 then
  kStart = 10
  kEnd = 20
  kFraction = kPhase*4
 elseif kPhase < .75 then
  kStart = 20
  kEnd = 30
  kFraction = (kPhase-.25)*2
 else
  kStart = 30
  kEnd = 99
  kFraction = (kPhase-.75)*4
 endif
 kExpected = kStart+(kEnd-kStart)*kFraction
 kExpectedCurve = kStart+(kEnd-kStart)*(exp(kFraction)-1)/(exp(1)-1)
 kError = abs(kLinear-kExpected)+abs(kTyped-kExpected)+abs(kDriven-kExpected)
 kError += abs(kHeld-kStart)+abs(kDrivenHeld-kStart)+abs(kCurve-kExpectedCurve)
 if !(kError < .00005) then
  printks "loop phase=%g step=%g retrigger=%g cycle=%g error=%g\n", 0, p4, p5, p6, kCycle, kError
  exitnowk(-1)
 endif
 kPhase += p5
 kCycle += 1
 if kCycle == 16 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 10 then
  prints "loop envelope checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Initial endpoints, continuous and pulsed retriggers, both directions.
i 1 0 .03125 0 .125 0 0
i 1 0 .03125 1 0 1 1
i 1 0 .03125 1 .125 0 0
i 1 0 .03125 1 -.125 2 -1
i 1 0 .03125 .75 .125 2 1
i 1 0 .03125 -.25 -.125 0 -1
i 1 0 .03125 2.25 4.125 0 8
i 1 0 .03125 -2.25 -4.125 0 -8
i 1 0 .03125 -1e-20 0 1 0
i 1 0 .03125 .5 0 0 0
i 99 .04 .001
e
</CsScore>
</CsoundSynthesizer>
