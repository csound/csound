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
giFreqs ftgen 0, 0, -4, -2, 440, 880, 220, 110

instr 1
  iNotes[] fillarray 69, 81, 69.25
  iHz[] mtof iNotes
  iMidi[] ftom iHz
  iRounded[] ftom iHz, 1
  iIndex = 0
  while iIndex < lenarray(iNotes) do
    iExpectedHz mtof iNotes[iIndex]
    iExpectedRounded = round(iNotes[iIndex])
    if abs(iHz[iIndex] - iExpectedHz) > .0001 || \
       abs(iMidi[iIndex] - iNotes[iIndex]) > .0001 || \
       abs(iRounded[iIndex] - iExpectedRounded) > .0001 then
      prints "pitch conversion failed at init time\n"
      exitnow(-1)
    endif
    iIndex += 1
  od

  kNotes[] fillarray 69, 69
  kFreqs[] fillarray 440, 440
  kCycle init 0
  kCycle += 1
  kNotes[0] = 81 + kCycle - 1
  kNotes[1] = 69.25 + kCycle - 1
  kFreqs[0] = 880 * kCycle
  kFreqs[1] = 445 * kCycle
  kHz[] mtof kNotes
  kMidi[] ftom kFreqs
  kRounded[] ftom kFreqs, 1
  kIndex = 0
  while kIndex < lenarray(kNotes) do
    kExpectedHz mtof kNotes[kIndex]
    kExpectedMidi ftom kFreqs[kIndex]
    kExpectedRounded ftom kFreqs[kIndex], 1
    if abs(kHz[kIndex] - kExpectedHz) > .0001 || \
       abs(kMidi[kIndex] - kExpectedMidi) > .0001 || \
       abs(kRounded[kIndex] - kExpectedRounded) > .0001 then
      printks "pitch conversion differs from scalar on cycle %d, element %d\n", 0, kCycle, kIndex
      exitnowk(-1)
    endif
    kIndex += 1
  od
  gkChecks += 1
endin

instr 2
  ; Vary the length within the capacity reserved at init time.
  kSize init 4
  kCycle init 0
  kCycle += 1
  kSize = kCycle + 1
  kFreqs[] tab2array giFreqs, 0, kSize
  kMidi[] ftom kFreqs
  kHz[] mtof kMidi
  kMidiSize lenarray kMidi
  kHzSize lenarray kHz
  if kMidiSize != kSize || kHzSize != kSize then
    printks "pitch conversion output length did not follow input\n", 0
    exitnowk(-1)
  endif
  kIndex = 0
  while kIndex < kSize do
    if abs(kHz[kIndex] - kFreqs[kIndex]) > .001 then
      printks "pitch conversion failed after length change\n", 0
      exitnowk(-1)
    endif
    kIndex += 1
  od
  gkChecks += 1
endin

instr 99
  if i(gkChecks) != 12 then
    prints "pitch conversion checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .003
i 2 0 .003
i 1 .01 .003
i 2 .01 .003
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
