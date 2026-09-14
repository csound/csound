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
  iOffset = int(p2*sr)%ksmps
  iSamples = int(p3*sr)
  kCycle timeinstk
  kCount init 0
  kAmplitude init 1
  kInitial init p5
  kR = kCycle < 3 ? .99 : .9
  aFrequency = p4*sr
  ; Seed inactive output slots so failure to clear them is visible.
  aPhase = 0
  aAudioPhase = 0
  vaset .75, 0, aPhase
  vaset .75, 15, aAudioPhase
  if p7 == 1 && kCycle == 3 then
    kInitial = -1
    kAmplitude = 1
    reinit OSC
  endif
OSC:
  aAmplitude,aPhase ephasor p4*sr,kR,i(kInitial)
  aAudioAmplitude,aAudioPhase ephasor aFrequency,kR,i(kInitial)
  rireturn
  kIndex = 0
  while kIndex < ksmps do
    kPhase vaget kIndex,aPhase
    kAudioPhase vaget kIndex,aAudioPhase
    kAmp vaget kIndex,aAmplitude
    kAudioAmp vaget kIndex,aAudioAmplitude
    if (kCycle != 1 || kIndex >= iOffset) && kCount < iSamples then
      ; Huge increments here are exact whole cycles; keep the oracle small.
      kStep = abs(p4) > 1e10 ? 0 : p4
      kUnwrapped = p6 + kCount*kStep
      kExpected = kUnwrapped - floor(kUnwrapped)
      kNext = kUnwrapped + kStep
      kNextPhase = kNext - floor(kNext)
      kDistance = abs(kPhase-kExpected)
      if kDistance > .5 then
        kDistance = 1-kDistance
      endif
      if !(kPhase >= 0 && kPhase < 1) || kDistance > .00001 || abs(kAudioPhase-kPhase) > .00001 || abs(kAmp-kAmplitude) > .00001 || abs(kAudioAmp-kAmplitude) > .00001 then
        printks "ephasor step=%g sample=%d phase=%g expected=%g amplitude=%g expected=%g\n",0,p4,kCount,kPhase,kExpected,kAmp,kAmplitude
        exitnowk(-1)
      endif
      ; This dyadic case wraps after four samples. Count them explicitly so
      ; the float oracle does not lose the increment next to phase one.
      if p4 == .00000001490116119384765625 then
        if kCount == 3 then
          kAmplitude = kR
        else
          kAmplitude *= kR
        endif
      elseif abs(p4) >= 1 || floor(kNext) != floor(kUnwrapped) then
        kAmplitude = kR ^ (1+kNextPhase)
      else
        kAmplitude *= kR
      endif
      kCount += 1
      if kCount == iSamples then
        gkChecks += 1
      endif
    elseif kPhase != 0 || kAudioPhase != 0 || kAmp != 0 || kAudioAmp != 0 then
      printks "ephasor left an inactive sample uncleared\n",0
      exitnowk(-1)
    endif
    kIndex += 1
  od
endin

instr 99
  if gkChecks != 15 then
    printks "missing ephasor cases: %d\n",0,gkChecks
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
; step, initial phase, wrapped initial phase, reinitialize with negative phase
i 1 0 .0078125 .25 .25 .25 0
i 1 0 .0078125 -.25 .25 .25 0
i 1 0 .0078125 2.5 .25 .25 0
i 1 0 .0078125 -2.5 .25 .25 0
i 1 0 .0078125 2 .25 .25 0
i 1 0 .0078125 -2 .25 .25 0
i 1 0 .0078125 1e20 .25 .25 0
i 1 0 .0078125 -1e20 .25 .25 0
i 1 0 .0078125 0 1e20 0 0
i 1 0 .0078125 .25 1.25 .25 0
i 1 0 .0078125 .00000001490116119384765625 .9999999403953552 .9999999403953552 0
i 1 .0159912109375 .0078125 .25 .25 .25 0
i 1 .0159912109375 .0078125 -2.5 .25 .25 0
i 1 .0159912109375 .0078125 2 .25 .25 0
i 1 .03125 .0078125 .25 .25 .25 1
i 99 .046875 .01
</CsScore>
</CsoundSynthesizer>
