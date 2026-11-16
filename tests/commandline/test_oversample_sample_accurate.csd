<CsTest>
description = "oversample and undersample run a sample-accurate note's start and end in the local cycles they fall in"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

gkRuns init 0
gkFirstOffset init 0
gkLastEarly init 0
gkSamples init 0
gkKept init 0
gkLast init 0
gkNotes init 0
gkLate init 0

; A new-style UDO called from a resampled one takes that UDO's bounds.
opcode Inner(value:i):a
  aSig:a = linseg:a(value, 10, value)
  xout(aSig)
endop

; Each local cycle that runs records the bounds its opcodes see, and the
; first one what init left in aSig: the cycles before it must not touch it.
; kCount counts the local cycles that ran, and lastcycle must come once
; (checked where the caller runs at the global ksmps).
opcode Over, aa[]k, ii
  iFactor, iMode xin
  oversample iFactor, iMode
  aSig init 1
  aArr[] init 2
  kCount init 0
  kLast = ksmps - 1
  gkRuns += 1
  if gkRuns == 1 then
    gkFirstOffset = offsetsmps()
    gkKept = aSig[kLast]
  endif
  gkLastEarly = earlysmps()
  gkSamples += ksmps - offsetsmps() - earlysmps()
  kCount += 1
  if lastcycle() == 1 then
    gkLast += 1
  endif
  aSig = linseg:a(1, 10, 1)
  aArr[0] = Inner(1)
  aArr[1] = Inner(2)
  xout aSig, aArr, kCount
endop

; The input converter must keep step through cycles that do not run.
opcode Pass, a[], a[]ii
  aIn[], iFactor, iMode xin
  oversample iFactor, iMode
  xout aIn
endop

opcode PassUnder, a[], a[]ii
  aIn[], iFactor, iMode xin
  undersample iFactor, iMode
  xout aIn
endop

opcode Under, aa[]k, ii
  iFactor, iMode xin
  undersample iFactor, iMode
  aArr[] init 2
  kCount init 0
  gkRuns += 1
  if gkRuns == 1 then
    gkFirstOffset = offsetsmps()
  endif
  gkLastEarly = earlysmps()
  gkSamples += ksmps - offsetsmps() - earlysmps()
  kCount += 1
  if lastcycle() == 1 then
    gkLast += 1
  endif
  aSig = linseg:a(1, 10, 1)
  aArr[0] = Inner(1)
  aArr[1] = Inner(2)
  xout aSig, aArr, kCount
endop

; p4: 0 for oversample, 1 for undersample, p5: factor, p6: converter,
; p7: the note's first sample in its global block, p8: its length in
; samples, p9: the caller's ksmps, or 0 for the global one.
instr Check
  setksmps p9
  iFactor = p5
  iFirst = p7
  iEnd = p7 + p8
  ; The caller's first block, and the note's bounds from its start.
  iBase = int(iFirst / ksmps) * ksmps
  iStart = iFirst - iBase
  iStop = iEnd - iBase
  iBlocks = ceil(iStop / ksmps)
  if p4 == 0 then
    ; The note's local samples, at the local rate, in local cycles of ksmps.
    iLocal = ksmps
    iLocalFirst = iStart * iFactor
    iLocalEnd = iStop * iFactor
    iRuns = ceil(iLocalEnd / ksmps) - floor(iLocalFirst / ksmps)
    iOffset = iLocalFirst % ksmps
    iEarly = (ksmps - iLocalEnd % ksmps) % ksmps
    ; These converters return the note one sample late.
    iSilentTo = iFirst + 1
    iFullFrom = iFirst + 1
  else
    ; One local cycle of ksmps / factor, rounded, per caller block, with the
    ; note's bounds rounded up to the next local sample. A block whose only
    ; samples of the note round up past its end has none of it.
    iLocal = round(ksmps / iFactor)
    iOffset = ceil(iStart * iLocal / ksmps)
    iRuns = iBlocks
    iSilentTo = iBase + ceil(iOffset * ksmps / iLocal)
    if iOffset == iLocal then
      iRuns -= 1
      iOffset = 0
    endif
    iEarly = iLocal - ceil((iStop - (iBlocks - 1) * ksmps) * iLocal / ksmps)
    ; Silence to the note's first local sample, then two of them late.
    iFullFrom = iSilentTo + 2 * ceil(ksmps / iLocal)
  endif
  ; The sinc converters ring through the note and may start on its first
  ; sample: check only its outside.
  if p6 % 5 < 3 then
    iSilentTo = iFirst
    iFullFrom = iEnd
  endif
  iSamples = iRuns * iLocal - iOffset - iEarly
  ; Through PassUnder, the ramp is whole three local samples into the note;
  ; when the note's first block is skipped, one sample after its silence, as
  ; the input converter has taken the note's first sample in that block.
  iRampFrom = iSilentTo + (iRuns < iBlocks ? ceil(ksmps / iLocal) + 1 : \
                                             3 * ceil(ksmps / iLocal))
  ; A float build steps it by 1 within 1.6e-5 at factor 3, a double one 3e-14;
  ; steps of a half and a quarter are exact in both.
  iRampError = 1 + 1e-10 == 1 && iFactor == 3 ? 1e-4 : 1e-9
  ; Init clears aSig up to the note's start in its global block, so its last
  ; sample keeps the 1 only when that start is in the caller's block.
  iKept = p4 == 0 && iFirst < ksmps ? 1 : 0
  gkRuns init 0
  gkSamples init 0
  gkKept init 0
  gkLast init 0
  kPos init iBase
  kPrevious init 0
  ; A ramp from 1 at the caller's first sample, on every sample of its
  ; blocks, through Pass or PassUnder.
  aRamp init 0
  kSample = 0
  while kSample < ksmps do
    vaset kPos + kSample + 1, kSample, aRamp
    kSample += 1
  od
  aIn[] init 2
  aIn[0] = aRamp
  aIn[1] = aRamp
  if p4 == 0 then
    aOut, aArr[], kCount Over iFactor, p6
    aPass[] Pass aIn, iFactor, p6
  else
    aOut, aArr[], kCount Under iFactor, p6
    aPass[] PassUnder aIn, iFactor, p6
  endif
  ; A k output that is not converted leaves with the last local cycle that
  ; ran, also when a cycle after it did not run.
  if (p4 == 1 || p6 >= 5) && kCount != gkRuns then
    printks "%s %d, converter %d, note %d+%d: k output %d after %d cycles\n", \
            0, p4 == 0 ? "oversample" : "undersample", iFactor, p6, \
            iFirst, p8, kCount, gkRuns
    exitnowk -1
  endif
  kSample = 0
  while kSample < ksmps do
    kAt = kPos + kSample
    ; vaget reads a member's whole block, where an assignment would clear
    ; the samples outside the note.
    kOne = vaget(kSample, aArr[0])
    kTwo = vaget(kSample, aArr[1])
    kExpected = -1
    if kAt < iSilentTo || kAt >= iEnd then
      kExpected = 0
    elseif kAt >= iFullFrom && kAt < iEnd then
      kExpected = 1
    endif
    if kExpected >= 0 && (aOut[kSample] != kExpected || \
        kOne != kExpected || kTwo != 2 * kExpected) then
      printks "%s %d, converter %d, note %d+%d: sample %d is %g %g %g, \
not %g\n", 0, p4 == 0 ? "oversample" : "undersample", iFactor, p6, \
              iFirst, p8, kAt, aOut[kSample], kOne, kTwo, kExpected
      exitnowk -1
    endif
    ; The ramp comes back a few samples late, rising by 1 a sample, and
    ; silent outside the note. Under zero-order hold, a note that starts on
    ; a local cycle's first sample returns the silence of the cycle before,
    ; and so does a block that undersample skips. Through undersample, only
    ; linear conversion returns a ramp, exact to the build's precision.
    kPassed = vaget(kSample, aPass[0])
    if p4 == 0 && p6 % 5 > 2 then
      if (kAt >= iFirst + 4 && kAt < iEnd && kPassed != kPrevious + 1) || \
         (kAt == iFirst && iLocalFirst % ksmps == 0 && p6 % 5 == 3 && \
          kPassed != 0) || ((kAt < iFirst || kAt >= iEnd) && kPassed != 0) then
        printks "oversample %d, converter %d, note %d+%d: sample %d passed \
as %g after %g\n", 0, iFactor, p6, iFirst, p8, kAt, kPassed, kPrevious
        exitnowk -1
      endif
    elseif p4 == 1 then
      if (p6 % 5 == 4 && kAt >= iRampFrom && kAt < iEnd && \
          abs(kPassed - kPrevious - 1) > iRampError) || \
         ((kAt < iFirst || kAt >= iEnd || \
           (iRuns < iBlocks && kAt < iSilentTo)) && kPassed != 0) then
        printks "undersample %d, converter %d, note %d+%d: sample %d passed \
as %g after %g\n", 0, iFactor, p6, iFirst, p8, kAt, kPassed, kPrevious
        exitnowk -1
      endif
    endif
    kPrevious = kPassed
    kSample += 1
  od
  kPos += ksmps
  if lastcycle() == 1 then
    if gkRuns != iRuns || gkFirstOffset != iOffset || \
       gkLastEarly != iEarly || gkSamples != iSamples || \
       (iKept == 1 && gkKept != 1) || (p9 == 0 && gkLast != 1) then
      printks "%s %d, converter %d, note %d+%d: %d local cycles of %d \
samples from offset %d to early %d, kept %g, lastcycle %d times, not %d of \
%d from %d to %d\n", 0, p4 == 0 ? "oversample" : "undersample", iFactor, \
              p6, iFirst, p8, gkRuns, gkSamples, gkFirstOffset, gkLastEarly, \
              gkKept, gkLast, iRuns, iSamples, iOffset, iEarly
      exitnowk -1
    endif
    gkNotes += 1
  endif
endin

; An oversampled UDO first called on the caller's second block takes that
; block's bounds, not the note's start.
instr Late
  iEnd = p5 + p6
  kPos init 0
  if kPos >= ksmps then
    aOut, aArr[], kCount Over p4, 4
    kSample = 0
    while kSample < ksmps do
      kAt = kPos + kSample
      if (kAt >= ksmps + 2 && kAt < iEnd && aOut[kSample] != 1) || \
         (kAt >= iEnd && aOut[kSample] != 0) then
        printks "oversample %d first called on block 2, note %d+%d: \
sample %d is %g\n", 0, p4, p5, p6, kAt, aOut[kSample]
        exitnowk -1
      endif
      kSample += 1
    od
    gkLate += 1
  endif
  kPos += ksmps
endin

giNote init 0
instr Case
  ; One note every 10 blocks, each starting p7 samples into its block. Its
  ; start and length are half and a quarter of a sample over p7 and p8, so
  ; both truncate to them and its end time is never a whole sample: on a
  ; block boundary, float and double p-fields round it to different blocks.
  ; No p7 + p8 is a multiple of ksmps, where the engine plays one block more.
  giNote += 1
  schedule("Check", (giNote * 10 * ksmps + p7 + 0.5) / sr, (p8 + 0.25) / sr, \
           p4, p5, p6, p7, p8, p9)
endin

instr CheckResults
  if i(gkNotes) != 56 || i(gkLate) != 4 then
    prints "Expected 56 notes and 4 late blocks checked, got %g and %g\n", \
           i(gkNotes), i(gkLate)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Notes from sample 16 for 144 samples, from 20 for 150, from 40 for 100, and
; from 8 for 20 within one block, at factors 2 and 4, through zero-order hold
; and linear conversion and their held-k modes, and two sinc converters.
#define NOTES(F'C) #
i "Case" 0 0 0 $F $C 16 144 0
i "Case" 0 0 0 $F $C 20 150 0
i "Case" 0 0 0 $F $C 40 100 0
i "Case" 0 0 0 $F $C 8 20 0
#
$NOTES(2'3)
$NOTES(4'3)
$NOTES(2'4)
$NOTES(4'4)
$NOTES(2'8)
$NOTES(4'8)
$NOTES(2'9)
$NOTES(4'9)
$NOTES(2'0)
$NOTES(4'2)
; undersample, with notes whose first sample rounds up to the next block,
; each after a note that leaves its outputs at 1: the block it skips must
; give the converter silence. Factor 3 gives local blocks of 21 samples.
i "Case" 0 0 1 2 3 16 144 0
i "Case" 0 0 1 2 4 20 150 0
i "Case" 0 0 1 4 4 40 100 0
i "Case" 0 0 1 2 4 16 111 0
i "Case" 0 0 1 2 4 63 66 0
i "Case" 0 0 1 4 3 16 111 0
i "Case" 0 0 1 4 3 62 70 0
i "Case" 0 0 1 4 4 8 20 0
i "Case" 0 0 1 3 4 16 144 0
i "Case" 0 0 1 3 4 16 111 0
i "Case" 0 0 1 3 4 62 70 0
; A caller with ksmps 16: its first block starts at 32, with an offset of 8.
i "Case" 0 0 0 2 4 40 100 16
i "Case" 0 0 0 4 9 40 100 16
i "Case" 0 0 0 2 3 20 150 16
i "Case" 0 0 1 2 4 40 100 16
i "Case" 0 0 1 2 4 30 100 16
i "Late" [(64 * 1300 + 16.5) / 48000] [144.5 / 48000] 2 16 144
i "Late" [(64 * 1310 + 16.5) / 48000] [144.5 / 48000] 4 16 144
i "CheckResults" 1.9 0
e 2
</CsScore>
</CsoundSynthesizer>
