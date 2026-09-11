<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
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
  kFrequency = kCycle < 3 ? 50 : (kCycle < 10 ? 500 : 100)
  kPulse gausstrig 1, kFrequency, 0, p4, p5
  aPulse gausstrig 1, kFrequency, 0, p4, p5
  kReuse = kFrequency
  kReuse gausstrig 1, kReuse, 0, p4, p5
  kAudio max_k aPulse, 1, 1
  if p4 == 0 then
    kExpected = kCycle == 21 || kCycle == 31 ? 1 : 0
  else
    kExpected = kCycle == 5 || kCycle == 7 || kCycle == 9 || kCycle == 20 || kCycle == 30 ? 1 : 0
  endif
  if kCycle == 1 && p5 == 0 then
    kExpected = 1
  endif
  if kPulse != kExpected || kReuse != kExpected || kAudio != kExpected then
    printks "gausstrig mode=%d first=%d cycle=%d: expected %g, got k=%g a=%g reused=%g\n", 0, p4, p5, kCycle, kExpected, kPulse, kAudio, kReuse
    exitnowk(-1)
  endif
  if kCycle == 32 then
    gkChecks += 1
  endif
endin

instr 2
  kCycle init 0
  kCycle += 1
  kPulse gausstrig 1, 250, 0, p4, p5
  aPulse gausstrig 1, 250, 0, p4, p5
  kAudio max_k aPulse, 1, 1
  kExpected = (kCycle-1) % 4 == 0 ? 1 : 0
  if kCycle == 1 && p5 != 0 then
    kExpected = 0
  endif
  if kPulse != kExpected || kAudio != kExpected then
    printks "gausstrig constant frequency/first impulse failed: mode=%d first=%d cycle=%d\n", 0, p4, p5, kCycle
    exitnowk(-1)
  endif
  if kCycle == 12 then
    gkChecks += 1
  endif
endin

instr 3
  aPulse gausstrig 1, 1000, 0, p4, p5
  aExpected mpulse 1, .001
  if p5 != 0 then
    aExpected delay aExpected, .001
  endif
  aError = abs(aPulse-aExpected)
  kError max_k aError, 1, 1
  if kError != 0 then
    printks "gausstrig partial block timing failed: mode=%d first=%d\n", 0, p4, p5
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 12 then
    prints "gausstrig checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .032 0 0
i 1 0 .032 1 0
i 1 0 .032 0 1
i 1 0 .032 1 1
i 2 .04 .012 0 0
i 2 .04 .012 1 0
i 2 .04 .012 0 1
i 2 .04 .012 1 1
i 3 .060625 .003 0 0
i 3 .070625 .003 1 0
i 3 .080625 .003 0 1
i 3 .090625 .003 1 1
i 99 .1 .001
</CsScore>
</CsoundSynthesizer>
