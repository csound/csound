<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 2
0dbfs = 1
giFile ftgen 0, 0, 0, -1, "test_flooper_stereo_guard.wav", 0, 0, 0
; Copy the samples into a table without soundfile rate metadata.
giRaw ftgen 0, 0, -ftlen(giFile), -2, 0
iN = 0
while iN < ftlen(giFile) do
  iValue table iN, giFile
  tableiw iValue, iN, giRaw
  iN += 1
od
gkChecks init 0

instr 1
  iTable = (p6 == 0 ? giFile : giRaw)
  iRatio = (p6 == 0 ? p7*sr/44100 : p7)
  iOffset = int(p2*sr + .5) % ksmps
  kCount init 0
  kPhase init 1
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  aAmp init 0
  kN = 0
  while kN < ksmps do
    kValue = 0
    if kN >= kStart && kN < kEnd then
      kValue = .25 + (kCount + kN - kStart)/128
    endif
    vaset kValue, kN, aAmp
    kN += 1
  od
  if p4 == 1 then
    if p5 == 0 then
      aLeft, aRight lposcilsa aAmp, iRatio, 1, 6, iTable, 1
    elseif p5 == 1 then
      aAmp, aRight lposcilsa aAmp, iRatio, 1, 6, iTable, 1
      aLeft = aAmp
    else
      aLeft, aAmp lposcilsa aAmp, iRatio, 1, 6, iTable, 1
      aRight = aAmp
    endif
  else
    if p5 == 0 then
      aLeft, aRight lposcilsa2 aAmp, p7, 1, 6, iTable, 1
    elseif p5 == 1 then
      aAmp, aRight lposcilsa2 aAmp, p7, 1, 6, iTable, 1
      aLeft = aAmp
    else
      aLeft, aAmp lposcilsa2 aAmp, p7, 1, 6, iTable, 1
      aRight = aAmp
    endif
  endif
  kN = 0
  while kN < ksmps do
    kExpectedL = 0
    kExpectedR = 0
    if kN >= kStart && kN < kEnd then
      kIndex = int(kPhase)*2
      kFraction = kPhase - int(kPhase)
      kL table kIndex, giFile
      kR table kIndex + 1, giFile
      kNextL table kIndex + 2, giFile
      kNextR table kIndex + 3, giFile
      kAmp = .25 + (kCount + kN - kStart)/128
      kExpectedL = kAmp*(kL + kFraction*(kNextL - kL))
      kExpectedR = kAmp*(kR + kFraction*(kNextR - kR))
      kPhase += p7
      if kPhase >= 6 then
        kPhase -= 5
      elseif kPhase < 1 then
        kPhase += 5
      endif
    endif
    kLeft vaget kN, aLeft
    kRight vaget kN, aRight
    if !(abs(kLeft - kExpectedL) < .000001 && abs(kRight - kExpectedR) < .000001) then
      printks "FAIL lposcilsa variant=%g reuse=%g raw=%g sample=%g: (%g,%g) expected (%g,%g)\n", \
          0, p4, p5, p6, kCount + kN - kStart, kLeft, kRight, kExpectedL, kExpectedR
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 14 then
    prints "FAIL lposcilsa checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; p7 is the phase step in frames. 44100/32768 gives an exact file-rate
; ratio of 1/32 in both float and double builds.
; Separate outputs, then amplitude reused as each output. Both table types.
i 1 0.0 .0625 1 0 0 1.3458251953125
i 1 0.1279296875 .0625 1 1 0 1.3458251953125
i 1 0.2529296875 .0625 1 2 0 1.3458251953125
i 1 0.375 .0625 1 0 1 .5
i 1 0.5029296875 .0625 1 1 1 .5
i 1 0.6279296875 .0625 1 2 1 .5
i 1 0.75 .0625 2 0 0 2
i 1 0.8779296875 .0625 2 1 0 2
i 1 1.0029296875 .0625 2 2 0 2
i 1 1.125 .0625 2 0 1 2
i 1 1.2529296875 .0625 2 1 1 2
i 1 1.3779296875 .0625 2 2 1 2
; Reverse playback uses the same amplitude for both channels.
i 1 1.5029296875 .0625 1 1 0 -1.3458251953125
i 1 1.6279296875 .0625 2 1 0 -1
i 99 1.75 .015625
</CsScore>
</CsoundSynthesizer>
