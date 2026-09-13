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

; With no spring force, equal velocities keep every position equal.
instr 1
  iPosition ftgen 0, 0, -p4, -2, 0
  iMass ftgen 0, 0, -p4, -7, 1, p4, 1
  iStiff ftgen 0, 0, -p4, -2, 0
  iDamp ftgen 0, 0, -p4, -7, 1, p4, 1
  iVelocity ftgen 0, 0, -p4, -7, 1, p4, 1
  aSignal scantable 1, p5, iPosition, iMass, iStiff, iDamp, iVelocity
  kCycle init 0
  kCycle += 1
  kError max_k abs(aSignal-(kCycle-1)/kr), 1, 1
  ; Interpolation across the final element uses the guard point.
  kPosition tablei p4-.5, iPosition
  kVelocity tablei p4-.5, iVelocity
  if !(kError < .000001 && abs(kPosition-kCycle/kr) < .000001 && kVelocity == 1) then
    printks "scantable size %g pitch %g: audio error %g, position %g, velocity %g\n", 0, p4, p5, kError, kPosition, kVelocity
    exitnowk -1
  endif
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

; Fixed points turn the model into a plain table oscillator.
instr 2
  iPosition ftgen 0, 0, 8, -7, 0, 8, 1
  iMass ftgen 0, 0, 8, -2, 0
  iStiff ftgen 0, 0, 8, -2, 0
  iDamp ftgen 0, 0, 8, -7, 1, 8, 1
  iVelocity ftgen 0, 0, 8, -2, 0
  kCycle init 0
  kCycle += 1
  kPitch = (p4 == 0 ? (kCycle == 1 ? 256 : 1e20) : p4)
  kReferencePitch = (p4 == 0 ? (kCycle == 1 ? 256 : 0) : p5)
  aSignal scantable 1, kPitch, iPosition, iMass, iStiff, iDamp, iVelocity
  aPhase phasor kReferencePitch
  aReference table aPhase, iPosition, 1
  kError max_k abs(aSignal-aReference), 1, 1
  if !(kError < .000001) then
    printks "scantable pitch %g differs from reference %g: %g\n", 0, p4, p5, kError
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 17 then
    prints "scantable checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .04 32 256
i 1 0 .04 7 8192
i 1 0 .04 1 512
i 1 0 .04 32 -256
i 1 0 .04 32 16640
i 1 0 .04 32 -16640
i 2 .05 .01 512 512
i 2 .05 .01 -512 -512
i 2 .05 .01 8192 0
i 2 .05 .01 -8192 0
i 2 .05 .01 16896 512
i 2 .05 .01 -16896 -512
; Begin three samples into a block and finish before its end.
i 2 .0706787109375 .0009765625 512 512
i 2 .0706787109375 .0009765625 -512 -512
i 2 .09 .01 1e20 0
i 2 .09 .01 -1e20 0
; Whole scans must preserve an already nonzero phase.
i 2 .10 .01 0 0
i 99 .12 .001
e
</CsScore>
</CsoundSynthesizer>
