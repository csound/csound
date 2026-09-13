<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gkSaved1 init 0
gkSaved2 init 0
gkNotes init 0

instr 1, 2
  setksmps p4
  iSlot = p1
  iSize = p4
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart - iStart%iSize
  iPrevious = (iSlot == 1 ? i(gkSaved1) : i(gkSaved2))
  kPrevious init (p5 == 0 ? 0 : iPrevious)
  kCycle init 0
  aSource phasor sr/32
  aSource += 1
  aSeparate delay1 aSource, p5
  aReuse = aSource
  aReuse delay1 aReuse, p5
  kN = 0
  while kN < iSize do
    kTime = iBlock + kCycle*iSize + kN
    kInput vaget kN, aSource
    kSeparate vaget kN, aSeparate
    kReuse vaget kN, aReuse
    if kTime >= iStart && kTime < iEnd then
      kExpected = kPrevious
      kPrevious = kInput
    else
      kExpected = 0
    endif
    if kSeparate != kExpected || kReuse != kExpected then
      printks "delay1 mismatch at sample %g, block size %g: %g, %g, expected %g\n", 0, kTime, iSize, kSeparate, kReuse, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  if iSlot == 1 then
    gkSaved1 = kPrevious
  else
    gkSaved2 = kPrevious
  endif
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 99
  if i(gkNotes) != 8 then
    prints "delay1 tests did not run every note\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full and partial blocks, including one active sample, with reset and preserved state.
i 1 0 .03125 16 0
i 1 .0654296875 .0205078125 16 1
i 1 .1279296875 .0009765625 16 1
i 1 .1904296875 .0205078125 16 0
; One-sample blocks must retain the original input for the next cycle.
i 2 .25 .0078125 1 0
i 2 .28125 .0078125 1 1
i 2 .3125 .0009765625 1 1
i 2 .34375 .0078125 1 0
i 99 .5 0
e
</CsScore>
</CsoundSynthesizer>
