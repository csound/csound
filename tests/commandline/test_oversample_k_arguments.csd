<CsTest>
description = "oversample converts k values in modes 0 to 4 and holds them in modes 5 to 9"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

gkPassed init 0
gkLocalMisreads init 0
gkNotes init 0

opcode Pass(value:k, factor:i, inMode:i, outMode:i):k
  oversample(factor, inMode, outMode)
  if value != gkPassed then
    gkLocalMisreads += 1
  endif
  result:k = value
  xout(result)
endop

opcode Copy(value:k, factor:i, inMode:i, outMode:i):k
  oversample(factor, inMode, outMode)
  result:k = value
  xout(result)
endop

; The local cycle count goes out: held, the caller gets the last of each group.
opcode LocalPeriod(factor:i, outMode:i):k
  oversample(factor, 0, outMode)
  period:k = timeinstk()
  xout(period)
endop

; Classic, with xin before the rate, for an output that is also the input.
opcode Increment, k, kii
  kValue, iFactor, iMode xin
  oversample iFactor, iMode
  xout kValue + 1
endop

opcode PassArray(values:k[], factor:i, mode:i):k[]
  oversample(factor, mode)
  results:k[] = values * 2
  xout(results)
endop

opcode IncrementArray, k[], k[]ii
  kValues[], iFactor, iMode xin
  oversample iFactor, iMode
  kResults[] = kValues + 1
  xout kResults
endop

opcode OverAudio(signal:a, factor:i, mode:i):a
  oversample(factor, mode)
  filtered:a = tone(signal, 3000)
  xout(filtered)
endop

opcode UnderAudio(signal:a, factor:i, mode:i):a
  undersample(factor, mode)
  filtered:a = tone(signal, 3000)
  xout(filtered)
endop

opcode UnderCopy(value:k, factor:i, mode:i):k
  undersample(factor, mode)
  result:k = value
  xout(result)
endop

; A held k and a string out of one UDO, each also its input: neither is
; converted, so both leave on the last local k-period.
opcode Both, kS, kSii
  kCount, SValue, iFactor, iMode xin
  oversample iFactor, iMode
  SResult strcatk SValue, "x"
  xout kCount + 1, SResult
endop

; Modes 5 to 9: on time, held through the local k-periods.
instr Held
  iFactor = p4
  iMode = p5
  kPeriod = timeinstk()
  if kPeriod == 1 then
    gkLocalMisreads = 0
  endif

  ; Starts at 1, for the first k-period, and steps at the hundredth. The
  ; output mode is left out, to take the input's.
  kPassed = kPeriod < 100 ? 1 : 0
  gkPassed = kPassed
  kReturned = Pass(kPassed, iFactor, iMode, -1)
  if kReturned != kPassed || gkLocalMisreads != 0 then
    printks "factor %g, mode %g, k-period %g: passed %g, returned %g, misread %g times\n", \
      0, iFactor, iMode, kPeriod, kPassed, kReturned, gkLocalMisreads
    exitnowk -1
  endif

  kLocal = LocalPeriod(iFactor, iMode)
  if kLocal != kPeriod * iFactor then
    printks "factor %g, mode %g, k-period %g: returned local k-period %g\n", \
      0, iFactor, iMode, kPeriod, kLocal
    exitnowk -1
  endif

  kCount init 0
  kCount Increment kCount, iFactor, iMode
  if kCount != kPeriod then
    printks "factor %g, mode %g, k-period %g: count %g\n", \
      0, iFactor, iMode, kPeriod, kCount
    exitnowk -1
  endif

  kArray[] init 2
  kArray[0] = kPassed
  kArray[1] = 1 - kPassed
  kDoubled[] = PassArray(kArray, iFactor, iMode)
  if kDoubled[0] != 2 * kPassed || kDoubled[1] != 2 - 2 * kPassed then
    printks "factor %g, mode %g, k-period %g: passed %g, %g, returned %g, %g\n", \
      0, iFactor, iMode, kPeriod, kArray[0], kArray[1], kDoubled[0], kDoubled[1]
    exitnowk -1
  endif

  ; A k[] returned into this array is copied out at init, from the last note.
  kCounts[] init 2
  if kPeriod == 1 then
    kCounts[0] = 0
    kCounts[1] = 0
  endif
  kCounts IncrementArray kCounts, iFactor, iMode
  if kCounts[0] != kPeriod || kCounts[1] != kPeriod then
    printks "factor %g, mode %g, k-period %g: counts %g, %g\n", \
      0, iFactor, iMode, kPeriod, kCounts[0], kCounts[1]
    exitnowk -1
  endif

  if kPeriod == 150 then
    gkNotes += 1
  endif
endin

; Modes 0 to 4: a step is converted, so it is late, and it lands.
instr Converted
  iFactor = p4
  iMode = p5
  kPeriod = timeinstk()
  if kPeriod == 1 then
    gkLocalMisreads = 0
  endif

  kPassed = kPeriod < 100 ? 0 : 1
  gkPassed = kPassed
  kReturned = Pass(kPassed, iFactor, iMode, iMode)
  if kPeriod == 100 && kReturned == kPassed then
    printks "factor %g, mode %g: returned %g on the step's own k-period\n", \
      0, iFactor, iMode, kReturned
    exitnowk -1
  endif
  ; Past the longest converter's settling time.
  if kPeriod == 520 then
    if abs(kReturned - 1) > 0.001 || gkLocalMisreads == 0 then
      printks "factor %g, mode %g: returned %g, misread %g times\n", \
        0, iFactor, iMode, kReturned, gkLocalMisreads
      exitnowk -1
    endif
    gkNotes += 1
  endif
endin

; Each side has its own mode: p5 is the converter, and p5 + 5 the same held.
instr Mixed
  iFactor = p4
  iConverted = p5
  iHeld = p5 + 5
  kPeriod = timeinstk()
  if kPeriod == 1 then
    gkLocalMisreads = 0
  endif

  kPassed = kPeriod < 100 ? 0 : 1
  gkPassed = kPassed
  ; Argument held, output converted: read on time, returned late.
  kReturned = Pass(kPassed, iFactor, iHeld, iConverted)
  if gkLocalMisreads != 0 || (kPeriod == 100 && kReturned == kPassed) then
    printks "factor %g, converter %g, k-period %g: returned %g, misread %g times\n", \
      0, iFactor, iConverted, kPeriod, kReturned, gkLocalMisreads
    exitnowk -1
  endif

  ; Argument converted, output held: late coming in.
  kCopied = Copy(kPassed, iFactor, iConverted, iHeld)
  if kPeriod == 100 && kCopied == kPassed then
    printks "factor %g, converter %g: copied %g on the step's own k-period\n", \
      0, iFactor, iConverted, kCopied
    exitnowk -1
  endif

  ; An output alone: on time held, late converted.
  kHeld = LocalPeriod(iFactor, iHeld)
  kLate = LocalPeriod(iFactor, iConverted)
  if kHeld != kPeriod * iFactor || (kPeriod == 100 && kLate == kHeld) then
    printks "factor %g, converter %g, k-period %g: held %g, converted %g\n", \
      0, iFactor, iConverted, kPeriod, kHeld, kLate
    exitnowk -1
  endif

  if kPeriod == 150 then
    gkNotes += 1
  endif
endin

; A held mode's audio is its converter's, sample for sample, in both opcodes;
; and undersample's k is on time either way.
instr SameAudio
  iFactor = p4
  iConverted = p5
  iHeld = p5 + 5
  kPeriod = timeinstk()

  aSignal = oscili(0.5, 440)
  aOverConverted = OverAudio(aSignal, iFactor, iConverted)
  aOverHeld = OverAudio(aSignal, iFactor, iHeld)
  aUnderConverted = UnderAudio(aSignal, iFactor, iConverted)
  aUnderHeld = UnderAudio(aSignal, iFactor, iHeld)

  kPeak init 0
  kSample = 0
  while kSample < ksmps do
    kOverConverted vaget kSample, aOverConverted
    kOverHeld vaget kSample, aOverHeld
    kUnderConverted vaget kSample, aUnderConverted
    kUnderHeld vaget kSample, aUnderHeld
    if kOverConverted != kOverHeld || kUnderConverted != kUnderHeld then
      printks "factor %g, converter %g, k-period %g, sample %g: over %g, %g, under %g, %g\n", \
        0, iFactor, iConverted, kPeriod, kSample, kOverConverted, kOverHeld, \
        kUnderConverted, kUnderHeld
      exitnowk -1
    endif
    kPeak = abs(kOverConverted) > kPeak ? abs(kOverConverted) : kPeak
    kSample += 1
  od

  kPassed = kPeriod < 100 ? 0 : 1
  kConverted = UnderCopy(kPassed, iFactor, iConverted)
  kHeld = UnderCopy(kPassed, iFactor, iHeld)
  if kConverted != kPassed || kHeld != kPassed then
    printks "factor %g, converter %g, k-period %g: passed %g, returned %g, %g\n", \
      0, iFactor, iConverted, kPeriod, kPassed, kConverted, kHeld
    exitnowk -1
  endif

  if kPeriod == 150 then
    if kPeak < 0.1 then
      printks "factor %g, converter %g: peak %g\n", 0, iFactor, iConverted, kPeak
      exitnowk -1
    endif
    gkNotes += 1
  endif
endin

instr Together
  iFactor = p4
  iMode = p5
  kPeriod = timeinstk()
  kCount init 0
  SValue init "a"
  kCount, SValue Both kCount, SValue, iFactor, iMode
  kLength strlenk SValue
  ; "a", the init pass's "x", and one for each caller k-period.
  if kCount != kPeriod || kLength != 2 + kPeriod then
    printks "factor %g, mode %g, k-period %g: count %g, string of length %g\n", \
      0, iFactor, iMode, kPeriod, kCount, kLength
    exitnowk -1
  endif
  if kPeriod == 5 then
    gkNotes += 1
    turnoff
  endif
endin

instr CheckResults
  if i(gkNotes) != 50 then
    prints "Expected 50 notes to reach their checks, got %g\n", i(gkNotes)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Every converter, sinc to linear, at two factors, for each instrument.
i "Held" 0 0.2 2 5
i "Held" + . 2 6
i "Held" + . 2 7
i "Held" + . 2 8
i "Held" + . 2 9
i "Held" + . 4 5
i "Held" + . 4 6
i "Held" + . 4 7
i "Held" + . 4 8
i "Held" + . 4 9
i "Converted" 2 0.7 2 0
i "Converted" + . 2 1
i "Converted" + . 2 2
i "Converted" + . 2 3
i "Converted" + . 2 4
i "Converted" + . 4 0
i "Converted" + . 4 1
i "Converted" + . 4 2
i "Converted" + . 4 3
i "Converted" + . 4 4
i "Mixed" 9 0.2 2 0
i "Mixed" + . 2 1
i "Mixed" + . 2 2
i "Mixed" + . 2 3
i "Mixed" + . 2 4
i "Mixed" + . 4 0
i "Mixed" + . 4 1
i "Mixed" + . 4 2
i "Mixed" + . 4 3
i "Mixed" + . 4 4
i "SameAudio" 11 0.2 2 0
i "SameAudio" + . 2 1
i "SameAudio" + . 2 2
i "SameAudio" + . 2 3
i "SameAudio" + . 2 4
i "SameAudio" + . 4 0
i "SameAudio" + . 4 1
i "SameAudio" + . 4 2
i "SameAudio" + . 4 3
i "SameAudio" + . 4 4
i "Together" 13 0.02 2 5
i "Together" + . 2 6
i "Together" + . 2 7
i "Together" + . 2 8
i "Together" + . 2 9
i "Together" + . 4 5
i "Together" + . 4 6
i "Together" + . 4 7
i "Together" + . 4 8
i "Together" + . 4 9
i "CheckResults" 13.3 0.1
e
</CsScore>
</CsoundSynthesizer>
