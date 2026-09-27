<CsTest>
description = "phasor and phasorbnk read frequency before writing a reused output"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 64
ksmps = 8
nchnls = 1
0dbfs = 1
gkSequenceChecks init 0
gkBankChecks init 0

instr CheckFrequencyChanges
  ; At kr=8, each cycle advances by frequency/8, after emitting its phase.
  ; Include forward and reverse wraps, frequency changes, and zero-frequency holds.
  kFrequencies[] fillarray 1, 1, 4, 0, -2, -4, 1, 1, 0
  kPhases[] fillarray .25, .375, .5, 0, 0, .75, .25, .375, .5
  kCycle init 0
  kFrequency = kFrequencies[kCycle]
  kExpected = kPhases[kCycle]

  kSeparate phasor kFrequency, .25
  kReused = kFrequency
  kReused phasor kReused, .25
  if kSeparate != kExpected || kReused != kExpected then
    printks "phasor cycle %g: expected %g, separate %g, reused %g\n", 0, kCycle, kExpected, kSeparate, kReused
    exitnowk -1
  endif

  kBankSeparate phasorbnk kFrequency, 0, 1, .25
  kBankReused = kFrequency
  kBankReused phasorbnk kBankReused, 0, 1, .25
  if kBankSeparate != kExpected || kBankReused != kExpected then
    printks "phasorbnk cycle %g: expected %g, separate %g, reused %g\n", 0, kCycle, kExpected, kBankSeparate, kBankReused
    exitnowk -1
  endif
  kCycle += 1
  gkSequenceChecks += 1
endin

instr CheckBankLoop
  kCycle init 0
  kIndex = 0
  while kIndex < 2 do
    ; One opcode instance serves two independent banks, at 1 Hz and 2 Hz.
    kFrequency = kIndex+1
    kSeparate phasorbnk kFrequency, kIndex, 2, .25
    kReused = kFrequency
    kReused phasorbnk kReused, kIndex, 2, .25
    kExpected = frac(.25 + (kIndex+1)*kCycle/kr)
    if kSeparate != kExpected || kReused != kExpected then
      printks "phasorbnk bank %g, cycle %g: expected %g, separate %g, reused %g\n", 0, kIndex, kCycle, kExpected, kSeparate, kReused
      exitnowk -1
    endif
    kIndex += 1
    gkBankChecks += 1
  od
  kCycle += 1
endin

instr CheckCompletion
  if i(gkSequenceChecks) != 9 || i(gkBankChecks) != 16 then
    prints "The frequency sequence and both banks must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckFrequencyChanges" 0 [9/8]
i "CheckBankLoop" 1.25 1
i "CheckCompletion" 2.375 .125
e
</CsScore>
</CsoundSynthesizer>
