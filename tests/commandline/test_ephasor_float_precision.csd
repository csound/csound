<CsTest>
description = "ephasor uses phase remainders to time exponential resets"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr Cross
  ; p6 is the exact wrap sample. Use a count instead of adding tiny steps
  ; to a float phase in the reference calculation.
  aFrequency = p5*sr
  aAmplitude,aPhase ephasor p5*sr, .9, p4
  aAudioAmplitude,aFrequency ephasor aFrequency, .9, p4
  kBase init 0
  kExpected init 1
  kN = 0
  while kN < ksmps do
    kSample = kBase+kN
    kAmp vaget kN, aAmplitude
    kAudioAmp vaget kN, aAudioAmplitude
    kPhase vaget kN, aPhase
    kAudioPhase vaget kN, aFrequency
    kReference = p4+kSample*p5
    kReference -= floor(kReference)
    kDistance = abs(kPhase-kReference)
    kDistance = min(kDistance, 1-kDistance)
    if !(abs(kAmp-kExpected) < .000002 && abs(kAudioAmp-kExpected) < .000002 && kPhase >= 0 && kPhase < 1 && kAudioPhase >= 0 && kAudioPhase < 1 && abs(kAudioPhase-kPhase) < .0000002 && kDistance < .0000002) then
      printks "Cross phase=%g step=%g sample=%g: amplitude %g/%g expected %g, phase %g/%g\n", 0, p4, p5, kSample, kAmp, kAudioAmp, kExpected, kPhase, kAudioPhase
      exitnowk -1
    endif
    if kSample+1 == p6 then
      kExpected = (p5 > 0 ? .9 : .9^(2+p5))
    else
      kExpected *= .9
    endif
    kN += 1
  od
  kBase += ksmps
  if kBase == 64 then
    gkChecks += 1
  endif
endin

instr RetainRemainder
  ; At block two the rounded phase equals one, but has not wrapped.
  ; p4 selects pausing, preserving phase on reinit, or resetting phase.
  kBlock init 0
  kInitial init 1-1/16777216
  kExpected init 1
  kStep = (p4 == 0 && kBlock == 2 ? 0 : 1/1073741824)
  aFrequency = kStep*sr
  if p4 != 0 && kBlock == 2 then
    kInitial = (p4 == 1 ? -1 : 1-1/16777216)
    kExpected = 1
    reinit OSC
  endif
OSC:
  aAmplitude,aPhase ephasor kStep*sr, .99, i(kInitial)
  aAudioAmplitude,aAudioPhase ephasor aFrequency, .99, i(kInitial)
  rireturn
  kN = 0
  while kN < ksmps do
    kSample = kBlock*ksmps+kN
    kAmp vaget kN, aAmplitude
    kAudioAmp vaget kN, aAudioAmplitude
    kPhase vaget kN, aPhase
    kAudioPhase vaget kN, aAudioPhase
    if !(abs(kAmp-kExpected) < .000002 && abs(kAudioAmp-kExpected) < .000002 && kPhase >= 0 && kPhase < 1 && kAudioPhase >= 0 && kAudioPhase < 1 && abs(kAudioPhase-kPhase) < .0000002) then
      printks "Retain remainder mode=%g sample=%g: amplitude %g/%g expected %g\n", 0, p4, kSample, kAmp, kAudioAmp, kExpected
      exitnowk -1
    endif
    kWrap = (p4 == 0 ? 80 : (p4 == 1 ? 64 : 96))
    if kSample+1 == kWrap then
      kExpected = .99
    else
      kExpected *= .99
    endif
    kN += 1
  od
  kBlock += 1
  if kBlock == 8 then
    gkChecks += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) != 6 then
    prints "Not all ephasor float checks completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Cross" 0 .0625 [1-1/16777216] [1/67108864] 4
i "Cross" 0 .0625 [1/16777216] [-1/67108864] 5
i "Cross" 0 .0625 0 [-1/67108864] 1
i "RetainRemainder" 0 .125 0
i "RetainRemainder" 0 .125 1
i "RetainRemainder" 0 .125 2
i "CheckCompletion" .25 .015625
e
</CsScore>
</CsoundSynthesizer>
