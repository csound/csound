<CsTest>
description = "every member of an array argument or output of a resampled UDO is converted"

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

gkChecked init 0
giFactor init 1
giMode init 0

; Each array's members are its first member times 1, 2, 3, ..., so every
; member must arrive as that multiple of the first. The UDO's first local is
; not the array, which is what the converters used to be sized from.

opcode AudioOut, a[], ii
  iFactor, iMode xin
  oversample iFactor, iMode
  aSine poscil 0.1, 440
  aOut[] init 3
  aOut[0] = aSine
  aOut[1] = aSine * 2
  aOut[2] = aSine * 3
  xout aOut
endop

opcode AudioOutUnder, a[], ii
  iFactor, iMode xin
  undersample iFactor, iMode
  aSine poscil 0.1, 440
  aOut[] init 3
  aOut[0] = aSine
  aOut[1] = aSine * 2
  aOut[2] = aSine * 3
  xout aOut
endop

; A local ahead of the arguments.
opcode AudioIn, aaa, a[]
  oversample giFactor, giMode
  iLead = 1
  aIn[] xin
  xout aIn[0], aIn[1], aIn[2]
endop

opcode AudioInUnder, aaa, a[]
  undersample giFactor, giMode
  iLead = 1
  aIn[] xin
  xout aIn[0], aIn[1], aIn[2]
endop

opcode ControlOut, k[], ik[]ii
  iGain, kIn[], iFactor, iMode xin
  oversample iFactor, iMode
  kOut[] = kIn * (iGain + timeinstk() * 0.01)
  xout kOut
endop

opcode Grid, k[][], ii
  iFactor, iMode xin
  oversample iFactor, iMode
  kRamp = timeinstk() * 0.01
  kOut[][] init 2, 2
  kOut[0][0] = kRamp
  kOut[0][1] = kRamp * 2
  kOut[1][0] = kRamp * 3
  kOut[1][1] = kRamp * 4
  xout kOut
endop

; A k first, which a reused instance holds from its last note.
opcode Scale(gain:k, values:k[], factor:i, mode:i):k[]
  oversample(factor, mode)
  scaled:k[] = values * (gain + timeinstk() * 0.01)
  xout(scaled)
endop

; How far member n of an array is from n + 1 times member 0, relative to it.
opcode Off, k, kkk
  kFirst, kValue, kMember xin
  kWant = (kMember + 1) * kFirst
  xout abs(kValue - kWant) / (1 + abs(kWant))
endop

opcode Multiples, k, k[]
  kValues[] xin
  kWorst = 0
  kMember = 1
  while kMember < lenarray(kValues) do
    kWorst = max(kWorst, Off(kValues[0], kValues[kMember], kMember))
    kMember += 1
  od
  xout kWorst
endop

opcode AudioMultiples, k, a[]
  aValues[] xin
  kWorst = 0
  kMember = 1
  while kMember < lenarray(aValues) do
    kSample = 0
    while kSample < ksmps do
      kWorst = max(kWorst, Off(vaget(kSample, aValues[0]), \
                               vaget(kSample, aValues[kMember]), kMember))
      kSample += 1
    od
    kMember += 1
  od
  xout kWorst
endop

; p4 factor, p5 mode, p6 1 for undersample
instr AudioOutput
  kPeriod = timeinstk()
  if p6 == 0 then
    aOut[] AudioOut p4, p5
  else
    aOut[] AudioOutUnder p4, p5
  endif
  kWorst = AudioMultiples(aOut)
  if kWorst > 1e-5 then
    printks "under %g, factor %g, mode %g, k-period %g: a[] output off by %g\n", \
      0, p6, p4, p5, kPeriod, kWorst
    exitnowk -1
  endif
  kPeak init 0
  kPeak = max(kPeak, abs(vaget(ksmps - 1, aOut[0])))
  ; All zero would pass the check above.
  if lastcycle() == 1 && kPeak < 0.05 then
    printks "under %g, factor %g, mode %g: a[] output silent\n", 0, p6, p4, p5
    exitnowk -1
  endif
  gkChecked += 1
endin

instr AudioInput
  kPeriod = timeinstk()
  giFactor = p4
  giMode = p5
  aSine poscil 0.1, 330
  aIn[] init 3
  aIn[0] = aSine
  aIn[1] = aSine * 2
  aIn[2] = aSine * 3
  aBack[] init 3
  if p6 == 0 then
    aBack[0], aBack[1], aBack[2] AudioIn aIn
  else
    aBack[0], aBack[1], aBack[2] AudioInUnder aIn
  endif
  kWorst = AudioMultiples(aBack)
  if kWorst > 1e-5 then
    printks "under %g, factor %g, mode %g, k-period %g: a[] input off by %g\n", \
      0, p6, p4, p5, kPeriod, kWorst
    exitnowk -1
  endif
  kPeak init 0
  kPeak = max(kPeak, abs(vaget(ksmps - 1, aBack[0])))
  if lastcycle() == 1 && kPeak < 0.05 then
    printks "under %g, factor %g, mode %g: a[] input silent\n", 0, p6, p4, p5
    exitnowk -1
  endif
  gkChecked += 1
endin

; undersample passes k through: the k checks are for oversample.
; p4 factor, p5 mode, p6 which
instr Control
  kPeriod = timeinstk()
  if p6 == 0 then
    SCheck = "k[] output"
    kIn[] fillarray 1, 2, 3
    kOut[] ControlOut 0.5, kIn, p4, p5
  elseif p6 == 1 then
    SCheck = "k[][] output"
    kGrid[][] Grid p4, p5
    kOut[] init 4
    kOut[0] = kGrid[0][0]
    kOut[1] = kGrid[0][1]
    kOut[2] = kGrid[1][0]
    kOut[3] = kGrid[1][1]
  else
    SCheck = "k[] after a k"
    values:k[] = fillarray(1, 2, 3)
    kOut[] = Scale(0.3, values, p4, p5)
  endif
  kWorst = Multiples(kOut)
  if kWorst > 1e-5 then
    printks "factor %g, mode %g, k-period %g: %s %g %g %g\n", \
      0, p4, p5, kPeriod, SCheck, kOut[0], kOut[1], kOut[2]
    exitnowk -1
  endif
  kPeak init 0
  kPeak = max(kPeak, abs(kOut[0]))
  ; Through the two slowest sincs, modes 0 and 1, k takes longer than a
  ; note to arrive.
  if lastcycle() == 1 && kPeak < 0.05 && p5 != 0 && p5 != 1 then
    printks "factor %g, mode %g: %s silent\n", 0, p4, p5, SCheck
    exitnowk -1
  endif
  gkChecked += 1
endin

instr Sweep
  iStart = 0
  iUnder = 0
  while iUnder < 2 do
    iFactor = 2
    while iFactor <= 4 do
      iMode = 0
      while iMode < 10 do
        schedule "AudioOutput", iStart, 0.064, iFactor, iMode, iUnder
        schedule "AudioInput", iStart, 0.064, iFactor, iMode, iUnder
        if iUnder == 0 then
          schedule "Control", iStart, 0.064, iFactor, iMode, 0
          schedule "Control", iStart, 0.064, iFactor, iMode, 1
          schedule "Control", iStart, 0.064, iFactor, iMode, 2
        endif
        iStart += 0.064
        iMode += 1
      od
      iFactor *= 2
    od
    iUnder += 1
  od
endin

instr CheckResults
  ; 140 notes of 48 k-periods.
  if i(gkChecked) != 6720 then
    prints "Expected 6720 k-periods checked, got %g\n", i(gkChecked)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Sweep" 0 0
i "CheckResults" 2.6 0
e 2.7
</CsScore>
</CsoundSynthesizer>
