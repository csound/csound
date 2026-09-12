<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kBlock init 0
 kBlock += 1
 kInterval = p5
 kPeriod = p7
 if p8 == 1 then
  ; Changes before the first pulse must not change its scheduled time.
  ; Each pulse uses the interval from its own control block.
  kPeriod = (kBlock == 1 ? 5 : (kBlock == 2 ? 10 : 7))
  kInterval = -kPeriod
 elseif p8 == 2 && kBlock > 1 then
  ; Changing the interval cannot restart a completed one-shot.
  kPeriod = 7
  kInterval = -7
 endif
 aPulse mpulse .5, kInterval, p4
 aGate = 1
 kElapsed init 0
 kNext init p6
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kPulse vaget kN, aPulse
  kExpected = 0
  if kActive != 0 then
   if kElapsed == kNext then
    kExpected = .5
    if kPeriod == 0 then
     kNext = -1
    else
     kNext += kPeriod
    endif
   endif
   kElapsed += 1
  endif
  if kPulse != kExpected then
   printks "mpulse delay=%g interval=%g block=%g sample=%g elapsed=%g actual=%g expected=%g\n", 0, p4, kInterval, kBlock, kN, kElapsed, kPulse, kExpected
   exitnowk(-1)
  endif
  kN += 1
 od
 if kElapsed == 64 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 24 then
  prints "mpulse cases did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Alternate full blocks and notes starting at sample 3.
i 1 0.0 .0078125 0 -32 0 32 0
i 1 0.0159912109375 .0078125 -20 -32 20 32 0
i 1 0.03125 .0078125 0.00244140625 -32 20 32 0
i 1 0.0472412109375 .0078125 -16 -7 16 7 0
i 1 0.0625 .0078125 -17 -7 17 7 0
i 1 0.0784912109375 .0078125 -15 -7 15 7 0
i 1 0.09375 .0078125 -32 -7 32 7 0
i 1 0.1097412109375 .0078125 -40 -7 40 7 0
i 1 0.125 .0078125 0 0.0008544921875 0 7 0
i 1 0.1409912109375 .0078125 -20 0.00390625 20 32 0
i 1 0.15625 .0078125 0.00244140625 0.0008544921875 20 7 0
i 1 0.1722412109375 .0078125 0 -1 0 1 0
i 1 0.1875 .0078125 0 0.0001220703125 0 1 0
i 1 0.2034912109375 .0078125 0 -0.25 0 1 0
i 1 0.21875 .0078125 0 3.0517578125e-05 0 1 0
i 1 0.2347412109375 .0078125 -20 -1.75 20 1 0
i 1 0.25 .0078125 0 0 0 0 0
i 1 0.2659912109375 .0078125 -20 0 20 0 0
i 1 0.28125 .0078125 0 0 0 0 2
i 1 0.2972412109375 .0078125 -20 -32 20 32 1
i 1 0.3125 .0078125 -4294967296 -32 4294967296 32 0
i 1 0.3284912109375 .0078125 524288.0 -32 4294967296 32 0
i 1 0.34375 .0078125 0 -4294967296 0 4294967296 0
i 1 0.3597412109375 .0078125 0 524288.0 0 4294967296 0
i 99 .4 .001
e
</CsScore>
</CsoundSynthesizer>
