<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps=16
nchnls=1
0dbfs=1
gkNotes init 0
gkGains init 0

instr 1
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart - iStart%ksmps
  kCycle init 0
  kFrequency = (kCycle%2 == 0 ? 512 : 1536)
  kResonance = (kCycle%3 == 0 ? 1 : 4)
  aFrequency upsamp kFrequency
  aResonance upsamp kResonance
  aInput oscili .01, 256
  aKK bqrez aInput, kFrequency, kResonance, p4
  aAK bqrez aInput, aFrequency, kResonance, p4
  aKA bqrez aInput, kFrequency, aResonance, p4
  aAA bqrez aInput, aFrequency, aResonance, p4
  aReuse = aInput
  aReuse bqrez aReuse, aFrequency, aResonance, p4

  ; Independent bilinear-transform coefficients, using g = tan(pi*f/sr).
  ; Preserve bqrez's factor of two in the low/high/band-pass modes.
  kG = tan($M_PI*kFrequency/sr)
  kDen = 1 + kG/kResonance + kG*kG
  kA1 = 2*(kG*kG-1)/kDen
  kA2 = (1-kG/kResonance+kG*kG)/kDen
  if p4 == 0 then
    kB0 = 2*kG*kG/kDen
    kB1 = 2*kB0
    kB2 = kB0
  elseif p4 == 1 then
    kB0 = 2/kDen
    kB1 = -2*kB0
    kB2 = kB0
  elseif p4 == 2 then
    kB0 = 2*kG/kDen
    kB1 = 0
    kB2 = -kB0
  elseif p4 == 3 then
    kB0 = (1+kG*kG)/kDen
    kB1 = kA1
    kB2 = kB0
  else
    kB0 = kA2
    kB1 = kA1
    kB2 = 1
  endif
  aReference biquad aInput, kB0, kB1, kB2, 1, kA1, kA2
  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle*ksmps + kN
    kReference vaget kN, aReference
    kKK vaget kN, aKK
    kAK vaget kN, aAK
    kKA vaget kN, aKA
    kAA vaget kN, aAA
    kReuse vaget kN, aReuse
    if kTime < iStart || kTime >= iEnd then
      kReference = 0
    endif
    if abs(kKK-kReference) > .000002 || abs(kAK-kReference) > .000002 || abs(kKA-kReference) > .000002 || abs(kAA-kReference) > .000002 || abs(kReuse-kReference) > .000002 then
      printks "bqrez mode %g sample %g: %g %g %g %g %g, expected %g\n", 0, p4, kTime, kKK, kAK, kKA, kAA, kReuse, kReference
      exitnowk -1
    endif
    kN += 1
  od
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 2
  setksmps 1
  aInput oscili .01, p4
  aOutput bqrez aInput, p4, p5, 2
  kInput downsamp aInput
  kOutput downsamp aOutput
  kSample init 0
  kInPower init 0
  kOutPower init 0
  if kSample >= 4096 then
    kInPower += kInput*kInput
    kOutPower += kOutput*kOutput
  endif
  if kSample == 8191 then
    kGain = sqrt(kOutPower/kInPower)
    if abs(kGain-2*p5) > .0001 then
      printks "bqrez center %g resonance %g: gain %g, expected %g\n", 0, p4, p5, kGain, 2*p5
      exitnowk -1
    endif
    gkGains += 1
  endif
  kSample += 1
endin

instr 99
  if i(gkNotes) != 10 || i(gkGains) != 4 then
    prints "bqrez cases did not all run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 0
i 1 0 .03125 1
i 1 0 .03125 2
i 1 0 .03125 3
i 1 0 .03125 4
i 1 .0631103515625 .0205078125 0
i 1 .0631103515625 .0205078125 1
i 1 .0631103515625 .0205078125 2
i 1 .0631103515625 .0205078125 3
i 1 .0631103515625 .0205078125 4
i 2 .125 1 1024 1
i 2 .125 1 2048 1
i 2 .125 1 1024 4
i 2 .125 1 2048 4
i 99 1.25 0
e
</CsScore>
</CsoundSynthesizer>
